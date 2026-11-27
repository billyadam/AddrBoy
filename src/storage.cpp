#include "storage.h"
#include "bn_sram.h"
#include "validation.h"
#include <cstddef>
#include <cstring>
namespace addr
{
namespace
{
constexpr uint32_t magic = 0x52444441; // ADDR, little endian
constexpr int records_offset = 256;
// Format 4 stores per-network dates in each wallet. Earlier formats are not read.
constexpr uint8_t format_version = 4;
struct Record
{
    Wallet wallet;
    uint8_t reserved[2];
    uint32_t checksum;
};
static_assert(sizeof(Wallet) == 378);
static_assert(sizeof(Record) == 384);
static_assert(records_offset + 65 * sizeof(Record) <= bn::sram::size());
uint32_t crc(const void* data, unsigned size)
{
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint32_t value = ~0u;
    for (unsigned i = 0; i < size; ++i)
    {
        value ^= bytes[i];
        for (int b = 0; b < 8; ++b)
            value = (value >> 1) ^ (0xedb88320u & (0u - (value & 1u)));
    }
    return ~value;
}
bool read_record(int slot, Record& record)
{
    if (slot > 64)
        return false;
    bn::sram::read_offset(record, records_offset + slot * sizeof(Record));
    return record.checksum == crc(&record, offsetof(Record, checksum)) && valid_wallet(record.wallet);
}
}
bool Storage::read_directory(int index, Directory& directory)
{
    bn::sram::read_offset(directory, index * 128);
    if (directory.magic != magic || directory.version != format_version || directory.count > max_wallets
        || directory.filter >= filter_count || directory.theme > 3 || directory.sort > 2
        || directory.checksum != crc(&directory, offsetof(Directory, checksum)))
        return false;
    bool used[65] = { };
    Record record;
    for (int i = 0; i < directory.count; ++i)
    {
        unsigned slot = directory.slots[i];
        if (slot > 64 || used[slot] || !read_record(slot, record))
            return false;
        used[slot] = true;
    }
    return true;
}
LoadResult Storage::load(Book& book)
{
    book = Book { };
    _writable = false;
    Directory a, b;
    bool va = read_directory(0, a), vb = read_directory(1, b);
    if (!va && !vb)
    {
        // A v3 directory has the same layout; report it clearly without changing any SRAM bytes.
        for (int i = 0; i < 2; ++i)
        {
            Directory old;
            bn::sram::read_offset(old, i * 128);
            if (old.magic == magic && old.version == 3
                && old.checksum == crc(&old, offsetof(Directory, checksum)))
                return LoadResult::incompatible;
        }
        // Never silently overwrite an unfamiliar or damaged save.
        bool zero = true, erased = true;
        for (int offset = 0; offset < bn::sram::size(); offset += 128)
        {
            uint8_t bytes[128];
            bn::sram::read_offset(bytes, offset);
            for (uint8_t byte : bytes)
            {
                zero &= byte == 0;
                erased &= byte == 255;
            }
        }
        if (!zero && !erased)
            return LoadResult::damaged;
        _directory = { };
        _directory.magic = magic;
        _directory.version = format_version;
        _active = 1;
        _writable = true;
        return LoadResult::empty;
    }
    _active = vb && (!va || int32_t(b.generation - a.generation) > 0) ? 1 : 0;
    _directory = _active ? b : a;
    book.count = _directory.count;
    book.filter = _directory.filter;
    book.theme = Theme(_directory.theme);
    book.sort = SortOrder(_directory.sort);
    for (int i = 0; i < book.count; ++i)
    {
        Record record;
        read_record(_directory.slots[i], record);
        book.wallets[i] = record.wallet;
    }
    _writable = true;
    return LoadResult::ready;
}
bool Storage::commit(Directory next)
{
    if (!_writable)
        return false;
    next.generation = _directory.generation + 1;
    next.checksum = crc(&next, offsetof(Directory, checksum));
    bn::sram::write_offset(next, (1 - _active) * 128);
    Directory verify;
    bn::sram::read_offset(verify, (1 - _active) * 128);
    if (std::memcmp(&next, &verify, sizeof(next)))
        return false;
    _directory = next;
    _active = 1 - _active;
    return true;
}
bool Storage::put(Book& book, int index, const Wallet& wallet)
{
    if (!_writable || index < 0 || index > book.count || index >= max_wallets || !valid_wallet(wallet))
        return false;
    // Establish an empty committed directory before the first record write.
    if (!_directory.generation && !commit(_directory))
        return false;
    bool used[65] = { };
    for (int i = 0; i < _directory.count; ++i)
        used[_directory.slots[i]] = true;
    int spare = 0;
    while (used[spare])
        ++spare;
    Record record = { };
    record.wallet = wallet;
    record.checksum = crc(&record, offsetof(Record, checksum));
    bn::sram::write_offset(record, records_offset + spare * sizeof(Record));
    Record verify;
    bn::sram::read_offset(verify, records_offset + spare * sizeof(Record));
    if (std::memcmp(&record, &verify, sizeof(record)))
        return false;
    Directory next = _directory;
    next.slots[index] = uint8_t(spare);
    if (index == book.count)
        ++next.count;
    if (!commit(next))
        return false;
    book.wallets[index] = wallet;
    book.count = next.count;
    return true;
}
bool Storage::erase(Book& book, int index)
{
    if (!_writable || index < 0 || index >= book.count)
        return false;
    Directory next = _directory;
    for (int i = index; i < next.count - 1; ++i)
        next.slots[i] = next.slots[i + 1];
    next.slots[--next.count] = 0;
    if (!commit(next))
        return false;
    for (int i = index; i < book.count - 1; ++i)
        book.wallets[i] = book.wallets[i + 1];
    book.wallets[--book.count] = Wallet { };
    return true;
}
bool Storage::preferences(Book& book, uint8_t filter, Theme theme, SortOrder sort)
{
    if (filter >= filter_count || unsigned(theme) > 3 || unsigned(sort) > 2)
        return false;
    Directory next = _directory;
    next.filter = filter;
    next.theme = uint8_t(theme);
    next.sort = uint8_t(sort);
    if (!commit(next))
        return false;
    book.filter = filter;
    book.theme = theme;
    book.sort = sort;
    return true;
}
bool Storage::swap(Book& book, int a, int b)
{
    if (!_writable || a < 0 || b < 0 || a >= book.count || b >= book.count || a == b)
        return false;
    Directory next = _directory;
    uint8_t slot = next.slots[a];
    next.slots[a] = next.slots[b];
    next.slots[b] = slot;
    if (!commit(next))
        return false;
    Wallet wallet = book.wallets[a];
    book.wallets[a] = book.wallets[b];
    book.wallets[b] = wallet;
    return true;
}
}
