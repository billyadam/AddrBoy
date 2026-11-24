#ifndef ADDRBOY_MODEL_H
#define ADDRBOY_MODEL_H
#include <cstdint>

namespace addr
{
constexpr int max_wallets = 64;
constexpr int network_count = 5;
constexpr int address_capacity = 67;
constexpr int name_limit = 20;
enum class Network : uint8_t
{
    btc,
    evm,
    sol,
    sui,
    apt
};
enum class WalletType : uint8_t
{
    cold,
    hot,
    exchange,
    external // someone else's wallet: how they keep it is unknown
};
constexpr int wallet_type_count = 4;
constexpr int filter_count = wallet_type_count + 1; // ALL plus one per type
// How the wallet list is shown (AuthBoy's sort): A-Z / Z-A by name (case-insensitive), or the stored
// custom order, which Move rearranges. Changing it never changes the stored order.
enum class SortOrder : uint8_t
{
    az,
    za,
    custom
};
enum class Theme : uint8_t
{
    field,
    net,
    lcd,
    amber
};
struct Wallet
{
    char name[name_limit + 1] = { };
    WalletType type = WalletType::cold;
    char addresses[network_count][address_capacity] = { };
};
struct Book
{
    Wallet wallets[max_wallets] = { };
    uint8_t count = 0;
    uint8_t filter = 0;
    Theme theme = Theme::field;
    SortOrder sort = SortOrder::az;
};
inline constexpr const char* tickers[] = { "BTC", "EVM", "SOL", "SUI", "APT" };
inline constexpr const char* network_names[] = { "Bitcoin", "ETH and L2s", "Solana", "Sui", "Aptos" };
inline constexpr const char* type_names[] = { "Cold", "Hot", "Exchange", "External" };
inline constexpr const char* filter_names[] = { "ALL", "COLD", "HOT", "EXCH", "EXT" };
inline constexpr const char* sort_names[] = { "A - Z", "Z - A", "Custom" };
inline constexpr const char* theme_names[] = { "Field Guide", "Net Link", "Pocket LCD", "Phosphor" };
inline int network_total(const Wallet& wallet)
{
    int count = 0;
    for (const auto& address : wallet.addresses)
        count += address[0] != 0;
    return count;
}
inline int address_limit(Network network)
{
    return network == Network::btc ? 62 : network == Network::evm ? 42 : network == Network::sol ? 44 : 66;
}
}
#endif
