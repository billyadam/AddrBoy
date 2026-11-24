// make_save: builds an AddrBoy save file (.sav, 32 KB SRAM) from a JSON wallet list.
//
//   make_save wallets.json AddrBoy.sav
//
// It validates and writes with the ROM's own storage.cpp and validation.cpp (on the same SRAM stub the
// tests use), so a save it writes loads in the ROM, and anything the ROM would refuse is reported here
// instead. Nothing is written if any entry is invalid.
//
// JSON format (see wallets.example.json):
//   {
//     "theme": "field" | "net" | "lcd" | "phosphor",        optional, default "field"
//     "filter": "all" | "cold" | "hot" | "exchange" | "external",  optional, default "all"
//     "sort": "a-z" | "z-a" | "custom",                     optional, default "a-z"; "custom" keeps
//                                                           the order of "wallets" below
//     "wallets": [                                          up to 64, in display order
//       { "name": "Main wallet",                            1-20 printable ASCII characters
//         "type": "cold" | "hot" | "exchange" | "external",
//         "btc": "...", "evm": "...", "sol": "...", "sui": "...", "apt": "..." }   each optional
//     ]
//   }

#include "bn_sram.h"
#include "storage.h"
#include "validation.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace addr;

namespace
{
// ---- Minimal JSON reader (objects, arrays, strings, true/false/null; ASCII strings only) ----
struct Value
{
    enum Kind
    {
        null,
        boolean,
        string,
        array,
        object,
    } kind = null;
    std::string text;
    std::vector<Value> items;
    std::vector<std::pair<std::string, Value>> members;
};

struct Parser
{
    const std::string& src;
    size_t pos = 0;
    std::string error;

