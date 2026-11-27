#include "explorers.h"
#include <cstring>
namespace addr
{
namespace
{
constexpr const char* labels[][5] = {
    { "Address", "Blockstream" },
    { "Address", "Etherscan", "Basescan", "Arbiscan", "Robinhood Chain" },
    { "Address", "Solscan" },
    { "Address", "Suiscan" },
    { "Address", "Aptos Explorer" },
};
constexpr const char* prefixes[][5] = {
    { "", "https://blockstream.info/address/" },
    { "", "https://etherscan.io/address/", "https://basescan.org/address/",
        "https://arbiscan.io/address/", "https://robinhoodchain.blockscout.com/address/" },
    { "", "https://solscan.io/account/" },
    { "", "https://suiscan.xyz/mainnet/account/" },
    { "", "https://explorer.aptoslabs.com/account/" },
};
}
int qr_choice_count(Network network)
{
    return network == Network::evm ? 5 : 2;
}
const char* qr_choice_label(Network network, int choice)
{
    if (unsigned(network) >= network_count || choice < 0 || choice >= qr_choice_count(network))
        return nullptr;
    return labels[int(network)][choice];
}
bool qr_payload(Network network, int choice, const char* address, char output[qr_payload_capacity])
{
    if (!qr_choice_label(network, choice) || !address || !address[0])
        return false;
    const char* prefix = prefixes[int(network)][choice];
    size_t prefix_length = std::strlen(prefix), address_length = std::strlen(address);
    if (prefix_length + address_length >= qr_payload_capacity)
        return false;
    std::memcpy(output, prefix, prefix_length);
    std::memcpy(output + prefix_length, address, address_length + 1);
    return true;
}
}
