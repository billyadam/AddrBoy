#ifndef ADDRBOY_STORAGE_H
#define ADDRBOY_STORAGE_H
#include "model.h"
namespace addr
{
enum class LoadResult
{
    ready,
    empty,
    incompatible,
    damaged
};
// 65 record slots: 64 live wallets plus one spare for copy-on-write edits.
// A checksummed pair of directories commits order and preferences atomically.
class Storage
{
public:
    LoadResult load(Book& book);
    bool put(Book& book, int index, const Wallet& wallet);
    bool erase(Book& book, int index);
    bool preferences(Book& book, uint8_t filter, Theme theme, SortOrder sort);
    // Swaps two wallets in the stored (custom) order; one directory commit, no record writes.
    bool swap(Book& book, int a, int b);

private:
    struct Directory
    {
        uint32_t magic;
        uint32_t generation;
        uint8_t version, count, filter, theme, sort, reserved[3];
        uint8_t slots[max_wallets];
        uint32_t checksum;
    };
    Directory _directory = { };
    int _active = 1;
    bool _writable = false;
    bool commit(Directory next);
    bool read_directory(int index, Directory& directory);
};
}
#endif