    int line() const
    {
        int n = 1;
        for (size_t i = 0; i < pos && i < src.size(); ++i)
            n += src[i] == '\n';
        return n;
    }
    bool fail(const std::string& message)
    {
        if (error.empty())
            error = "line " + std::to_string(line()) + ": " + message;
        return false;
    }
    void skip()
    {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\n' || src[pos] == '\r'))
            ++pos;
    }
    bool literal(const char* word)
    {
        size_t n = std::strlen(word);
        if (src.compare(pos, n, word))
            return false;
        pos += n;
        return true;
    }
    bool parse_string(std::string& out)
    {
        if (pos >= src.size() || src[pos] != '"')
            return fail("expected a string");
        ++pos;
        while (pos < src.size() && src[pos] != '"')
        {
            unsigned char c = src[pos++];
            if (c < 32)
                return fail("control character in a string");
            if (c >= 128)
                return fail("only ASCII text is supported");
            if (c != '\\')
            {
                out += char(c);
                continue;
            }
            if (pos >= src.size())
                break;
            char e = src[pos++];
            switch (e)
            {
            case '"':
            case '\\':
            case '/':
                out += e;
                break;
            case 'n':
            case 't':
            case 'r':
            case 'b':
            case 'f':
                return fail("control character in a string");
            case 'u':
            {
                if (pos + 4 > src.size())
                    return fail("bad \\u escape");
                unsigned code = std::stoul(src.substr(pos, 4), nullptr, 16);
                pos += 4;
                if (code < 32 || code > 126)
                    return fail("only printable ASCII is supported");
                out += char(code);
                break;
            }
            default:
                return fail("bad escape in a string");
            }
        }
        if (pos >= src.size())
            return fail("unterminated string");
        ++pos;
        return true;
    }
    bool parse_value(Value& v)
    {
        skip();
        if (pos >= src.size())
            return fail("unexpected end of file");
        char c = src[pos];
        if (c == '"')
        {
            v.kind = Value::string;
            return parse_string(v.text);
        }
        if (c == '{')
        {
            v.kind = Value::object;
            ++pos;
            skip();
            if (pos < src.size() && src[pos] == '}')
                return ++pos, true;
            while (true)
            {
                skip();
                std::string key;
                if (!parse_string(key))
                    return false;
                skip();
                if (pos >= src.size() || src[pos] != ':')
                    return fail("expected ':'");
                ++pos;
                Value member;
                if (!parse_value(member))
                    return false;
                v.members.emplace_back(key, std::move(member));
                skip();
                if (pos < src.size() && src[pos] == ',')
                {
                    ++pos;
                    continue;
                }
                if (pos < src.size() && src[pos] == '}')
                    return ++pos, true;
                return fail("expected ',' or '}'");
            }
        }
        if (c == '[')
        {
            v.kind = Value::array;
            ++pos;
            skip();
            if (pos < src.size() && src[pos] == ']')
                return ++pos, true;
            while (true)
            {
                Value item;
                if (!parse_value(item))
                    return false;
                v.items.push_back(std::move(item));
                skip();
                if (pos < src.size() && src[pos] == ',')
                {
                    ++pos;
                    continue;
                }
                if (pos < src.size() && src[pos] == ']')
                    return ++pos, true;
                return fail("expected ',' or ']'");
            }
        }
        if (literal("null"))
            return v.kind = Value::null, true;
        if (literal("true") || literal("false"))
            return v.kind = Value::boolean, true;
        return fail("expected a string, object, array or null");
    }
};

// Looks up key in an object; unknown keys are reported so typos (e.g. "eth") don't pass silently.
const Value* member(const Value& object, const char* key)
{
    for (const auto& [name, value] : object.members)
        if (name == key)
            return &value;
    return nullptr;
}
bool only_keys(const Value& object, std::initializer_list<const char*> keys, const std::string& where)
{
    for (const auto& [name, value] : object.members)
    {
        bool known = false;
        for (const char* k : keys)
            known |= name == k;
        if (!known)
        {
            std::fprintf(stderr, "%s: unknown key \"%s\"\n", where.c_str(), name.c_str());
            return false;
        }
    }
    return true;
}
// Index of text in choices, or -1.
int choice(const Value* v, std::initializer_list<const char*> choices)
{
    if (!v || v->kind != Value::string)
        return -1;
    int i = 0;
    for (const char* c : choices)
    {
        if (v->text == c)
            return i;
        ++i;
    }
    return -1;
}
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::fprintf(stderr, "usage: make_save WALLETS.json OUTPUT.sav\n");
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in)
    {
        std::fprintf(stderr, "%s: cannot open\n", argv[1]);
        return 2;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string source = buffer.str();
    Parser parser { source, 0, {} };
    Value root;
    if (!parser.parse_value(root) || (parser.skip(), parser.pos != source.size() && parser.fail("extra text")))
    {
        std::fprintf(stderr, "%s: %s\n", argv[1], parser.error.c_str());
        return 1;
    }
    std::string file = argv[1];
    if (root.kind != Value::object || !only_keys(root, { "theme", "filter", "sort", "wallets" }, file))
    {
        if (root.kind != Value::object)
            std::fprintf(stderr, "%s: the top level must be an object\n", argv[1]);
        return 1;
    }

    // Preferences.
    int theme = 0, filter = 0;
    if (const Value* v = member(root, "theme"); v && (theme = choice(v, { "field", "net", "lcd", "phosphor" })) < 0)
    {
        std::fprintf(stderr, "%s: theme must be \"field\", \"net\", \"lcd\" or \"phosphor\"\n", argv[1]);
        return 1;
    }
    if (const Value* v = member(root, "filter");
        v && (filter = choice(v, { "all", "cold", "hot", "exchange", "external" })) < 0)
    {
        std::fprintf(stderr, "%s: filter must be \"all\", \"cold\", \"hot\", \"exchange\" or \"external\"\n", argv[1]);
        return 1;
    }

    int sort = 0;
    if (const Value* v = member(root, "sort"); v && (sort = choice(v, { "a-z", "z-a", "custom" })) < 0)
    {
        std::fprintf(stderr, "%s: sort must be \"a-z\", \"z-a\" or \"custom\"\n", argv[1]);
        return 1;
    }

    // Wallets: every entry is checked with the ROM's rules before anything is written.
    const Value* list = member(root, "wallets");
    if (!list || list->kind != Value::array)
    {
        std::fprintf(stderr, "%s: \"wallets\" must be an array\n", argv[1]);
        return 1;
    }
    if (list->items.size() > size_t(max_wallets))
    {
        std::fprintf(stderr, "%s: %zu wallets; at most %d fit\n", argv[1], list->items.size(), max_wallets);
        return 1;
    }
    constexpr const char* keys[] = { "btc", "evm", "sol", "sui", "apt" };
    std::vector<Wallet> wallets;
    bool ok = true;
    for (size_t i = 0; i < list->items.size(); ++i)
    {
        const Value& entry = list->items[i];
        std::string where = file + ": wallet " + std::to_string(i + 1);
        if (entry.kind != Value::object)
        {
            std::fprintf(stderr, "%s: must be an object\n", where.c_str());
            ok = false;
            continue;
        }
        const Value* name = member(entry, "name");
        if (name && name->kind == Value::string)
            where += " (\"" + name->text + "\")";
        if (!only_keys(entry, { "name", "type", "btc", "evm", "sol", "sui", "apt" }, where))
        {
            ok = false;
            continue;
        }
        Wallet w;
        bool has_text = false;
        if (!name || name->kind != Value::string || name->text.empty() || name->text.size() > size_t(name_limit))
        {
            std::fprintf(stderr, "%s: name must be 1-%d characters\n", where.c_str(), name_limit);
            ok = false;
        }
        else
        {
            for (char c : name->text)
                has_text |= c != ' ';
            if (!has_text)
            {
                std::fprintf(stderr, "%s: name cannot be only spaces\n", where.c_str());
                ok = false;
            }
            std::strcpy(w.name, name->text.c_str());
        }
        int type = choice(member(entry, "type"), { "cold", "hot", "exchange", "external" });
        if (type < 0)
        {
            std::fprintf(stderr, "%s: type must be \"cold\", \"hot\", \"exchange\" or \"external\"\n", where.c_str());
            ok = false;
        }
        else
            w.type = WalletType(type);
        for (int n = 0; n < network_count; ++n)
        {
            const Value* v = member(entry, keys[n]);
            if (!v || v->kind == Value::null || (v->kind == Value::string && v->text.empty()))
                continue;
            if (v->kind != Value::string)
            {
                std::fprintf(stderr, "%s: %s must be a string\n", where.c_str(), keys[n]);
                ok = false;
                continue;
            }
            if (v->text.size() >= size_t(address_capacity))
            {
                std::fprintf(stderr, "%s: %s: Address too long\n", where.c_str(), keys[n]);
                ok = false;
                continue;
            }
            // The ROM's validator; it also writes the canonical form (APT padded to 64 digits).
            if (const char* error = validate_address(Network(n), v->text.c_str(), w.addresses[n]))
            {
                std::fprintf(stderr, "%s: %s: %s\n", where.c_str(), keys[n], error);
                ok = false;
            }
        }
        if (ok && !valid_wallet(w))
        {
            std::fprintf(stderr, "%s: not accepted by the ROM\n", where.c_str());
            ok = false;
        }
        wallets.push_back(w);
    }
    if (!ok)
    {
        std::fprintf(stderr, "No save written.\n");
        return 1;
    }

    // Write with the ROM's storage code, onto a fresh (zero-filled) SRAM image.
    std::memset(bn::sram::data, 0, bn::sram::size());
    Book book;
    Storage storage;
    if (storage.load(book) != LoadResult::empty)
        return 3;
    for (size_t i = 0; i < wallets.size(); ++i)
        if (!storage.put(book, int(i), wallets[i]))
        {
            std::fprintf(stderr, "wallet %zu: storage refused it\n", i + 1);
            return 3;
        }
    if (!storage.preferences(book, uint8_t(filter), Theme(theme), SortOrder(sort)))
        return 3;

    // Read it back as the ROM will on boot.
    Book check;
    Storage reboot;
    if (reboot.load(check) != LoadResult::ready || std::memcmp(&check, &book, sizeof(book)))
    {
        std::fprintf(stderr, "internal error: the written save does not read back\n");
        return 3;
    }

    std::FILE* out = std::fopen(argv[2], "wb");
    if (!out || std::fwrite(bn::sram::data, 1, bn::sram::size(), out) != size_t(bn::sram::size()))
    {
        std::fprintf(stderr, "%s: cannot write\n", argv[2]);
        return 2;
    }
    std::fclose(out);
    std::printf("%s: %zu wallet%s, theme %s, filter %s, sort %s\n", argv[2], wallets.size(),
        wallets.size() == 1 ? "" : "s", theme_names[theme], filter_names[filter], sort_names[sort]);
    return 0;
}
