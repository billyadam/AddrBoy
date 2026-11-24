#ifndef ADDRBOY_CONTROLLER_H
#define ADDRBOY_CONTROLLER_H
#include "storage.h"
namespace addr
{
enum class Screen
{
    welcome, // boot title screen (AuthBoy's welcome): A or START continues
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
// START menu items. Move is only offered in the custom sort order, as in AuthBoy.
enum class MenuItem
{
    new_wallet,
    edit_wallet,
    networks,
    move,
    remove
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
    bool moving = false;    // custom order: Up/Down move the selected wallet
    bool damaged_save = false; // the welcome screen leads to the recovery screen
    int settings_item = 0;  // 0 Sort, 1 Theme
    int order[max_wallets] = { }; // display order: indices into book.wallets
    char input[address_capacity] = { };
    const char* message = nullptr;
    Storage storage;
    void boot();
    void press(Button button);
    int visible_count() const;
    int wallet_index() const;
    int visible_index(int position) const;
    int menu_count() const
    {
        return book.sort == SortOrder::custom ? 5 : 4;
    }
    MenuItem menu_item(int position) const;
    char keyboard_char(int position) const;
    int keyboard_columns() const
    {
        return screen == Screen::name ? 10 : 9;
    }

private:
    void handle(Button button);
    void refresh_order();
    int position_of(int index) const;
    void begin_edit(bool fresh, bool only_networks = false);
    void save_draft();
    void home();
    void next_network(int delta);
};
}
#endif
