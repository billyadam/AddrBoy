#include "validation.h"
#include <cstring>
#include <initializer_list>

namespace addr
{
namespace
{
int hex(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}
char lower(char c)
{
    return c >= 'A' && c <= 'Z' ? char(c + 32) : c;
}
uint32_t rotr(uint32_t x, int n)
{
    return (x >> n) | (x << (32 - n));
}
// SHA-256 for the 21-byte Base58Check payload and its 32-byte first digest.
void sha256(const uint8_t* input, int length, uint8_t out[32])
{
    constexpr uint32_t k[64] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa,
        0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb,
        0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624,
        0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb,
        0xbef9a3f7, 0xc67178f2 };
    uint8_t block[64] = { };
    std::memcpy(block, input, length);
    block[length] = 0x80;
    block[62] = uint8_t(length >> 5);
    block[63] = uint8_t(length << 3);
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = (uint32_t(block[i * 4]) << 24) | (uint32_t(block[i * 4 + 1]) << 16)
            | (uint32_t(block[i * 4 + 2]) << 8) | block[i * 4 + 3];
    for (int i = 16; i < 64; ++i)
    {
        uint32_t a = w[i - 15], b = w[i - 2];
        w[i] = w[i - 16] + (rotr(a, 7) ^ rotr(a, 18) ^ (a >> 3)) + w[i - 7]
            + (rotr(b, 17) ^ rotr(b, 19) ^ (b >> 10));
    }
    uint32_t h[8]
        = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], v = h[7];
    for (int i = 0; i < 64; ++i)
    {
        uint32_t t = v + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
        uint32_t u = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        v = g;
        g = f;
        f = e;
        e = d + t;
        d = c;
        c = b;
        b = a;
        a = t + u;
    }
    uint32_t result[8] = { a + h[0], b + h[1], c + h[2], d + h[3], e + h[4], f + h[5], g + h[6], v + h[7] };
    for (int i = 0; i < 32; ++i)
        out[i] = uint8_t(result[i / 4] >> (24 - (i % 4) * 8));
}
uint64_t rotl64(uint64_t x, int n)
{
    return n ? (x << n) | (x >> (64 - n)) : x;
}
// Legacy Keccak-256, not SHA3-256: EIP-55 uses domain suffix 0x01.
void keccak40(const char* input, uint8_t output[32])
{
    constexpr uint64_t rc[] = { 0x1ULL, 0x8082ULL, 0x800000000000808aULL, 0x8000000080008000ULL, 0x808bULL,
        0x80000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL, 0x8aULL, 0x88ULL, 0x80008009ULL,
        0x8000000aULL, 0x8000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
        0x8000000000008002ULL, 0x8000000000000080ULL, 0x800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
        0x8000000000008080ULL, 0x80000001ULL, 0x8000000080008008ULL };
    constexpr int rotations[]
        = { 0, 1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43, 25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14 };
    uint64_t s[25] = { };
    for (int i = 0; i < 40; ++i)
        s[i / 8] |= uint64_t(uint8_t(input[i])) << ((i % 8) * 8);
    s[5] ^= 1;
    s[16] ^= 0x8000000000000000ULL;
    for (uint64_t round : rc)
    {
        uint64_t c[5], d[5], b[25];
        for (int x = 0; x < 5; ++x)
            c[x] = s[x] ^ s[x + 5] ^ s[x + 10] ^ s[x + 15] ^ s[x + 20];
        for (int x = 0; x < 5; ++x)
            d[x] = c[(x + 4) % 5] ^ rotl64(c[(x + 1) % 5], 1);
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 5; ++x)
            {
                int i = x + 5 * y;
                s[i] ^= d[x];
                b[y + 5 * ((2 * x + 3 * y) % 5)] = rotl64(s[i], rotations[i]);
            }
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 5; ++x)
                s[x + 5 * y] = b[x + 5 * y] ^ ((~b[(x + 1) % 5 + 5 * y]) & b[(x + 2) % 5 + 5 * y]);
        s[0] ^= round;
    }
    for (int i = 0; i < 32; ++i)
        output[i] = uint8_t(s[i / 8] >> ((i % 8) * 8));
}
int base58(const char* input, uint8_t output[40])
{
    constexpr const char* alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    uint8_t bytes[40] = { };
    int leading = 0;
    for (const char* p = input; *p == '1'; ++p)
        ++leading;
    for (const char* p = input; *p; ++p)
    {
        const char* digit = std::strchr(alphabet, *p);
        if (!digit)
            return -1;
        unsigned carry = unsigned(digit - alphabet);
        for (int j = 39; j >= 0; --j)
        {
            carry += unsigned(bytes[j]) * 58;
            bytes[j] = uint8_t(carry);
            carry >>= 8;
        }
        if (carry)
            return -1;
    }
    int start = 0;
    while (start < 40 && !bytes[start])
        ++start;
    int length = leading + 40 - start;
    if (length > 40)
        return -1;
    std::memset(output, 0, leading);
    std::memcpy(output + leading, bytes + start, 40 - start);
    return length;
}
uint32_t polymod(uint32_t chk, uint8_t value)
{
    constexpr uint32_t gen[] = { 0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3 };
    uint32_t top = chk >> 25;
    chk = ((chk & 0x1ffffff) << 5) ^ value;
    for (int i = 0; i < 5; ++i)
        if ((top >> i) & 1)
            chk ^= gen[i];
    return chk;
}
// Mainnet Bitcoin: Native SegWit (bech32, witness version 0, "bc1q": a 20- or 32-byte program),
// Taproot (bech32m, version 1, "bc1p": a 32-byte program), and Base58Check Legacy ("1") / P2SH ("3").
// Other witness versions (2-16) are not used by wallets and are refused.
const char* bitcoin(const char* input, int length)
{
    if (lower(input[0]) == 'b' && lower(input[1]) == 'c' && input[2] == '1')
    {
        if (length < 14)
            return "BTC address too short";
        bool upper = false, low = false;
        uint32_t check = 1;
        for (int i = 0; i < length; ++i)
        {
            upper |= input[i] >= 'A' && input[i] <= 'Z';
            low |= input[i] >= 'a' && input[i] <= 'z';
        }
        if (upper && low)
            return "BTC cannot mix case";
        for (uint8_t v : { uint8_t(3), uint8_t(3), uint8_t(0), uint8_t(2), uint8_t(3) })
            check = polymod(check, v);
        constexpr const char* alphabet = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
        uint8_t values[64];
        for (int i = 3; i < length; ++i)
        {
            const char* p = std::strchr(alphabet, lower(input[i]));
            if (!p)
                return "Invalid bech32 character";
            values[i - 3] = uint8_t(p - alphabet);
            check = polymod(check, values[i - 3]);
        }
        int version = values[0];
        if (version > 1)
            return "Use bc1q or bc1p";
        // Version 0 uses the bech32 constant (1), version 1 bech32m (BIP-350).
        if (check != (version == 0 ? 1u : 0x2bc830a3u))
            return "BTC checksum mismatch";
        unsigned acc = 0;
        int bits = 0, bytes = 0;
        for (int i = 1; i < length - 9; ++i)
        {
            acc = (acc << 5) | values[i];
            bits += 5;
            if (bits >= 8)
            {
                bits -= 8;
                ++bytes;
            }
        }
        if (bits >= 5 || ((acc << (8 - bits)) & 255))
            return "Invalid witness padding";
        if (version == 0 ? bytes != 20 && bytes != 32 : bytes != 32)
            return "Invalid witness length";
        return nullptr;
    }
    if (input[0] != '1' && input[0] != '3')
        return "Use BTC mainnet address";
    uint8_t bytes[40], first[32], second[32];
    if (base58(input, bytes) != 25 || (bytes[0] != 0 && bytes[0] != 5))
        return "Invalid Base58 address";
    sha256(bytes, 21, first);
    sha256(first, 32, second);
    return std::memcmp(bytes + 21, second, 4) ? "BTC checksum mismatch" : nullptr;
}
}
const char* validate_address(Network network, const char* input, char output[address_capacity])
{
    if (unsigned(network) >= network_count)
        return "Unknown network";
    int length = 0;
    while (length < address_capacity && input[length])
        ++length;
    if (!length)
        return "Enter an address";
    if (length > address_limit(network))
        return "Address too long";
    char canonical[address_capacity] = { };
    std::memcpy(canonical, input, length);
    if (network == Network::btc)
    {
        if (const char* error = bitcoin(input, length))
            return error;
    }
    else if (network == Network::sol)
    {
        uint8_t decoded[40];
        if (length < 32 || base58(input, decoded) != 32)
            return "SOL needs 32 decoded bytes";
    }
    else
    {
        if (length < 3 || input[0] != '0' || input[1] != 'x')
            return "Address must start with 0x";
        if (network == Network::evm && length != 42)
            return "EVM needs 40 hex digits";
        if (network == Network::sui && length != 66)
            return "SUI needs 64 hex digits";
        bool low = false, upper = false;
        for (int i = 2; i < length; ++i)
        {
            if (hex(input[i]) < 0)
                return "Only hexadecimal digits";
            low |= input[i] >= 'a' && input[i] <= 'f';
            upper |= input[i] >= 'A' && input[i] <= 'F';
        }
        if (network == Network::evm && low && upper)
        {
            char normalized[40];
            uint8_t digest[32];
            for (int i = 0; i < 40; ++i)
                normalized[i] = lower(input[i + 2]);
            keccak40(normalized, digest);
            for (int i = 0; i < 40; ++i)
                if (normalized[i] >= 'a' && normalized[i] <= 'f')
                {
                    int nibble = (digest[i / 2] >> (i % 2 ? 0 : 4)) & 15;
                    bool is_upper = input[i + 2] >= 'A' && input[i + 2] <= 'F';
                    if (is_upper != (nibble >= 8))
                        return "EIP-55 checksum mismatch";
                }
        }
        if (network == Network::apt && length < 66)
        {
            std::memset(canonical + 2, '0', 64);
            std::memcpy(canonical + 66 - (length - 2), input + 2, length - 2);
        }
    }
    std::memcpy(output, canonical, address_capacity);
    return nullptr;
}
bool valid_wallet(const Wallet& wallet)
{
    if (unsigned(wallet.type) > 2 || !wallet.name[0] || wallet.name[name_limit])
        return false;
    bool has_text = false;
    for (int i = 0; wallet.name[i]; ++i)
    {
        if (wallet.name[i] < 32 || wallet.name[i] > 126)
            return false;
        has_text |= wallet.name[i] != ' ';
    }
    if (!has_text)
        return false;
    char canonical[address_capacity];
    for (int i = 0; i < network_count; ++i)
    {
        if (!std::memchr(wallet.addresses[i], 0, address_capacity))
            return false;
        if (wallet.addresses[i][0] && validate_address(Network(i), wallet.addresses[i], canonical))
            return false;
    }
    return true;
}
const char* bitcoin_kind(const char* address)
{
    if (lower(address[0]) == 'b' && lower(address[1]) == 'c' && address[2] == '1')
        return lower(address[3]) == 'p' ? "Taproot" : "N. SegWit";
    return address[0] == '3' ? "P2SH" : "Legacy";
}
}
