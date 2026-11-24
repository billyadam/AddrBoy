#include "controller.h"
#include "validation.h"
#include <cstring>
namespace addr
{
void Controller::boot()
{
    if (storage.load(book) == LoadResult::damaged)
        screen = Screen::damaged;
}
int Controller::visible_count() const
{
    int count = 0;
    for (int i = 0; i < book.count; ++i)
        count += !book.filter || unsigned(book.wallets[i].type) + 1 == book.filter;
    return count;
}
int Controller::visible_index(int position) const
{
    for (int i = 0; i < book.count; ++i)
        if (!book.filter || unsigned(book.wallets[i].type) + 1 == book.filter)
        {
            if (!position--)
                return i;
        }
    return -1;
}
int Controller::wallet_index() const
{
    return visible_index(selected);
}
void Controller::home()
{
    screen = Screen::wallets;
    if (selected >= visible_count())
        selected = visible_count() - 1;
    if (selected < 0)
        selected = 0;
    network = 0;
}
void Controller::next_network(int delta)
{
    int index = wallet_index();
    if (index < 0 || !network_total(book.wallets[index]))
        return;
    do
    {
        network = (network + delta + 5) % 5;
    } while (!book.wallets[index].addresses[network][0]);
}
char Controller::keyboard_char(int position) const
{
    // Name keys as AuthBoy's name keyboard, with ' in place of its @ (R types a space).
    const char* keys = screen == Screen::name ? "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._'"
                                              : "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char c = keys[position];
    if (lowercase && c >= 'A' && c <= 'Z')
        c = char(c + 32);
    return c;
}
void Controller::begin_edit(bool fresh, bool only_networks)
{
    if (fresh && book.count == max_wallets)
    {
        message = "Wallet limit reached (64)";
        return;
    }
    edit_index = fresh ? book.count : wallet_index();
    if (edit_index < 0)
        return;
    draft = fresh ? Wallet { } : book.wallets[edit_index];
    if (fresh && book.filter)
        draft.type = WalletType(book.filter - 1);
    networks_only = only_networks;
    screen = only_networks ? Screen::setup : Screen::name;
    setup_network = 0;
    key = 0;
    lowercase = false;
    std::memset(input, 0, sizeof(input));
    std::strcpy(input, draft.name);
}
void Controller::save_draft()
{
    if (!storage.put(book, edit_index, draft))
    {
        message = "Save failed; try again";
        return;
    }
    home();
    for (int i = 0; i < visible_count(); ++i)
        if (visible_index(i) == edit_index)
            selected = i;
    message = "Wallet saved";
}
void Controller::press(Button b)
{
    if (screen == Screen::damaged)
        return;
    // SELECT opens settings from the home screen (wallets / networks), as in AuthBoy, and switches
    // the keyboard case; it is unused everywhere else.
    bool home_screen = screen == Screen::wallets || screen == Screen::networks;
    if (b == Button::select && !home_screen && screen != Screen::name && screen != Screen::address)
        return;
    message = nullptr;
    int delta = b == Button::up || b == Button::left || b == Button::l ? -1 : 1;
    if (screen == Screen::wallets || screen == Screen::networks || screen == Screen::receive)
    {
        if (b == Button::start)
        {
            return_screen = screen;
            screen = Screen::menu;
            menu = 0;
            return;
        }
        if (b == Button::select)
        {
            return_screen = screen;
            screen = Screen::settings;
            return;
        }
        if ((b == Button::l || b == Button::r) && screen != Screen::receive)
        {
            if (!storage.preferences(book, uint8_t((book.filter + delta + 4) % 4), book.theme))
                message = "Save failed; try again";
            selected = 0;
            home();
            return;
        }
        int index = wallet_index();
        if (b == Button::b)
        {
            if (screen == Screen::receive)
                screen = Screen::networks;
            else
                home();
            return;
        }
        if (index < 0)
            return;
        // Lists move with Up/Down; the receive screen's networks are laid out left to right (its
        // position dots), so Left/Right switch them there.
        bool vertical = b == Button::up || b == Button::down;
        bool horizontal = b == Button::left || b == Button::right;
        if (screen == Screen::wallets && vertical)
            selected = (selected + delta + visible_count()) % visible_count();
        else if ((screen == Screen::networks && vertical) || (screen == Screen::receive && horizontal))
            next_network(delta);
        if (b == Button::a)
        {
            if (!network_total(book.wallets[index]))
            {
                message = "STA > Networks to add";
                return;
            }
            if (screen == Screen::wallets)
            {
                network = 4;
                next_network(1);
                screen = Screen::networks;
            }
            else if (screen == Screen::networks)
                screen = Screen::receive;
        }
        return;
    }
    if (screen == Screen::menu)
    {
        // New wallet / Edit wallet / Networks / Delete (Settings is on SELECT).
        if (b == Button::up || b == Button::down)
            menu = (menu + delta + 4) % 4;
        if (b == Button::b || b == Button::start)
            screen = return_screen;
        if (b == Button::a)
        {
            if (menu == 0)
                begin_edit(true);
            else if (wallet_index() < 0)
                message = "Choose a wallet first";
            else if (menu == 1)
                begin_edit(false);
            else if (menu == 2)
                begin_edit(false, true);
            else
                screen = Screen::remove_wallet;
        }
        return;
    }
    if (screen == Screen::settings)
    {
        if (b == Button::b || b == Button::start)
        {
            screen = return_screen;
            return;
        }
        if (b == Button::a || b == Button::left || b == Button::right)
        {
            Theme theme = Theme((int(book.theme) + delta + 4) % 4);
            if (!storage.preferences(book, book.filter, theme))
                message = "Save failed; try again";
        }
        return;
    }
    if (screen == Screen::remove_wallet)
    {
        if (b == Button::b)
            screen = Screen::menu;
        if (b == Button::a)
        {
            if (storage.erase(book, wallet_index()))
            {
                home();
                message = "Wallet deleted";
            }
            else
                message = "Save failed; try again";
        }
        return;
    }
    if (screen == Screen::discard)
    {
        if (b == Button::b)
            screen = discard_return;
        if (b == Button::a)
        {
            home();
            message = "Changes discarded";
        }
        return;
    }
    if (screen == Screen::remove_address)
    {
        if (b == Button::b)
            screen = Screen::address;
        if (b == Button::a)
        {
            draft.addresses[setup_network][0] = 0;
            screen = Screen::setup;
        }
        return;
    }
    if (screen == Screen::type)
    {
        if (b == Button::up || b == Button::down)
            draft.type = WalletType((int(draft.type) + delta + 3) % 3);
        if (b == Button::a || b == Button::start)
            screen = Screen::setup;
        if (b == Button::b)
        {
            screen = Screen::name;
            std::strcpy(input, draft.name);
        }
        return;
    }
    if (screen == Screen::setup)
    {
        if (b == Button::up || b == Button::down)
            setup_network = (setup_network + delta + 5) % 5;
        if (b == Button::start)
            save_draft();
        if (b == Button::b)
        {
            if (networks_only)
            {
                discard_return = screen;
                screen = Screen::discard;
            }
            else
                screen = Screen::type;
        }
        if (b == Button::a)
        {
            screen = Screen::address;
            std::strcpy(input, draft.addresses[setup_network]);
            key = 0;
            lowercase = true;
        }
        return;
    }
    if (screen == Screen::name || screen == Screen::address)
    {
        int columns = keyboard_columns(), row = key / columns, column = key % columns;
        if (b == Button::up || b == Button::down)
            row = (row + delta + 4) % 4;
        if (b == Button::left || b == Button::right)
            column = (column + delta + columns) % columns;
        key = row * columns + column;
        if (b == Button::select)
            lowercase = !lowercase;
        int size = int(std::strlen(input));
        // Name keyboard, as in AuthBoy: R types a space (not as the first character), L cancels
        // (here: the discard confirmation). The address keyboard ignores L and R.
        if (screen == Screen::name && b == Button::r)
        {
            if (size && size < name_limit)
            {
                input[size] = ' ';
                input[size + 1] = 0;
            }
            else if (size)
                message = "Character limit reached";
            return;
        }
        if (screen == Screen::name && b == Button::l)
        {
            discard_return = screen;
            screen = Screen::discard;
            return;
        }
        if (b == Button::b)
        {
            if (size)
                input[size - 1] = 0;
            else if (screen == Screen::name)
            {
                discard_return = screen;
                screen = Screen::discard;
            }
            else
                screen = Screen::setup;
        }
        if (b == Button::a)
        {
            int limit = screen == Screen::name ? name_limit : address_limit(Network(setup_network));
            if (size < limit)
            {
                input[size] = keyboard_char(key);
                input[size + 1] = 0;
            }
            else
                message = "Character limit reached";
        }
        if (b == Button::start)
        {
            if (screen == Screen::name)
            {
                bool nonspace = false;
                for (int i = 0; i < size; ++i)
                    nonspace |= input[i] != ' ';
                if (!nonspace)
                {
                    message = "Enter a wallet name";
                    return;
                }
                std::strcpy(draft.name, input);
                screen = Screen::type;
            }
            else if (!size && draft.addresses[setup_network][0])
                screen = Screen::remove_address;
            else
            {
                char result[address_capacity];
                if (const char* error = validate_address(Network(setup_network), input, result))
                    message = error;
                else
                {
                    std::memcpy(draft.addresses[setup_network], result, sizeof(result));
                    screen = Screen::setup;
                }
            }
        }
    }
}
}
