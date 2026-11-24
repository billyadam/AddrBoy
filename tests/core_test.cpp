#include "bn_sram.h"
#include "controller.h"
#include "validation.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace addr;
void good(Network n, const char* s)
{
    char result[address_capacity];
    const char* error = validate_address(n, s, result);
    if (error)
    {
        std::fprintf(stderr, "Rejected %s: %s\n", s, error);
        std::abort();
    }
}
void bad(Network n, const char* s)
{
    char result[address_capacity];
    std::memset(result, 42, sizeof(result));
    assert(validate_address(n, s, result));
    for (char c : result)
        assert(c == 42);
}
void validators()
{
    // EIP-55 official vectors: https://eips.ethereum.org/EIPS/eip-55
    const char* evm[]
        = { "0x52908400098527886E0F7030069857D2E4169EE7", "0x8617E340B3D01FA5F11F306F4090FD50E238070D",
              "0xde709f2102306220921060314715629080e2fb77", "0x27b1fdb04752bbc536007a920d24acb045561c26",
              "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed", "0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359",
              "0xdbF03B407c01E7cD3CBea99509d93f8DDDC8C6FB", "0xD1220A0cf47c7B9Be7A2E6BA89F429762e7b9aDb" };
    for (const char* s : evm)
        good(Network::evm, s);
    for (int v = 4; v < 8; ++v)
        for (int i = 2; i < 42; ++i)
        {
            std::string s = evm[v];
            char c = s[i];
            if (c >= 'a' && c <= 'f')
            {
                s[i] = char(c - 32);
                bad(Network::evm, s.c_str());
            }
            if (c >= 'A' && c <= 'F')
            {
                s[i] = char(c + 32);
                bad(Network::evm, s.c_str());
            }
        }
    bad(Network::evm, "0x1234");
    bad(Network::evm, "0X52908400098527886E0F7030069857D2E4169EE7");
    bad(Network::evm, "0x52908400098527886E0F7030069857D2E4169EEZ");
    // BIP-173 / BIP-350 mainnet vectors: Native SegWit (P2WPKH both cases, P2WSH), Taproot,
    // plus Base58Check Legacy and P2SH.
    const char* btc[] = { "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4", "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4",
        "bc1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3qccfmv3",
        "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0", "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
        "3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy", "1111111111111111111114oLvT2" };
    for (const char* s : btc)
        good(Network::btc, s);
    const char* kinds[] = { "N. SegWit", "N. SegWit", "N. SegWit", "Taproot", "Legacy", "P2SH", "Legacy" };
    for (int i = 0; i < 7; ++i)
        assert(!std::strcmp(bitcoin_kind(btc[i]), kinds[i]));
    // Witness versions 2-16 are valid bech32m but unused by wallets: refused. So is a bad checksum.
    const char* other_kinds[]
        = { "BC1SW50QGDZ25J", "bc1zw508d6qejxtdg4y5r3zarvaryvaxxpcs", "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t5" };
    for (const char* s : other_kinds)
        bad(Network::btc, s);
    const char* invalid[] = { "tc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vq5zuyut",
        "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqh2y7hd",
        "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kemeawh",
        "bc1p38j9r5y49hruaue7wxjce0updqjuyyx0kh56v8s25huc6995vvpql3jow4",
        "BC130XLXVLHEMJA6C4DQV22UAPCTQUPFHLXM9H8Z3K2E72Q4K9HCZ7VQ7ZWS8R", "bc1pw5dgrnzv",
        "BC1QR508D6QEJXTDG4Y5R3ZARVARYV98GJ9P", "bc1gmk9yu", "bC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4",
        "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNb", "3J98t1WpEZ73CNmQviecrnyiWrnqRhWNL0" };
    for (const char* s : invalid)
        bad(Network::btc, s);
    good(Network::sol, "11111111111111111111111111111111");
    good(Network::sol, "So11111111111111111111111111111111111111112");
    bad(Network::sol, "1111111111111111111111111111111");
    bad(Network::sol, "111111111111111111111111111111111");
    bad(Network::sol, "So11111111111111111111111111111111111111110");
    std::string full = "0x" + std::string(64, 'a');
    good(Network::sui, full.c_str());
    good(Network::apt, full.c_str());
    good(Network::apt, "0x1");
    bad(Network::sui, "0x1");
    bad(Network::apt, "0x");
    bad(Network::apt, "0xg");
    char normalized[address_capacity] = "0xAb";
    assert(!validate_address(Network::apt, normalized, normalized));
    assert(std::strlen(normalized) == 66 && std::string(normalized).substr(64) == "Ab");
    for (int n = 0; n < 5; ++n)
    {
        bad(Network(n), "");
        bad(Network(n), std::string(100, 'a').c_str());
    }
    std::puts("Address validation: checksum vectors, case mutations, lengths, normalization passed");
}
Wallet wallet(const char* name = "Main wallet")
{
    Wallet w;
    std::strcpy(w.name, name);
    std::strcpy(w.addresses[0], "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4");
    std::strcpy(w.addresses[1], "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed");
    std::strcpy(w.addresses[2], "So11111111111111111111111111111111111111112");
    for (int n = 3; n < 5; ++n)
    {
        std::strcpy(w.addresses[n], "0x");
        std::memset(w.addresses[n] + 2, 'a' + n - 3, 64);
    }
    return w;
}
void saves()
{
    std::memset(bn::sram::data, 255, bn::sram::size());
    Book book;
    Storage store;
    assert(store.load(book) == LoadResult::empty);
    for (int i = 0; i < 64; ++i)
    {
        Wallet w = wallet();
        std::strcpy(w.name, ("Wallet " + std::to_string(i)).c_str());
        w.type = WalletType(i % 3);
        assert(store.put(book, i, w));
    }
    assert(book.count == 64 && !store.put(book, 64, wallet()));
    assert(store.preferences(book, 3, Theme::amber));
    Book loaded;
    Storage reboot;
    assert(reboot.load(loaded) == LoadResult::ready);
    assert(!std::memcmp(&book, &loaded, sizeof(book)));
    assert(reboot.erase(loaded, 17));
    assert(loaded.count == 63 && !std::strcmp(loaded.wallets[17].name, "Wallet 18"));
    assert(reboot.put(loaded, 63, wallet("New last")));
    // Power loss after every possible byte of a full-capacity edit.
    std::vector<unsigned char> snapshot(bn::sram::data, bn::sram::data + bn::sram::size());
    Wallet edited = wallet("Edited");
    for (int cutoff = 0; cutoff <= 444; ++cutoff)
    {
        std::memcpy(bn::sram::data, snapshot.data(), snapshot.size());
        Storage current;
        Book current_book;
        assert(current.load(current_book) == LoadResult::ready);
        bn::sram::write_budget = cutoff;
        bool saved = current.put(current_book, 0, edited);
        bn::sram::write_budget = -1;
        Storage after;
        Book recovered;
        assert(after.load(recovered) == LoadResult::ready);
        assert(recovered.count == 64 && recovered.filter == 3 && recovered.theme == Theme::amber);
        assert(!std::strcmp(recovered.wallets[0].name, saved ? "Edited" : "Wallet 0"));
        assert(!std::memcmp(recovered.wallets + 1, loaded.wallets + 1, 63 * sizeof(Wallet)));
    }
    // Interrupted delete and preference commits retain the previous directory.
    for (int operation = 0; operation < 2; ++operation)
        for (int cutoff = 0; cutoff <= 80; ++cutoff)
        {
            std::memcpy(bn::sram::data, snapshot.data(), snapshot.size());
            Storage current;
            Book current_book;
            assert(current.load(current_book) == LoadResult::ready);
            bn::sram::write_budget = cutoff;
            bool saved = operation ? current.preferences(current_book, 1, Theme::lcd)
                                   : current.erase(current_book, 20);
            bn::sram::write_budget = -1;
            Storage after;
            Book recovered;
            assert(after.load(recovered) == LoadResult::ready);
            assert(recovered.count == (saved && !operation ? 63 : 64));
            assert(recovered.filter == (saved && operation ? 1 : 3));
        }
    // A 20-character name survives a reload.
    {
        Wallet longest = wallet("Twenty characters!!!");
        assert(std::strlen(longest.name) == name_limit);
        assert(reboot.put(loaded, 0, longest));
        Storage again;
        Book reread;
        assert(again.load(reread) == LoadResult::ready && !std::strcmp(reread.wallets[0].name, longest.name));
    }
    // A format 1 save (16-character names) is not read, and is left untouched.
    {
        std::memset(bn::sram::data, 0, bn::sram::size());
        Storage fresh;
        Book empty_book;
        assert(fresh.load(empty_book) == LoadResult::empty && fresh.put(empty_book, 0, wallet()));
        bn::sram::data[8] = 1; // directory version byte of both copies
        bn::sram::data[128 + 8] = 1;
        std::vector<unsigned char> old(bn::sram::data, bn::sram::data + bn::sram::size());
        Storage later;
        assert(later.load(empty_book) == LoadResult::damaged);
        assert(!std::memcmp(old.data(), bn::sram::data, old.size()));
    }
    std::memset(bn::sram::data, 0x42, bn::sram::size());
    assert(store.load(book) == LoadResult::damaged && !store.put(book, 0, wallet()));
    for (auto c : bn::sram::data)
        assert(c == 0x42);
    std::puts("Storage: 64 wallets, reload, order, 607 interrupted writes, damaged-save protection passed");
}
void controls()
{
    std::memset(bn::sram::data, 0, bn::sram::size());
    Controller app;
    app.boot();
    assert(app.visible_count() == 0);
    app.press(Button::a);
    assert(app.screen == Screen::wallets);
    app.press(Button::start);
    app.press(Button::a);
    assert(app.screen == Screen::name);
    app.press(Button::start);
    assert(app.message && app.screen == Screen::name);
    // Name keyboard (AuthBoy's): R types a space but never as the first character, L discards,
    // the last key row ends with an apostrophe, and names stop at 20 characters.
    app.press(Button::r);
    assert(!app.input[0]);
    assert(app.keyboard_char(39) == '\'' && app.keyboard_char(38) == '_');
    app.press(Button::l);
    assert(app.screen == Screen::discard);
    app.press(Button::b);
    assert(app.screen == Screen::name);
    for (int i = 0; i < name_limit + 2; ++i)
        app.press(i == 1 ? Button::r : Button::a);
    assert(std::strlen(app.input) == name_limit && app.input[1] == ' ' && app.message);
    while (app.input[1])
        app.press(Button::b);
    app.press(Button::start);
    assert(app.screen == Screen::type);
    app.press(Button::down);
    app.press(Button::a);
    app.press(Button::start);
    assert(app.book.count == 1 && app.book.wallets[0].type == WalletType::hot
        && !network_total(app.book.wallets[0]));
    app.press(Button::a);
    assert(app.screen == Screen::wallets && app.message);
    assert(app.storage.put(app.book, 0, wallet()));
    app.press(Button::a);
    assert(app.screen == Screen::networks && app.network == 0);
    app.press(Button::a);
    assert(app.screen == Screen::receive);
    // The receive screen switches networks with Left/Right (wrapping); Up/Down do nothing there.
    app.press(Button::up);
    app.press(Button::down);
    assert(app.network == 0);
    app.press(Button::left);
    assert(app.network == 4);
    app.press(Button::right);
    assert(app.network == 0);
    app.press(Button::r);
    assert(app.screen == Screen::receive && app.book.filter == 0);
    // SELECT is unused on the receive screen, and opens settings from both home panels.
    app.press(Button::select);
    assert(app.screen == Screen::receive);
    app.press(Button::b);
    app.press(Button::select);
    assert(app.screen == Screen::settings);
    app.press(Button::b);
    assert(app.screen == Screen::networks);
    app.press(Button::b);
    app.press(Button::select);
    assert(app.screen == Screen::settings);
    app.press(Button::b);
    assert(app.screen == Screen::wallets);
    app.press(Button::a);
    app.press(Button::a);
    assert(app.screen == Screen::receive);
    app.press(Button::b);
    app.press(Button::r);
    assert(app.screen == Screen::wallets && app.book.filter == 1);
    app.press(Button::r);
    assert(app.visible_count() == 0);
    app.press(Button::a);
    assert(app.screen == Screen::wallets);
    app.press(Button::l);
    app.press(Button::start);
    app.press(Button::down);
    app.press(Button::down);
    app.press(Button::a);
    assert(app.screen == Screen::setup);
    app.press(Button::a);
    assert(app.screen == Screen::address && app.lowercase);
    const std::string before = app.input;
    // SELECT toggles the case tab; L/R no longer affect it.
    app.press(Button::l);
    app.press(Button::r);
    assert(app.screen == Screen::address && app.lowercase);
    app.press(Button::select);
    assert(!app.lowercase);
    app.press(Button::select);
    assert(app.lowercase && before == app.input);
    std::strcpy(app.input, "invalid");
    app.press(Button::start);
    assert(app.screen == Screen::address && app.message);
    app.input[0] = 0;
    app.press(Button::start);
    assert(app.screen == Screen::remove_address);
    app.press(Button::a);
    assert(app.screen == Screen::setup && !app.draft.addresses[0][0]);
    app.press(Button::b);
    assert(app.screen == Screen::discard);
    app.press(Button::a);
    assert(app.book.wallets[0].addresses[0][0]);
    Controller reboot;
    reboot.boot();
    assert(reboot.book.filter == 1 && reboot.book.count == 1);
    std::puts("Controls: create empty wallet, filtering, QR navigation, validation, removal/discard passed");
}
int main(int argc, char** argv)
{
    validators();
    saves();
    controls();
    if (argc == 2)
    {
        std::memset(bn::sram::data, 255, bn::sram::size());
        Storage store;
        Book book;
        store.load(book);
        const char* names[]
            = { "Main wallet", "Bob's Ledger", "Daily spend", "Binance", "Phantom", "Savings" };
        for (int i = 0; i < 6; ++i)
        {
            Wallet w = wallet(names[i]);
            w.type = WalletType(i % 3);
            assert(store.put(book, i, w));
        }
        FILE* file = std::fopen(argv[1], "wb");
        assert(file);
        std::fwrite(bn::sram::data, 1, bn::sram::size(), file);
        std::fclose(file);
    }
}
