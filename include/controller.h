#ifndef ADDRBOY_CONTROLLER_H
#define ADDRBOY_CONTROLLER_H
#include "storage.h"
namespace addr
{
enum class Screen
{
    wallets,
    networks,
    receive,
    menu,
    name,
    type,
    setup,
    address,
    settings,
    remove_wallet,
    discard,
    remove_address,
    damaged
};
enum class Button
{
    up,
    down,
    left,
    right,
    a,
    b,
    l,
    r,
    start,
    select
};
class Controller
{
public:
    Book book;
    Wallet draft;
    Screen screen = Screen::wallets, return_screen = Screen::wallets, discard_return = Screen::name;
    int selected = 0, network = 0, menu = 0, setup_network = 0, key = 0;
    int edit_index = -1;
    bool lowercase = false, networks_only = false;
    char input[address_capacity] = { };
    const char* message = nullptr;
    Storage storage;
    void boot();
    void press(Button button);
    int visible_count() const;
    int wallet_index() const;
    int visible_index(int position) const;
    char keyboard_char(int position) const;
    int keyboard_columns() const
    {
        return screen == Screen::name ? 10 : 9;
    }

private:
    void begin_edit(bool fresh, bool only_networks = false);
    void save_draft();
    void home();
    void next_network(int delta);
};
}
#endif
