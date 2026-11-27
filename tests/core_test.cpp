#include "bn_sram.h"
#include "controller.h"
#include "validation.h"
#include "dates.h"
#include "qrcodegen.h"
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
void explorer_links()
{
    Wallet w = wallet();
    const char* expected[][6] = {
        { "", "https://blockstream.info/address/" },
        { "", "https://etherscan.io/address/", "https://basescan.org/address/",
            "https://arbiscan.io/address/", "https://robinhoodchain.blockscout.com/address/",
            "https://bscscan.com/address/" },
        { "", "https://solscan.io/account/", "https://explorer.solana.com/address/" },
        { "", "https://suiscan.xyz/mainnet/account/", "https://suivision.xyz/account/" },
        { "", "https://explorer.aptoslabs.com/account/" },
    };
    for (int n = 0; n < network_count; ++n)
    {
        int count = qr_choice_count(Network(n));
        assert(count == (n == 1 ? 6 : n == 2 || n == 3 ? 3 : 2));
        assert(!qr_choice_label(Network(n), count));
        for (int choice = 0; choice < count; ++choice)
        {
            char result[qr_payload_capacity] = { };
            assert(qr_choice_label(Network(n), choice));
            assert(qr_payload(Network(n), choice, w.addresses[n], result));
            assert(std::string(result) == std::string(expected[n][choice]) + w.addresses[n]);
            assert(std::strlen(result) <= 105); // fits a version 5, ECC L byte-mode QR
            uint8_t data[qrcodegen_BUFFER_LEN_FOR_VERSION(5)] = { };
            uint8_t qr[qrcodegen_BUFFER_LEN_FOR_VERSION(5)] = { };
            size_t length = std::strlen(result);
            std::memcpy(data, result, length);
            assert(qrcodegen_encodeBinary(data, length, qr, qrcodegen_Ecc_LOW, 3, 5,
                qrcodegen_Mask_AUTO, false));
            assert((qrcodegen_getSize(qr) + 8) * 2 <= 112);
        }
    }
    char result[qr_payload_capacity];
    assert(!qr_payload(Network::evm, 6, w.addresses[1], result));
    assert(!qr_payload(Network::evm, 1, "", result));
}
void dates_and_checks()
{
    assert(day_from_ymd(2000, 1, 1) == 1);
    assert(day_from_ymd(2024, 2, 29));
    assert(!day_from_ymd(2025, 2, 29));
    assert(!day_from_ymd(2100, 1, 1));
    int year, month, day;
    ymd_from_day(day_from_ymd(2099, 12, 31), year, month, day);
    assert(year == 2099 && month == 12 && day == 31);
    uint16_t stored = day_from_ymd(2026, 10, 28);
    assert(age_days(day_from_ymd(2026, 11, 27), stored) == 30);
    assert(age_days(day_from_ymd(2026, 11, 28), stored) == 31);
    assert(age_days(0, stored) == -1 && age_days(stored - 1, stored) == -1);

    std::memset(bn::sram::data, 0, bn::sram::size());
    Controller app;
    app.today = day_from_ymd(2026, 11, 28);
    app.boot();
    app.press(Button::a);
    Wallet exchange = wallet("Deposit");
    exchange.type = WalletType::exchange;
    exchange.stored_day[0] = exchange.checked_day[0] = stored;
    assert(app.storage.put(app.book, 0, exchange));
    app.press(Button::a);
    app.press(Button::a);
    assert(app.screen == Screen::qr_choices && app.network == 0);
    app.press(Button::select);
    assert(app.screen == Screen::confirm_checked);
    app.press(Button::b);
    assert(app.screen == Screen::qr_choices && app.book.wallets[0].checked_day[0] == stored);
    app.press(Button::select);
    app.press(Button::a);
    assert(app.screen == Screen::qr_choices && app.book.wallets[0].stored_day[0] == stored
        && app.book.wallets[0].checked_day[0] == app.today);
    Book reloaded;
    Storage reboot;
    assert(reboot.load(reloaded) == LoadResult::ready && reloaded.wallets[0].checked_day[0] == app.today);
    app.today = 0;
    app.press(Button::select);
    assert(app.screen == Screen::qr_choices && app.message);
    app.press(Button::a);
    assert(app.screen == Screen::receive); // RTC failure never blocks QR.
    app.today = day_from_ymd(2026, 12, 1);
    app.press(Button::b);
    app.press(Button::b);
    app.press(Button::start);
    app.press(Button::down);
    app.press(Button::down);
    app.press(Button::a);
    assert(app.screen == Screen::setup);
    app.press(Button::a);
    assert(app.screen == Screen::address);
    app.press(Button::start);
    assert(app.screen == Screen::setup && app.draft.stored_day[0] == stored
        && app.draft.checked_day[0] == day_from_ymd(2026, 11, 28));
    app.press(Button::a);
    std::strcpy(app.input, "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa");
    app.press(Button::start);
    assert(app.screen == Screen::setup && app.draft.stored_day[0] == app.today
        && app.draft.checked_day[0] == app.today);
    app.press(Button::start);
    assert(app.book.wallets[0].stored_day[0] == app.today);
    std::puts("Dates: leap years, 30-day boundary, check action, RTC fallback passed");
}
uint32_t directory_crc(const uint8_t* data, int size)
{
    uint32_t value = ~0u;
    for (int i = 0; i < size; ++i)
    {
        value ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ (0xedb88320u & (0u - (value & 1u)));
    }
    return ~value;
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
    assert(store.preferences(book, 3, Theme::amber, SortOrder::za));
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
    for (int cutoff = 0; cutoff <= 480; ++cutoff)
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
    // Interrupted delete, preference and move commits retain the previous directory.
    for (int operation = 0; operation < 3; ++operation)
        for (int cutoff = 0; cutoff <= 84; ++cutoff)
        {
            std::memcpy(bn::sram::data, snapshot.data(), snapshot.size());
            Storage current;
            Book current_book;
            assert(current.load(current_book) == LoadResult::ready);
            bn::sram::write_budget = cutoff;
            bool saved = operation == 1 ? current.preferences(current_book, 1, Theme::lcd, SortOrder::custom)
                : operation == 2        ? current.swap(current_book, 0, 1)
                                        : current.erase(current_book, 20);
            bn::sram::write_budget = -1;
            Storage after;
            Book recovered;
            assert(after.load(recovered) == LoadResult::ready);
            assert(recovered.count == (saved && operation == 0 ? 63 : 64));
            assert(recovered.filter == (saved && operation == 1 ? 1 : 3));
            assert(recovered.sort == (saved && operation == 1 ? SortOrder::custom : SortOrder::za));
            assert(!std::strcmp(recovered.wallets[0].name, saved && operation == 2 ? "Wallet 1" : "Wallet 0"));
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
    // A valid old-format directory is identified without overwriting the save.
    {
        std::memset(bn::sram::data, 0, bn::sram::size());
        Storage fresh;
        Book empty_book;
        assert(fresh.load(empty_book) == LoadResult::empty && fresh.put(empty_book, 0, wallet()));
        for (int offset : { 0, 128 })
        {
            bn::sram::data[offset + 8] = 3;
            uint32_t checksum = directory_crc(bn::sram::data + offset, 80);
            std::memcpy(bn::sram::data + offset + 80, &checksum, 4);
        }
        std::vector<unsigned char> old(bn::sram::data, bn::sram::data + bn::sram::size());
        Storage later;
        assert(later.load(empty_book) == LoadResult::incompatible);
        assert(!std::memcmp(old.data(), bn::sram::data, old.size()));
    }
    // A corrupt format 2 save is not read, and is left untouched.
    {
        std::memset(bn::sram::data, 0, bn::sram::size());
        Storage fresh;
        Book empty_book;
        assert(fresh.load(empty_book) == LoadResult::empty && fresh.put(empty_book, 0, wallet()));
        bn::sram::data[8] = 2; // directory version byte of both copies
        bn::sram::data[128 + 8] = 2;
        std::vector<unsigned char> old(bn::sram::data, bn::sram::data + bn::sram::size());
        Storage later;
        assert(later.load(empty_book) == LoadResult::damaged);
        assert(!std::memcmp(old.data(), bn::sram::data, old.size()));
    }
    std::memset(bn::sram::data, 0x42, bn::sram::size());
    {
        // A damaged save: the welcome screen first, then the recovery screen, which ignores input.
        Controller damaged;
        damaged.boot();
        assert(damaged.screen == Screen::welcome && damaged.damaged_save);
        damaged.press(Button::a);
        assert(damaged.screen == Screen::damaged);
        damaged.press(Button::a);
        assert(damaged.screen == Screen::damaged);
    }
    assert(store.load(book) == LoadResult::damaged && !store.put(book, 0, wallet()));
    for (auto c : bn::sram::data)
        assert(c == 0x42);
    std::puts("Storage: 64 wallets, reload, order, 619 interrupted writes, damaged-save protection passed");
}
void controls()
{
    std::memset(bn::sram::data, 0, bn::sram::size());
    Controller app;
    app.boot();
    // Every boot opens on the welcome screen (AuthBoy's): only A or START continue.
    assert(app.screen == Screen::welcome && app.visible_count() == 0);
    for (Button other : { Button::b, Button::up, Button::l, Button::r, Button::select })
        app.press(other);
    assert(app.screen == Screen::welcome && app.book.filter == 0);
    app.press(Button::a);
    assert(app.screen == Screen::wallets);
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
    assert(app.screen == Screen::qr_choices && app.qr_choice == 0);
    app.press(Button::b);
    assert(app.screen == Screen::networks);
    app.press(Button::down);
    assert(app.network == 1);
    app.press(Button::a);
    assert(app.screen == Screen::qr_choices && qr_choice_count(Network(app.network)) == 6);
    for (int i = 0; i < 5; ++i)
        app.press(Button::down);
    assert(app.qr_choice == 5);
    app.press(Button::a);
    assert(app.screen == Screen::receive);
    app.press(Button::right);
    assert(app.network == 1); // Explorer QR stays on its chosen chain.
    app.press(Button::b);
    assert(app.screen == Screen::qr_choices && app.qr_choice == 5);
    app.press(Button::b);
    app.press(Button::up);
    assert(app.screen == Screen::networks && app.network == 0);
    app.press(Button::a);
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
    assert(app.screen == Screen::qr_choices);
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
    app.press(Button::a);
    assert(app.screen == Screen::receive);
    app.press(Button::b);
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
// AuthBoy's sort: A-Z / Z-A by name (case-insensitive, equal names in stored order), or the custom
// order, which Move rearranges.
void sorting()
{
    std::memset(bn::sram::data, 0, bn::sram::size());
    Controller app;
    app.boot();
    app.press(Button::start); // past the welcome screen
    assert(app.book.sort == SortOrder::az);
    const char* names[] = { "bravo", "Alpha", "charlie", "alpha", "Bravo" };
    for (int i = 0; i < 5; ++i)
        assert(app.storage.put(app.book, i, wallet(names[i])));
    auto listed = [&] {
        std::string s;
        for (int i = 0; i < app.visible_count(); ++i)
            s += std::string(app.book.wallets[app.visible_index(i)].name) + ",";
        return s;
    };
    auto highlighted = [&] { return std::string(app.book.wallets[app.wallet_index()].name); };
    app.press(Button::down); // any press refreshes the display order
    assert(listed() == "Alpha,alpha,bravo,Bravo,charlie," && highlighted() == "alpha");
    assert(app.menu_count() == 4);
    // Settings: Sort is the first item; A cycles A-Z -> Z-A -> Custom; the highlight stays on "alpha".
    app.press(Button::select);
    assert(app.screen == Screen::settings && app.settings_item == 0);
    app.press(Button::a);
    assert(app.book.sort == SortOrder::za && listed() == "charlie,bravo,Bravo,Alpha,alpha,");
    assert(highlighted() == "alpha");
    app.press(Button::a);
    assert(app.book.sort == SortOrder::custom && listed() == "bravo,Alpha,charlie,alpha,Bravo,");
    assert(highlighted() == "alpha" && app.menu_count() == 5);
    app.press(Button::b);
    assert(app.screen == Screen::wallets);
    // Move (custom order only): Up/Down move the wallet and stop at the ends; A finishes.
    app.press(Button::start);
    for (int i = 0; i < 3; ++i)
        app.press(Button::down);
    assert(app.menu_item(app.menu) == MenuItem::move);
    app.press(Button::a);
    assert(app.moving && app.screen == Screen::wallets);
    app.press(Button::up);
    app.press(Button::up);
    assert(listed() == "bravo,alpha,Alpha,charlie,Bravo," && app.selected == 1);
    app.press(Button::up);
    app.press(Button::up);
    app.press(Button::start); // ignored while moving
    assert(listed() == "alpha,bravo,Alpha,charlie,Bravo," && app.selected == 0 && app.moving);
    app.press(Button::a);
    assert(!app.moving && app.screen == Screen::wallets);
    // The custom order and the sort setting survive a reboot.
    Controller reboot;
    reboot.boot();
    assert(reboot.book.sort == SortOrder::custom);
    for (int i = 0; i < 5; ++i)
        assert(!std::strcmp(reboot.book.wallets[i].name, app.book.wallets[i].name));
    // Theme is the second settings item.
    app.press(Button::select);
    app.press(Button::down);
    app.press(Button::a);
    assert(app.book.theme == Theme::net && app.book.sort == SortOrder::custom);
    // About (SELECT in Settings): Left/Right and L/R turn the four pages, stopping at the ends;
    // B or SELECT return to Settings.
    assert(app.screen == Screen::settings);
    app.press(Button::select);
    assert(app.screen == Screen::about && app.about_page == 0);
    app.press(Button::left);
    assert(app.about_page == 0);
    for (int i = 0; i < 5; ++i)
        app.press(i % 2 ? Button::r : Button::right);
    assert(app.about_page == Controller::about_page_count - 1);
    app.press(Button::l);
    assert(app.about_page == 2);
    app.press(Button::b);
    assert(app.screen == Screen::settings);
    app.press(Button::select);
    assert(app.about_page == 0);
    app.press(Button::select);
    assert(app.screen == Screen::settings);
    app.press(Button::b);
    assert(app.screen == Screen::wallets);
    std::puts("Sorting: A-Z / Z-A with ties, custom order, Move, settings, About, persistence passed");
}
// The External type (someone else's wallet): saved, filtered, and chosen like the others.
void external_type()
{
    std::memset(bn::sram::data, 0, bn::sram::size());
    Controller app;
    app.boot();
    app.press(Button::start); // past the welcome screen
    Wallet theirs = wallet("Alice");
    theirs.type = WalletType::external;
    assert(app.storage.put(app.book, 0, wallet("Mine")) && app.storage.put(app.book, 1, theirs));
    Wallet bad = wallet("Bad");
    bad.type = WalletType(wallet_type_count);
    assert(!valid_wallet(bad) && !app.storage.put(app.book, 2, bad));
    // Filter: ALL -> COLD -> HOT -> EXCH -> EXT -> ALL.
    for (int i = 0; i < 4; ++i)
        app.press(Button::r);
    assert(app.book.filter == 4 && app.visible_count() == 1 && app.wallet_index() == 1);
    app.press(Button::r);
    assert(app.book.filter == 0 && app.visible_count() == 2);
    // A new wallet made under the EXT filter starts as External; Up/Down cycle all four types.
    app.press(Button::l);
    app.press(Button::start);
    app.press(Button::a);
    app.press(Button::a);
    app.press(Button::start);
    assert(app.screen == Screen::type && app.draft.type == WalletType::external);
    app.press(Button::down);
    assert(app.draft.type == WalletType::cold);
    app.press(Button::up);
    assert(app.draft.type == WalletType::external);
    Controller reboot;
    reboot.boot();
    assert(reboot.book.wallets[1].type == WalletType::external && reboot.book.filter == 4);
    std::puts("External type: save, filter, new wallet type, type cycling passed");
}
int main(int argc, char** argv)
{
    validators();
    explorer_links();
    dates_and_checks();
    saves();
    controls();
    sorting();
    external_type();
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
        // Custom order: the emulator test expects the stored order (a new wallet comes last).
        assert(store.preferences(book, 0, Theme::field, SortOrder::custom));
        FILE* file = std::fopen(argv[1], "wb");
        assert(file);
        std::fwrite(bn::sram::data, 1, bn::sram::size(), file);
        std::fclose(file);
    }
}
