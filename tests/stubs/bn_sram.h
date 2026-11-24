#ifndef TEST_BN_SRAM_H
#define TEST_BN_SRAM_H
#include <cassert>
#include <cstring>
namespace bn::sram
{
inline unsigned char data[32768];
inline int write_budget = -1;
constexpr int size()
{
    return sizeof(data);
}
template <class T> void read_offset(T& value, int offset)
{
    assert(offset >= 0 && offset + sizeof(T) <= sizeof(data));
    std::memcpy(&value, data + offset, sizeof(T));
}
template <class T> void write_offset(const T& value, int offset)
{
    assert(offset >= 0 && offset + sizeof(T) <= sizeof(data));
    auto* bytes = reinterpret_cast<const unsigned char*>(&value);
    for (unsigned i = 0; i < sizeof(T); ++i)
    {
        if (write_budget == 0)
            return;
        data[offset + i] = bytes[i];
        if (write_budget > 0)
            --write_budget;
    }
}
}
#endif
