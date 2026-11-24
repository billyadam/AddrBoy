#include "screens.h"
#include "bn_string.h"
#include "qrcodegen.h"
#include "ui.h"
#include "validation.h"
#include "logo_bookboy.h"
#include <cstdio>
#include <cstring>

// Screens follow addrboy_handoff_theme_mapping: the themes are not color swaps. Each AddrBoy part is
// built from the matching AuthBoy component of the active theme (Field Guide, Net Link, Pocket LCD,
// Phosphor), using the "Theme mapping" element map of the handoff.
//
// Fonts: body for running text, digits for labels, counters, badge letters and addresses.

namespace addr
{
namespace
{
using ui::font;

Theme theme;

// ---- Text roles ----
struct Roles
{
    uint8_t text, dim, danger, accent;
};
constexpr Roles roles_by_theme[] = {
    { ui::ink, ui::secondary, ui::red, ui::red },
    { ui::net_text, ui::net_secondary, ui::net_danger_text, ui::net_green },
    { ui::lcd_dark, ui::lcd_mid, ui::lcd_dark, ui::lcd_dark },
    { ui::amber_text, ui::amber_dim, ui::amber_text, ui::amber_text },
};
Roles role;

constexpr uint8_t type_colors[] = { ui::badge_0, ui::badge_0 + 3, ui::badge_0 + 1, ui::badge_0 + 5 }; // blue, orange, purple, green
constexpr uint8_t coin_colors[] = { ui::badge_0 + 3, ui::badge_0 + 1, ui::badge_0 + 7, ui::badge_0 + 2, ui::ink };
constexpr const char* card_names[] = { "Cold wallet", "Hot wallet", "Exchange", "External" };

// ---- Primitives ----
void text(const char* s, int x, int y, uint8_t color, int max_width = 240, font f = font::body)
{
    bool truncated = false;
    auto fitted = ui::fit_text(f, s, max_width, 1, truncated);
    bn::string<100> label(fitted);
    if (truncated)
        label += "...";
    ui::text(f, label, x, y, color);
}
void line(int x, int y, int width, uint8_t c)
{
    ui::fill(x, y, width, 1, c);
}
void outline(int x, int y, int w, int h, uint8_t c)
{
    line(x, y, w, c);
    line(x, y + h - 1, w, c);
    ui::fill(x, y, 1, h, c);
    ui::fill(x + w - 1, y, 1, h, c);
}
// AuthBoy skin helper: each row moves left by slant / (height - 1) px from top to bottom.
void parallelogram(int x, int y, int width, int height, int slant, uint8_t color, bool left_straight = false)
{
    for (int row = 0; row < height; ++row)
    {
        int shift = height > 1 ? (slant * (height - 1 - row) + (height - 1) / 2) / (height - 1) : 0;
        int left = left_straight ? x : x + shift;
        int right = x + width - slant + shift;
        ui::fill(left, y + row, right - left, 1, color);
    }
}
bn::string<40> upper(const char* s)
{
    bn::string<40> result;
    for (; *s && result.size() < 39; ++s)
        result += *s >= 'a' && *s <= 'z' ? char(*s - 32) : *s;
    return result;
}
// Phosphor dot leader: dots every 4 px, hidden when the ends touch.
void leader(int x0, int x1, int y, uint8_t color)
{
    if (x1 - x0 > 1)
        ui::pattern_hline(x0, y, x1 - x0, 1, 4, color);
}

// ---- Screen, header and footer ----
void background()
{
    switch (theme)
    {
    case Theme::field:
        ui::background();
        break;
    case Theme::net:
        ui::fill(0, 0, 240, 160, ui::net_bg);
        for (int x = 0; x < 240; x += 16)
            ui::fill(x, 0, 1, 160, ui::net_grid);
        for (int y = 0; y < 160; y += 16)
            line(0, y, 240, ui::net_grid);
        break;
    case Theme::lcd:
        ui::fill(0, 0, 240, 160, ui::lcd_screen);
        break;
    case Theme::amber:
    default:
        ui::fill(0, 0, 240, 160, ui::amber_bg);
        break;
    }
}
void header(const char* title)
{
    switch (theme)
    {
    case Theme::field:
        ui::header(title);
        break;
    case Theme::net:
    {
        // Deep band with a cyan title tab (slanted right end).
        ui::fill(0, 0, 240, 22, ui::net_deep);
        line(0, 22, 240, ui::net_cyan);
        parallelogram(0, 3, 6 + ui::text_width(font::title12, title) + 16, 16, 8, ui::net_cyan, true);
        ui::text(font::title12, title, 6, 5, ui::net_deep);
        break;
    }
    case Theme::lcd:
        ui::fill(0, 0, 240, 22, ui::lcd_dark);
        line(0, 22, 240, ui::lcd_mid);
        text(upper(title).c_str(), 6, 7, ui::lcd_screen, 160);
        break;
    case Theme::amber:
    default:
    {
        // No band: "> TITLE" over a dashed rule.
        bn::string<44> label = "> ";
        label += upper(title);
        text(label.c_str(), 6, 7, ui::amber_text, 160);
        ui::pattern_hline(4, 23, 232, 2, 4, ui::amber_frame);
        break;
    }
    }
}
// Right side of the header: AuthBoy's header counter (also used for the RECEIVE tag).
void header_counter(const char* s, bool alert = false, int right = 233)
{
    uint8_t c = 0;
    int x = right, y = 7;
    switch (theme)
    {
    case Theme::field:
        // Danger red would vanish on the red band: alerts use the selection yellow.
        c = alert ? ui::selection : ui::white;
        y = 8;
        break;
    case Theme::net:
        c = alert ? ui::net_danger_text : ui::net_text;
        break;
    case Theme::lcd:
        c = alert ? ui::lcd_screen : ui::lcd_light;
        break;
    case Theme::amber:
    default:
        c = alert ? ui::amber_text : ui::amber_dim;
        x = right + 1;
        break;
    }
    ui::text(font::digits, s, x, y, c, ui::no_shadow, 1, ui::align::right);
}
// New wallet step pips, 14x6 at x 190 / 206 / 222.
void step_pips(int step)
{
    for (int i = 0; i < 3; ++i)
    {
        int x = 190 + i * 16;
        bool on = i < step;
        switch (theme)
        {
        case Theme::field:
            ui::round_fill(x, 7, 14, 6, 2, on ? ui::white : ui::header_shadow);
            break;
        case Theme::net:
            parallelogram(x, 7, 14, 6, 2, on ? ui::net_cyan : ui::net_grid);
            break;
        case Theme::lcd:
            ui::fill(x, 7, 14, 6, on ? ui::lcd_screen : ui::lcd_mid);
            break;
        case Theme::amber:
        default:
            ui::fill(x, 7, 14, 6, on ? ui::amber_text : ui::amber_track);
            break;
        }
    }
}
// Skinned themes' button pill (digits label), label_y being the label's top. Returns its width.
int skin_pill_width(const char* b)
{
    return ui::text_width(font::digits, b) + (theme == Theme::net ? 10 : 6);
}
int skin_pill(int x, int label_y, const char* b)
{
    int w = skin_pill_width(b);
    if (theme == Theme::net)
    {
        parallelogram(x, label_y - 2, w, 11, 3, ui::net_cyan);
        ui::text(font::digits, b, x + 5, label_y, ui::net_deep);
    }
    else if (theme == Theme::lcd)
    {
        ui::round_fill(x, label_y - 2, w, 11, 2, ui::lcd_dark);
        ui::text(font::digits, b, x + 3, label_y, ui::lcd_screen);
    }
    else
    {
        ui::fill(x, label_y - 2, w, 11, ui::amber_text);
        ui::text(font::digits, b, x + 3, label_y, ui::amber_bg);
    }
    return w;
}
// One centered row of footer hints.
void footer_row(int label_y, const ui::hint* hints, int count)
{
    if (theme == Theme::field)
    {
        ui::footer_hints(label_y, hints, count);
        return;
    }
    constexpr int gap = 10;
    int total = 0;
    for (int i = 0; i < count; ++i)
        total += skin_pill_width(hints[i].button) + 3 + ui::text_width(font::body, hints[i].label) + (i ? gap : 0);
    int x = (240 - total) / 2;
    for (int i = 0; i < count; ++i)
    {
        int w = skin_pill(x, label_y, hints[i].button);
        ui::text(font::body, hints[i].label, x + w + 3, label_y, role.text);
        x += w + 3 + ui::text_width(font::body, hints[i].label) + gap;
    }
}
// AuthBoy footers: Field Guide's bar with round A/B buttons, or the skin's bar and pills. The name
// keyboard uses AuthBoy's two-row footer from y 134 (row2: its second row of hints).
void footer(const ui::hint* hints, int count, const char* message = nullptr, int top = 144,
    const ui::hint* row2 = nullptr, int row2_count = 0)
{
    int label_y = top + 3;
    if (theme == Theme::field)
        ui::footer_bar(top);
    else if (theme == Theme::net)
    {
        line(0, top - 1, 240, ui::net_cyan);
        ui::fill(0, top, 240, 160 - top, ui::net_deep);
    }
    else if (theme == Theme::lcd)
        line(0, top, 240, ui::lcd_light);
    else
        ui::pattern_hline(4, top, 232, 2, 4, ui::amber_frame);
    if (message)
    {
        if (theme == Theme::field)
            ui::footer_message(label_y, message);
        else
            ui::text(font::body, message, 120, label_y, role.text, ui::no_shadow, 1, ui::align::center);
    }
    else
        footer_row(label_y, hints, count);
    if (row2)
        footer_row(top + 14, row2, row2_count);
}

// ---- Panels, boxes and section labels ----
// Panel frame; the unfocused one of two side-by-side panels is drawn with the quieter frame.
void panel(int x, int y, int w, int h, bool focused = true)
{
    switch (theme)
    {
    case Theme::field:
        ui::panel(x, y, w, h, ui::white, false);
        break;
    case Theme::net:
        ui::round_fill(x, y, w, h, 3, focused ? ui::net_cyan : ui::net_chip_hi);
        ui::round_fill(x + 1, y + 1, w - 2, h - 2, 2, ui::net_deep);
        break;
    case Theme::lcd:
        if (focused)
        {
            ui::round_fill(x, y, w, h, 2, ui::lcd_dark);
            ui::fill(x + 2, y + 2, w - 4, h - 4, ui::lcd_screen);
        }
        else
        {
            ui::round_fill(x, y, w, h, 2, ui::lcd_mid);
            ui::round_fill(x + 1, y + 1, w - 2, h - 2, 1, ui::lcd_screen);
        }
        break;
    case Theme::amber:
    default:
        ui::fill(x, y, w, h, ui::amber_bg);
        outline(x, y, w, h, focused ? ui::amber_text : ui::amber_frame);
        break;
    }
}
// Modal box (menu, dialogue, confirmations): AuthBoy's box for the theme.
void box(int x, int y, int w, int h)
{
    switch (theme)
    {
    case Theme::field:
        ui::panel(x, y, w, h, ui::white, true);
        break;
    case Theme::net:
        ui::round_fill(x + 3, y + 3, w, h, 3, ui::net_deep);
        ui::round_fill(x - 1, y - 1, w + 2, h + 2, 4, ui::net_deep);
        ui::round_fill(x, y, w, h, 3, ui::net_cyan);
        ui::round_fill(x + 2, y + 2, w - 4, h - 4, 1, ui::net_chip);
        break;
    case Theme::lcd:
        ui::round_fill(x + 3, y + 3, w, h, 2, ui::lcd_dark);
        ui::round_fill(x, y, w, h, 2, ui::lcd_dark);
        ui::fill(x + 2, y + 2, w - 4, h - 4, ui::lcd_screen);
        break;
    case Theme::amber:
    default:
        ui::fill(x, y, w, h, ui::amber_bg);
        outline(x, y, w, h, ui::amber_text);
        outline(x + 2, y + 2, w - 4, h - 4, ui::amber_text);
        break;
    }
}
void modal_backdrop()
{
    switch (theme)
    {
    case Theme::field:
        ui::checker(0, 24, 240, 136, ui::stripe_a);
        break;
    case Theme::net:
        ui::fade(0, 0, 240, 160, ui::fade_style::net_link);
        break;
    case Theme::lcd:
        ui::checker(0, 0, 240, 160, ui::lcd_screen);
        break;
    case Theme::amber:
    default:
        ui::fade(0, 0, 240, 160, ui::fade_style::phosphor);
        break;
    }
}
// 1 px rule under a section label ("NETWORKS 3/5", above "ADDRESS").
void rule(int x, int y, int w)
{
    switch (theme)
    {
    case Theme::field:
        line(x, y, w, ui::bevel);
        break;
    case Theme::net:
        line(x, y, w, ui::net_grid);
        break;
    case Theme::lcd:
        ui::pattern_hline(x, y, w, 1, 2, ui::lcd_light);
        break;
    case Theme::amber:
    default:
        ui::pattern_hline(x, y, w, 2, 4, ui::amber_frame);
        break;
    }
}
// Tag ("ADDRESS", "NAME", "TYPE"): a box with the label; y is the text top. Returns its width.
int tag(const char* s, int x, int y)
{
    int w = ui::text_width(font::digits, s) + (theme == Theme::net ? 10 : 6);
    switch (theme)
    {
    case Theme::field:
        ui::round_fill(x, y - 2, w, 11, 2, ui::ink);
        ui::text(font::digits, s, x + 3, y, ui::selection);
        break;
    case Theme::net:
        parallelogram(x, y - 2, w, 11, 3, ui::net_cyan);
        ui::text(font::digits, s, x + 5, y, ui::net_deep);
        break;
    case Theme::lcd:
        ui::round_fill(x, y - 2, w, 11, 2, ui::lcd_dark);
        ui::text(font::digits, s, x + 3, y, ui::lcd_screen);
        break;
    case Theme::amber:
    default:
        ui::fill(x, y - 2, w, 11, ui::amber_text);
        ui::text(font::digits, s, x + 3, y, ui::amber_bg);
        break;
    }
    return w;
}
// Info box with a status dot (settings), AuthBoy's info_box at 4,28 232x18.
void info_box(const char* s)
{
    switch (theme)
    {
    case Theme::field:
        ui::panel(4, 28, 232, 18, ui::info, false);
        ui::round_fill(8, 33, 8, 8, 4, ui::green);
        ui::fill(10, 34, 4, 2, ui::green_shine);
        ui::text(font::body, s, 18, 33, ui::ink);
        break;
    case Theme::net:
        ui::round_fill(4, 28, 232, 18, 3, ui::net_cyan);
        ui::round_fill(5, 29, 230, 16, 2, ui::net_deep);
        ui::round_fill(10, 33, 7, 7, 3, ui::net_green);
        ui::text(font::body, s, 22, 33, ui::net_text);
        break;
    case Theme::lcd:
        ui::round_fill(4, 28, 232, 18, 2, ui::lcd_dark);
        ui::round_fill(5, 29, 230, 16, 1, ui::lcd_screen);
        ui::fill(10, 34, 6, 6, ui::lcd_dark);
        ui::text(font::body, s, 21, 33, ui::lcd_dark);
        break;
    case Theme::amber:
    default:
        outline(4, 28, 232, 18, ui::amber_frame);
        ui::fill(10, 34, 6, 6, ui::amber_text);
        ui::text(font::body, s, 21, 33, ui::amber_text);
        break;
    }
}

// ---- Rows ----
enum class Row
{
    normal,
    selected, // in the focused panel
    locked,   // the chosen wallet while the network panel has focus
    dimmed,   // rows of the unfocused panel
};
struct RowStyle
{
    uint8_t text, dim;
    int x; // where the row content starts
};
// AuthBoy's moving cursor: up and down arrows in place of the row cursor.
void move_arrows(int x, int y, int h, uint8_t color)
{
    ui::cursor_up(x, y + h / 2 - 5, color);
    ui::cursor_down(x, y + h / 2 + 2, color);
}
// Draws a list row's background (and cursor). index alternates the Field Guide zebra rows. strip is
// the type strip color (0 for none). moving: the selected row is being moved (custom order).
RowStyle row(int x, int y, int w, int h, Row state, int index, uint8_t strip = 0, bool moving = false)
{
    switch (theme)
    {
    case Theme::field:
    {
        // Wallet rows carry the type as a 3 px strip on the left edge (as Net Link's chips do); the
        // cursor sits right of it.
        int cursor_x = strip ? x + 5 : x + 2, content_x = strip ? x + 13 : x + 10;
        uint8_t text_c = ui::ink;
        if (state == Row::selected)
        {
            ui::round_fill(x, y, w, h, 2, ui::selection);
            outline(x, y, w, h, ui::selection_edge);
            line(x + 1, y + h - 2, w - 2, ui::row_inner_shade);
        }
        else if (state == Row::locked)
        {
            ui::fill(x, y, w, h, ui::zebra);
            outline(x, y, w, h, ui::secondary);
        }
        else
        {
            if (index % 2)
                ui::fill(x, y, w, h, ui::zebra);
            if (state == Row::dimmed)
                text_c = ui::secondary;
        }
        if (strip)
            ui::fill(x, y, 3, h, strip);
        if (state == Row::selected && moving)
            move_arrows(cursor_x, y, h, ui::red);
        else if (state == Row::selected)
            ui::cursor_right(cursor_x, y + (h - 8) / 2, ui::red);
        return { text_c, ui::secondary, content_x };
    }
    case Theme::net:
    {
        // Chips; the selected chip moves selected_shift_x = 3 right for the green cursor.
        int sw = state == Row::selected ? 5 : 4;
        if (state == Row::selected)
        {
            x += 3;
            w -= 3;
            ui::round_fill(x, y, w, h, 2, ui::net_green);
            ui::fill(x + 2, y + 2, w - 4, h - 4, ui::net_selected);
            ui::fill(x + 2 + (strip ? sw : 0), y + 2, w - 4 - (strip ? sw : 0), 1, ui::net_cyan);
            if (moving)
                move_arrows(x - 6, y, h, ui::net_green);
            else
                ui::cursor_right(x - 6, y + (h - 8) / 2, ui::net_green);
        }
        else if (state == Row::locked)
        {
            ui::round_fill(x, y, w, h, 2, ui::net_cyan);
            ui::round_fill(x + 1, y + 1, w - 2, h - 2, 1, ui::net_selected);
        }
        else if (state == Row::dimmed)
            ui::round_fill(x, y, w, h, 2, ui::net_chip_dark);
        else
        {
            ui::round_fill(x, y, w, h, 2, ui::net_chip);
            line(x + 2, y, w - 4, ui::net_chip_hi);
        }
        if (strip)
        {
            ui::fill(x, y + 1, sw, h - 2, strip);
            ui::fill(x + 1, y, sw - 1, h, strip);
        }
        bool quiet = state == Row::dimmed;
        return { quiet ? uint8_t(ui::net_secondary) : uint8_t(ui::net_text),
            state == Row::selected ? uint8_t(ui::net_shine) : uint8_t(ui::net_secondary),
            x + (strip ? sw : 0) + 5 };
    }
    case Theme::lcd:
        if (state == Row::selected)
        {
            // No cursor, except the arrows while moving (the content makes room for them).
            ui::round_fill(x, y, w, h, 2, ui::lcd_dark);
            if (moving)
                move_arrows(x + 2, y, h, ui::lcd_light);
            return { ui::lcd_screen, ui::lcd_light, x + (moving ? 9 : 4) };
        }
        if (state == Row::locked)
            outline(x, y, w, h, ui::lcd_dark);
        return { state == Row::dimmed ? uint8_t(ui::lcd_mid) : uint8_t(ui::lcd_dark), ui::lcd_mid, x + 4 };
    case Theme::amber:
    default:
        if (state == Row::selected)
        {
            ui::fill(x, y, w, h, ui::amber_text);
            if (moving)
                move_arrows(x + 2, y, h, ui::amber_bg);
            else
                ui::text(font::body, ">", x + 2, y + (h > 17 ? 3 : (h - 8) / 2), ui::amber_bg);
            return { ui::amber_bg, ui::amber_bg, x + 8 };
        }
        if (state == Row::locked)
            outline(x, y, w, h, ui::amber_text);
        return { state == Row::dimmed ? uint8_t(ui::amber_dim) : uint8_t(ui::amber_text), ui::amber_dim, x + 3 };
    }
    return { role.text, role.dim, x };
}
// Pocket LCD: dotted separator in the gap below a row, between two unselected rows.
void separator(int x, int y, int w, Row above, Row below)
{
    auto plain = [](Row r) { return r == Row::normal || r == Row::dimmed; };
    if (theme == Theme::lcd && plain(above) && plain(below))
        ui::pattern_hline(x, y, w, 1, 2, ui::lcd_light);
}

// ---- Wallet and network marks ----
// Wallet type mark at x, centered in a row of height h from y. Returns where the name starts.
// chip: Net Link draws the type as the chip's strip (row()), so nothing is drawn here.
int type_mark(WalletType type, int x, int y, int h, uint8_t color, bool /* big */ = false, bool chip = false)
{
    char letter[2] = { "CHEX"[int(type)], 0 }; // eXternal: E is Exchange
    switch (theme)
    {
    case Theme::field:
        // A 3 px type strip: on a list row it is the row's left edge (row()), elsewhere a 3x10 bar.
        if (chip)
            return x;
        ui::fill(x, y + (h - 10) / 2, 3, 10, type_colors[int(type)]);
        return x + 6;
    case Theme::net:
        if (chip)
            return x;
        ui::fill(x, y + (h - 10) / 2, 4, 10, type_colors[int(type)]);
        return x + 8;
    case Theme::lcd:
    {
        int top = y + (h - 9) / 2;
        outline(x, top, 9, 9, color);
        ui::text(font::digits, letter, x + 5, top + 1, color, ui::no_shadow, 1, ui::align::center);
        return x + 13;
    }
    case Theme::amber:
    default:
    {
        char mark[4] = { '[', letter[0], ']', 0 };
        ui::text(font::digits, mark, x, y + (h > 17 ? 3 : (h - 8) / 2), color);
        return x + ui::text_width(font::digits, mark) + 2;
    }
    }
    return x;
}
// Network coin (Field Guide and Net Link only). Returns where the ticker starts.
int coin(int net, int x, int y)
{
    if (theme != Theme::field && theme != Theme::net)
        return x;
    bool net_apt = net == 4 && theme == Theme::net;
    ui::round_fill(x, y, 10, 10, 5, net_apt ? uint8_t(ui::net_text) : coin_colors[net]);
    char label[2] = { tickers[net][0], 0 };
    ui::text(font::digits, label, x + 5, y + 1, net_apt ? ui::net_deep : ui::white, ui::no_shadow, 1,
        ui::align::center);
    return x + 13;
}
// Short form "..18fd": an ellipsis then the last 4 characters in digits, right-aligned at right. A
// full-width ".." would take 14 px, so the ellipsis is two 2x2 dots in 8 px; the whole form is 36 px.
constexpr int short_address_tail = 4;
constexpr int short_address_width = 8 + short_address_tail * 7;
void short_text(const char* address, int right, int y, uint8_t color)
{
    constexpr int tail = short_address_tail;
    int size = int(std::strlen(address));
    if (size <= tail)
    {
        ui::text(font::digits, address, right, y, color, ui::no_shadow, 1, ui::align::right);
        return;
    }
    int x = right - short_address_width;
    ui::fill(x, y + 5, 2, 2, color);
    ui::fill(x + 4, y + 5, 2, 2, color);
    ui::text(font::digits, bn::string_view(address + size - tail, tail), x + 8, y, color);
}
// Short address in a row of height h from y, right-aligned at right. Returns its left edge.
int short_address(const char* address, int right, int y, int h, const RowStyle& s, Row state)
{
    int ty = y + (h - 8) / 2;
    switch (theme)
    {
    case Theme::net:
    {
        // AuthBoy's code slot, widened to the short form.
        constexpr int slot = short_address_width + 6;
        ui::round_fill(right - slot, y + (h - 11) / 2, slot, 11, 2, ui::net_deep);
        short_text(address, right - 3, ty, state == Row::dimmed ? ui::net_secondary : ui::net_cyan);
        return right - slot;
    }
    case Theme::lcd:
        short_text(address, right, ty, state == Row::selected ? ui::lcd_screen : ui::lcd_mid);
        break;
    default:
        short_text(address, right, ty, s.text);
        break;
    }
    return right - short_address_width;
}

// ---- Menus ----
// One menu item; y is the top of its 15 px bar, inside the box at bx with width bw.
void menu_item(int bx, int bw, int y, const char* label, bool selected, bool danger, bool disabled,
    const char* value = nullptr)
{
    switch (theme)
    {
    case Theme::field:
    {
        uint8_t c = disabled ? ui::secondary : danger ? ui::red : ui::ink;
        ui::menu_item(bx, bw, y + 4, label, c, danger ? ui::danger_shadow : ui::light_shadow, selected, 0);
        if (value)
            ui::text(font::body, value, bx + bw - 10, y + 4, ui::ink, ui::light_shadow, 1, ui::align::right);
        break;
    }
    case Theme::net:
    {
        int x = bx + 14, w = bw - 20;
        ui::fill(bx + 2, y, bw - 4, 15, ui::net_chip);
        if (selected)
        {
            ui::round_fill(x, y, w, 15, 2, ui::net_green);
            ui::round_fill(x + 1, y + 1, w - 2, 13, 1, ui::net_selected);
            ui::cursor_right(bx + 6, y + 4, ui::net_green);
        }
        else
            ui::round_fill(x, y, w, 15, 2, ui::net_chip_dark);
        int tx = x + 4;
        if (danger)
        {
            ui::fill(x, y + 1, 3, 13, ui::net_danger);
            ui::fill(x + 1, y, 2, 15, ui::net_danger);
            tx += 3;
        }
        uint8_t c = disabled ? ui::net_secondary : danger ? ui::net_danger_text : ui::net_text;
        ui::text(font::body, label, tx, y + 4, c);
        if (value)
            ui::text(font::body, value, x + w - 4, y + 4, ui::net_cyan, ui::no_shadow, 1, ui::align::right);
        break;
    }
    case Theme::lcd:
    case Theme::amber:
    default:
    {
        // Inverted bar; danger items end with "!" (danger_bang).
        bool lcd = theme == Theme::lcd;
        uint8_t ground = lcd ? ui::lcd_screen : ui::amber_bg, ink = lcd ? ui::lcd_dark : ui::amber_text;
        uint8_t quiet = lcd ? ui::lcd_mid : ui::amber_dim;
        int x = bx + 6, w = bw - 12;
        ui::fill(x, y, w, 15, ground);
        if (selected)
            ui::round_fill(x, y, w, 15, lcd ? 1 : 0, ink);
        uint8_t c = selected ? ground : disabled ? quiet : ink;
        ui::text(font::body, label, x + 4, y + 4, c);
        if (value)
            ui::text(font::body, value, x + w - 4, y + 4, c, ui::no_shadow, 1, ui::align::right);
        else if (danger)
            ui::text(font::body, "!", x + w - 4, y + 4, c, ui::no_shadow, 1, ui::align::right);
        break;
    }
    }
}

// ---- Keyboards ----
void input_box(int x, int y, int w, int h)
{
    switch (theme)
    {
    case Theme::field:
        ui::panel(x, y, w, h, ui::white, false);
        break;
    case Theme::net:
        ui::round_fill(x, y, w, h, 3, ui::net_cyan);
        ui::round_fill(x + 1, y + 1, w - 2, h - 2, 2, ui::net_deep);
        break;
    case Theme::lcd:
        ui::round_fill(x, y, w, h, 2, ui::lcd_dark);
        ui::fill(x + 2, y + 2, w - 4, h - 4, ui::lcd_screen);
        break;
    case Theme::amber:
    default:
        ui::fill(x, y, w, h, ui::amber_bg);
        outline(x, y, w, h, ui::amber_frame);
        break;
    }
}
void key_panel(int x, int y, int w, int h)
{
    switch (theme)
    {
    case Theme::field:
        ui::panel(x, y, w, h, ui::info, false);
        break;
    case Theme::net:
        ui::round_fill(x, y, w, h, 3, ui::net_cyan);
        ui::round_fill(x + 1, y + 1, w - 2, h - 2, 2, ui::net_chip);
        break;
    case Theme::lcd:
        ui::fill(x, y, w, h, ui::lcd_screen);
        break;
    case Theme::amber:
    default:
        ui::fill(x, y, w, h, ui::amber_bg);
        outline(x, y, w, h, ui::amber_frame);
        break;
    }
}
// Key cap (13 px tall); the space key shows a bar instead of a glyph.
void key(int x, int y, int w, int h, char label, bool selected)
{
    uint8_t c = 0;
    switch (theme)
    {
    case Theme::field:
        ui::key_cap(x, y, w, h, label, selected, ui::info);
        c = ui::ink;
        break;
    case Theme::net:
        ui::fill(x - 2, y - 2, w + 4, h + 4, ui::net_chip);
        if (selected)
        {
            ui::round_fill(x, y, w, h, 2, ui::net_text);
            ui::round_fill(x + 1, y + 1, w - 2, h - 2, 1, ui::net_green);
        }
        else
        {
            ui::round_fill(x, y, w, h, 2, ui::net_chip_dark);
            line(x + 2, y, w - 4, ui::net_chip_hi);
        }
        c = selected ? ui::net_deep : ui::net_text;
        break;
    case Theme::lcd:
        ui::fill(x - 2, y - 2, w + 4, h + 4, ui::lcd_screen);
        if (selected)
        {
            ui::round_fill(x - 2, y - 2, w + 4, h + 4, 3, ui::lcd_dark);
            ui::round_fill(x - 1, y - 1, w + 2, h + 2, 2, ui::lcd_screen);
        }
        ui::round_fill(x, y, w, h, 2, selected ? ui::lcd_dark : ui::lcd_light);
        c = selected ? ui::lcd_screen : ui::lcd_dark;
        break;
    case Theme::amber:
    default:
        // No cap graphics: the selected key's cell fills amber.
        ui::fill(x - 2, y - 2, w + 4, h + 4, ui::amber_bg);
        if (selected)
            ui::fill(x, y, w, h, ui::amber_text);
        c = selected ? ui::amber_bg : ui::amber_text;
        break;
    }
    if (label == ' ')
        line(x + 5, y + 9, w - 10, c);
    else if (theme != Theme::field)
    {
        char s[2] = { label, 0 };
        ui::text(font::digits, s, x + w / 2, y + (h - 7) / 2, c, ui::no_shadow, 1, ui::align::center);
    }
}
// ABC / abc case tabs; x is the left edge of the pair, y their top (they sit on the key panel top).
void case_tabs(int x, int y, bool lowercase)
{
    for (int i = 0; i < 2; ++i)
    {
        bool active = (i == 1) == lowercase;
        const char* label = i ? "abc" : "ABC";
        switch (theme)
        {
        case Theme::field:
        {
            int tx = x + i * 27;
            if (active)
            {
                ui::round_fill(tx, y, 26, 12, 3, ui::ink);
                ui::fill(tx + 2, y + 2, 22, 10, ui::info);
            }
            else
                ui::round_fill(tx + 1, y + 2, 24, 9, 2, ui::tab_inactive);
            ui::text(font::digits, label, tx + 13, y + 3, active ? ui::ink : ui::tab_inactive_text, ui::no_shadow, 1,
                ui::align::center);
            break;
        }
        case Theme::net:
        {
            int tx = x + i * 34;
            parallelogram(tx, y, 32, 10, 3, active ? ui::net_green : ui::net_chip_dark);
            ui::text(font::digits, label, tx + 16, y + 2, active ? ui::net_deep : ui::net_secondary, ui::no_shadow, 1,
                ui::align::center);
            break;
        }
        case Theme::lcd:
        {
            int tx = x + i * 30;
            if (active)
            {
                ui::round_fill(tx, y, 28, 10, 2, ui::lcd_dark);
                ui::fill(tx, y + 5, 28, 5, ui::lcd_dark);
            }
            ui::text(font::digits, label, tx + 14, y + 2, active ? ui::lcd_screen : ui::lcd_mid, ui::no_shadow, 1,
                ui::align::center);
            break;
        }
        case Theme::amber:
        default:
        {
            int tx = x + i * 30;
            if (active)
                ui::fill(tx, y, 26, 10, ui::amber_text);
            ui::text(font::digits, label, tx + 13, y + 2, active ? ui::amber_bg : ui::amber_dim, ui::no_shadow, 1,
                ui::align::center);
            break;
        }
        }
    }
}
int case_tabs_width()
{
    return theme == Theme::field ? 53 : theme == Theme::net ? 66 : theme == Theme::lcd ? 58 : 56;
}

// ---- Header filter pill ----
void filter(const Controller& app)
{
    char label[20];
    std::snprintf(label, sizeof(label), "%s %d", filter_names[app.book.filter], app.visible_count());
    int lw = ui::text_width(font::digits, "L");
    if (theme == Theme::field)
    {
        int swatch = app.book.filter ? 8 : 0;
        int chip = ui::text_width(font::digits, label) + 7 + swatch;
        int side = lw + 9; // letter, gap, 3 px arrow, gap
        int left = 235 - chip - 2 * side - 4;
        ui::round_fill(left, 4, chip + 2 * side + 4, 15, 6, ui::header_shadow);
        ui::text(font::digits, "L", left + 4, 8, ui::white);
        ui::text(font::body6, "\x01", left + side - 3, 9, ui::white);
        int chip_x = left + side + 2;
        ui::round_fill(chip_x, 6, chip, 11, 3, ui::white);
        int tx = chip_x + 4;
        if (swatch)
        {
            ui::fill(tx, 8, 6, 6, type_colors[app.book.filter - 1]);
            tx += swatch;
        }
        ui::text(font::digits, label, tx, 8, ui::ink);
        int right = chip_x + chip + 2;
        ui::text(font::body6, "\x02", right, 9, ui::white);
        ui::text(font::digits, "R", right + 6, 8, ui::white);
        return;
    }
    if (theme == Theme::amber)
    {
        // No band: L < [COLD 3] > R, the chip inverted.
        bn::string<24> chip_text = "[";
        chip_text += label;
        chip_text += ']';
        int chip = ui::text_width(font::digits, chip_text) + 4;
        int total = 4 * (lw + 3) + chip;
        int x = 235 - total;
        ui::text(font::digits, "L", x, 7, ui::amber_dim);
        ui::text(font::digits, "<", x + lw + 3, 7, ui::amber_dim);
        x += 2 * (lw + 3);
        ui::fill(x - 1, 5, chip, 11, ui::amber_text);
        ui::text(font::digits, chip_text, x + 1, 7, ui::amber_bg);
        x += chip + 3;
        ui::text(font::digits, ">", x, 7, ui::amber_dim);
        ui::text(font::digits, "R", x + lw + 3, 7, ui::amber_dim);
        return;
    }
    // Net Link: cyan parallelogram chip with the type swatch. Pocket LCD: light chip, no swatch.
    bool net = theme == Theme::net;
    int swatch = net && app.book.filter ? 8 : 0;
    int chip = ui::text_width(font::digits, label) + (net ? 10 : 6) + swatch;
    int side = lw + 9;
    int left = 235 - chip - 2 * side;
    uint8_t quiet = net ? ui::net_secondary : ui::lcd_light;
    ui::text(font::digits, "L", left, 7, quiet);
    ui::text(font::body6, "\x01", left + lw + 3, 8, quiet);
    int chip_x = left + side;
    if (net)
        parallelogram(chip_x, 5, chip, 11, 3, ui::net_cyan);
    else
        ui::round_fill(chip_x, 5, chip, 11, 2, ui::lcd_screen);
    int tx = chip_x + (net ? 5 : 3);
    if (swatch)
    {
        ui::fill(tx, 7, 6, 6, type_colors[app.book.filter - 1]);
        tx += swatch;
    }
    ui::text(font::digits, label, tx, 7, net ? ui::net_deep : ui::lcd_dark);
    int right = chip_x + chip + 3;
    ui::text(font::body6, "\x02", right, 8, quiet);
    ui::text(font::digits, "R", right + 6, 7, quiet);
}

// ---- Screens ----
void scrollbar(int count, int selected)
{
    constexpr int x = 126, top = 30, height = 107;
    int h = height * 6 / count;
    if (h < 5)
        h = 5;
    int y = top + (height - h) * selected / (count - 1);
    switch (theme)
    {
    case Theme::field:
        ui::fill(x, top, 3, height, ui::zebra);
        ui::fill(x, y, 3, h, ui::secondary);
        break;
    case Theme::net:
        ui::round_fill(x, top, 3, height, 1, ui::net_chip_dark);
        ui::round_fill(x, y, 3, h, 1, ui::net_cyan);
        break;
    case Theme::lcd:
        for (int i = 0; i < height; i += 2)
            ui::fill(x, top + i, 3, 1, ui::lcd_light);
        ui::fill(x, y, 3, h, ui::lcd_mid);
        break;
    case Theme::amber:
    default:
        ui::fill(x, top, 3, height, ui::amber_track);
        ui::fill(x, y, 3, h, ui::amber_text);
        break;
    }
}
void home(const Controller& app, bool right_active)
{
    header("AddrBoy");
    filter(app);
    // Left: wallets, 4..132 (128 px). Right: networks of the highlighted wallet, 136..236 (100 px);
    // its rows only need a coin, the ticker and the "..18fd" short form.
    constexpr int left_x = 4, left_w = 128, right_x = 136, right_w = 100;
    constexpr int row_x = left_x + 4, row_w = left_w - 12;               // 8..124, scrollbar at 126
    constexpr int net_x = right_x + 4, net_w = right_w - 8;              // 140..232
    panel(left_x, 26, left_w, 114, !right_active);
    panel(right_x, 26, right_w, 114, right_active);
    int count = app.visible_count();
    if (!count)
    {
        const char* empty[] = { "No wallets yet", "No cold wallets", "No hot wallets", "No exchanges", "No external" };
        text(empty[app.book.filter], left_x + 6, 66, role.dim, left_w - 12);
        text("STA to add", left_x + 6, 83, role.dim, left_w - 12);
    }
    else
    {
        // Wallets (left panel).
        // All rows are single-line (the network count shows as "NETWORKS n/5"): six 17 px rows with
        // 1 px gaps fill y 30-136, like the setup list. Text sits at y + 4, centered in the row.
        constexpr int visible = 6, row_height = 17;
        int first = app.selected < visible ? 0 : app.selected - (visible - 1);
        int y = 30;
        Row previous = Row::selected;
        for (int pos = first; pos < count && pos < first + visible; ++pos)
        {
            const auto& wallet = app.book.wallets[app.visible_index(pos)];
            bool active = pos == app.selected;
            Row state = active ? (right_active ? Row::locked : Row::selected)
                : right_active ? Row::dimmed
                               : Row::normal;
            separator(row_x, y - 1, row_w, previous, state);
            RowStyle s = row(row_x, y, row_w, row_height, state, pos - first, type_colors[int(wallet.type)],
                active && app.moving);
            int name_x = type_mark(wallet.type, s.x, y, row_height, s.text, false, true);
            text(wallet.name, name_x, y + (row_height - 8) / 2, s.text, row_x + row_w - name_x);
            previous = state;
            y += row_height + 1;
        }
        if (count > 6)
            scrollbar(count, app.selected);

        // Networks of the highlighted wallet (right panel).
        const auto& wallet = app.book.wallets[app.wallet_index()];
        // Label at 32, rule at 43, rows from 48: 4 px around each. Five 17 px rows (18 px pitch) then
        // end at y 136, level with the wallet list.
        ui::text(font::digits, "NETWORKS", right_x + 7, 32, role.dim);
        rule(right_x + 6, 43, right_w - 12);
        if (!network_total(wallet))
        {
            text("No addresses yet", right_x + 8, 66, role.dim, right_w - 14);
            text("STA > Networks", right_x + 8, 83, role.dim, right_w - 14);
        }
        int index = 0;
        previous = Row::selected;
        for (int n = 0; n < 5; ++n)
            if (wallet.addresses[n][0])
            {
                constexpr int net_row = 17;
                y = 48 + index * (net_row + 1);
                Row state = !right_active ? Row::dimmed : app.network == n ? Row::selected : Row::normal;
                separator(net_x, y - 1, net_w, previous, state);
                RowStyle s = row(net_x, y, net_w, net_row, state, index);
                int tx = coin(n, s.x, y + (net_row - 10) / 2);
                ui::text(font::body, tickers[n], tx, y + (net_row - 8) / 2, s.text);
                int left = short_address(wallet.addresses[n], net_x + net_w - 2, y, net_row, s, state);
                if (theme == Theme::amber)
                    leader(tx + ui::text_width(font::body, tickers[n]) + 4, left - 4, y + net_row - 6,
                        state == Row::selected ? ui::amber_bg : ui::amber_frame);
                previous = state;
                ++index;
            }
    }
    if (right_active)
    {
        const ui::hint hints[] = { { "A", "show QR" }, { "B", "wallets" }, { "STA", "menu" } };
        footer(hints, 3, app.message);
    }
    else
    {
        // As AuthBoy's home: SEL opens settings (it also works from the networks panel, whose
        // footer has no room for a fourth hint). While moving: AuthBoy's move hints.
        const ui::hint hints[] = { { "A", "select" }, { "STA", "menu" }, { "SEL", "settings" } };
        const ui::hint move_hints[] = { { ui::dpad_up_down, "move" }, { "A", "done" } };
        if (app.moving)
            footer(move_hints, 2, app.message);
        else
            footer(hints, 3, app.message);
    }
}
uint8_t qr[qrcodegen_BUFFER_LEN_FOR_VERSION(4)];
char qr_address[address_capacity] = { };
bool qr_valid = false;
void receive(const Controller& app)
{
    header("AddrBoy");
    header_counter("RECEIVE");
    // Left: 4,25 112x116. Right (QR): 120,25 116x116, so its full theme frame (2 px in Field Guide
    // and Pocket LCD) surrounds a 112x112 white area at 122,27.
    panel(4, 25, 112, 116);
    panel(120, 25, 116, 116);
    const auto& wallet = app.book.wallets[app.wallet_index()];
    const char* address = wallet.addresses[app.network];
    int name_x = type_mark(wallet.type, 10, 31, 12, role.text);
    text(wallet.name, name_x, 34, role.text, 112 - name_x);
    int tx = coin(app.network, 10, 47);
    ui::text(font::body, tickers[app.network], tx + 1, 49, role.text);
    // What the address is, after the ticker (all fit the 62 px left there); BTC names its format.
    constexpr const char* kinds[] = { "", "ETH + L2s", "Solana", "Sui", "Aptos" };
    text(app.network == 0 ? bitcoin_kind(address) : kinds[app.network], 49, 49, role.dim, 62);
    rule(10, 60, 102);
    tag("ADDRESS", 10, 65); // box 63-73: 2 px from the rule and from the address
    int len = int(std::strlen(address));
    for (int start = 0, line_index = 0; start < len; start += 12, ++line_index)
    {
        char line_text[16] = { };
        int dest = 0;
        for (int j = 0; j < 12 && start + j < len; ++j)
        {
            if (j && j % 4 == 0)
                line_text[dest++] = ' ';
            line_text[dest++] = address[start + j];
        }
        // 14 cells x 7 px = 98 px per line; 9 px pitch so six lines (66 characters) end at y 129.
        ui::text(font::digits, line_text, 10, 76 + line_index * 9, role.text);
    }
    // Network position dots, centered in the left panel (4..116) below the address.
    uint8_t current = 0, other = 0;
    switch (theme)
    {
    case Theme::field:
        current = ui::red;
        other = ui::bevel;
        break;
    case Theme::net:
        current = ui::net_cyan;
        other = ui::net_grid;
        break;
    case Theme::lcd:
        current = ui::lcd_dark;
        other = ui::lcd_light;
        break;
    case Theme::amber:
    default:
        current = ui::amber_text;
        other = ui::amber_track;
        break;
    }
    int dots = network_total(wallet), pos = 0;
    int dots_x = 60 - (dots * 6 - 2) / 2;
    for (int n = 0; n < 5; ++n)
        if (wallet.addresses[n][0])
        {
            ui::round_fill(dots_x + pos * 6, 131, 4, 4, 2, n == app.network ? current : other);
            ++pos;
        }
    if (std::strcmp(qr_address, address))
    {
        std::strcpy(qr_address, address);
        uint8_t data[qrcodegen_BUFFER_LEN_FOR_VERSION(4)] = { };
        std::memcpy(data, address, len);
        qr_valid = qrcodegen_encodeBinary(data, len, qr, qrcodegen_Ecc_LOW, 3, 4, qrcodegen_Mask_AUTO, false);
    }
    // The theme's panel frame; inside it always pure black on white, 112x112: version 3 with the
    // four-module quiet zone fits at 3 px (111 px), and version 4 uses 2 px modules so its full quiet
    // zone also fits.
    constexpr int qr_x = 122, qr_y = 27, qr_size = 112;
    ui::fill(qr_x, qr_y, qr_size, qr_size, ui::qr_white);
    if (qr_valid)
    {
        int modules = qrcodegen_getSize(qr) + 8;
        int scale = qr_size / modules;
        if (scale > 3)
            scale = 3;
        ui::qr_code(qr, qr_x + (qr_size - modules * scale) / 2, qr_y, qr_size, 3, false);
    }
    else
        text("QR unavailable", qr_x + 7, 80, ui::qr_dark, qr_size - 12);
    const ui::hint hints[] = { { ui::dpad_left_right, "network" }, { "B", "back" }, { "STA", "menu" } };
    footer(hints, 3, app.message);
}
void menu(const Controller& app)
{
    if (app.return_screen == Screen::receive)
        receive(app);
    else
        home(app, app.return_screen == Screen::networks);
    modal_backdrop();
    box(4, 108, 128, 50);
    // Command box: four items, five with Move in the custom order (Settings is on SELECT),
    // bottom-aligned where the handoff's box ends.
    int items = app.menu_count(), box_h = 12 + items * 16, box_y = 158 - box_h;
    box(134, box_y, 102, box_h);
    int index = app.wallet_index();
    if (index >= 0)
    {
        const auto& wallet = app.book.wallets[index];
        text(wallet.name, 12, 117, role.text, 112);
        char subtitle[30];
        std::snprintf(subtitle, sizeof(subtitle), "%s / %d networks", type_names[int(wallet.type)],
            network_total(wallet));
        text(subtitle, 12, 130, role.dim, 112);
    }
    else
        text("Your address book", 12, 119, role.dim, 112);
    constexpr const char* labels[] = { "New wallet", "Edit wallet", "Networks", "Move", "Delete" };
    for (int i = 0; i < items; ++i)
    {
        MenuItem item = app.menu_item(i);
        menu_item(134, 102, box_y + 5 + i * 16, labels[int(item)], i == app.menu, item == MenuItem::remove,
            index < 0 && item != MenuItem::new_wallet);
    }
    if (app.message)
        text(app.message, 12, 143, role.danger, 112);
}
void setup(const Controller& app)
{
    header(app.edit_index == app.book.count ? "New wallet" : "Edit wallet");
    step_pips(3);
    panel(4, 27, 232, 15);
    int name_x = type_mark(app.draft.type, 9, 27, 15, role.text);
    text(app.draft.name, name_x, 31, role.text, 140 - name_x);
    text("Add networks", 145, 31, role.dim, 86);
    panel(4, 44, 232, 96);
    Row previous = Row::selected;
    for (int n = 0; n < 5; ++n)
    {
        int y = 48 + n * 18;
        Row state = n == app.setup_network ? Row::selected : Row::normal;
        bool selected = state == Row::selected;
        separator(8, y - 1, 224, previous, state);
        RowStyle s = row(8, y, 224, 17, state, n);
        int tx = coin(n, s.x, y + 3);
        ui::text(font::body, tickers[n], tx, y + 4, s.text);
        ui::text(font::body, network_names[n], 65, y + 4, selected ? s.text : s.dim);
        int left;
        if (app.draft.addresses[n][0])
        {
            left = short_address(app.draft.addresses[n], 216, y, 17, s, state);
            // Status: set.
            switch (theme)
            {
            case Theme::field:
                ui::round_fill(222, y + 5, 6, 6, 3, ui::green);
                break;
            case Theme::net:
                ui::round_fill(222, y + 5, 6, 6, 3, ui::net_green);
                break;
            case Theme::lcd:
                ui::fill(222, y + 5, 6, 6, selected ? ui::lcd_screen : ui::lcd_dark);
                break;
            case Theme::amber:
            default:
                ui::fill(222, y + 5, 6, 6, selected ? ui::amber_bg : ui::amber_text);
                break;
            }
        }
        else
        {
            // Status: "+ add" on the selected row, else empty.
            const char* label = selected ? "+ add" : theme == Theme::amber ? "--" : "empty";
            uint8_t c = 0;
            switch (theme)
            {
            case Theme::field:
                c = selected ? ui::ink : ui::secondary;
                break;
            case Theme::net:
                c = selected ? ui::net_green : ui::net_secondary;
                break;
            case Theme::lcd:
                c = selected ? ui::lcd_screen : ui::lcd_mid;
                break;
            case Theme::amber:
            default:
                c = selected ? ui::amber_bg : ui::amber_frame;
                break;
            }
            left = 228 - ui::text_width(font::body, label);
            ui::text(font::body, label, left, y + 4, c);
        }
        if (theme == Theme::amber)
            leader(65 + ui::text_width(font::body, network_names[n]) + 4, left - 4, y + 11,
                selected ? ui::amber_bg : ui::amber_frame);
        previous = state;
    }
    const ui::hint hints[] = { { "A", "add / edit" }, { "B", "back" }, { "STA", "save" } };
    footer(hints, 3, app.message);
}
// Type card (06), 232x31 at y.
// Type card (06), 232x24 at y: name at y+3, description at y+13. Four cards on a 26 px pitch fill
// y 40-141 (the handoff's three 31 px cards had no room for External).
constexpr int card_h = 24;
void type_card(int i, int y, bool selected)
{
    constexpr const char* descriptions[] = { "Hardware or paper / offline", "Phone or browser app",
        "Exchange deposit address", "Someone else's wallet" };
    WalletType type = WalletType(i);
    uint8_t name_c = 0, desc_c = 0;
    int x = 0;
    switch (theme)
    {
    case Theme::field:
        // Type strip on the card's left edge, inside the frame; the cursor sits right of it.
        panel(4, y, 232, card_h);
        if (selected)
        {
            ui::round_fill(6, y + 2, 228, card_h - 4, 2, ui::selection);
            outline(6, y + 2, 228, card_h - 4, ui::selection_edge);
        }
        ui::fill(6, y + 2, 3, card_h - 4, type_colors[i]);
        if (selected)
            ui::cursor_right(11, y + (card_h - 8) / 2, ui::red);
        name_c = ui::ink;
        desc_c = selected ? ui::ink : ui::secondary;
        x = 19;
        break;
    case Theme::net:
    {
        // Chip with a 5 px type strip; the selected chip shifts right for the cursor.
        int cx = selected ? 10 : 4, cw = selected ? 226 : 232;
        if (selected)
        {
            ui::round_fill(cx, y, cw, card_h, 2, ui::net_green);
            ui::fill(cx + 2, y + 2, cw - 4, card_h - 4, ui::net_selected);
            ui::cursor_right(4, y + (card_h - 8) / 2, ui::net_green);
        }
        else
        {
            ui::round_fill(cx, y, cw, card_h, 2, ui::net_chip);
            line(cx + 2, y, cw - 4, ui::net_chip_hi);
        }
        ui::fill(cx, y + 1, 5, card_h - 2, type_colors[i]);
        ui::fill(cx + 1, y, 4, card_h, type_colors[i]);
        name_c = ui::net_text;
        desc_c = selected ? ui::net_shine : ui::net_secondary;
        x = cx + 14;
        break;
    }
    case Theme::lcd:
        ui::round_fill(4, y, 232, card_h, 2, ui::lcd_dark);
        if (!selected)
            ui::fill(6, y + 2, 228, card_h - 4, ui::lcd_screen);
        name_c = selected ? ui::lcd_screen : ui::lcd_dark;
        desc_c = selected ? ui::lcd_light : ui::lcd_mid;
        x = type_mark(type, 14, y, 14, name_c);
        break;
    case Theme::amber:
    default:
        if (selected)
            ui::fill(4, y, 232, card_h, ui::amber_text);
        else
            outline(4, y, 232, card_h, ui::amber_frame);
        name_c = selected ? ui::amber_bg : ui::amber_text;
        desc_c = selected ? ui::amber_bg : ui::amber_dim;
        x = type_mark(type, 14, y, 14, name_c);
        break;
    }
    text(card_names[i], x, y + 3, name_c, 230 - x);
    text(descriptions[i], theme == Theme::lcd || theme == Theme::amber ? 14 : x, y + 13, desc_c, 220);
}
void type(const Controller& app)
{
    header(app.edit_index == app.book.count ? "New wallet" : "Edit wallet");
    step_pips(2);
    int w = tag("TYPE", 7, 29);
    text(app.draft.name, 7 + w + 5, 29, role.text, 222 - w);
    for (int i = 0; i < wallet_type_count; ++i)
        type_card(i, 40 + i * (card_h + 2), int(app.draft.type) == i);
    const ui::hint hints[] = { { "A", "choose" }, { "B", "back" } };
    footer(hints, 2);
}
void keyboard_key(const Controller& app, int index)
{
    // Name: AuthBoy's name keyboard grid (Field Guide's sits 2 px further in, inside its panel border).
    // Address: 9x4 caps of 23x13 on a 25x15 pitch below the three-line input.
    bool name = app.screen == Screen::name;
    int cols = name ? 10 : 9, pitch = name ? 21 : 25, width = name ? 19 : 23;
    int left = name ? (theme == Theme::field ? 15 : 13) : 8, top = name ? (theme == Theme::field ? 68 : 66) : 79;
    key(left + (index % cols) * pitch, top + (index / cols) * 15, width, 13, app.keyboard_char(index),
        index == app.key);
}
// Name being typed: AuthBoy's name_input for the theme, in its 232x22 box at 4,28.
void name_input(const char* input)
{
    constexpr int bx = 4, by = 28, bw = 232, bh = 22;
    int length = int(std::strlen(input));
    switch (theme)
    {
    case Theme::field:
    {
        int scale = ui::text_width(font::body, input, 2) + 12 <= bw - 12 ? 2 : 1;
        int y = by + (bh - 8 * scale) / 2;
        ui::text(font::body, input, bx + 6, y, ui::ink, ui::input_shadow, scale);
        int x = bx + 6 + ui::text_width(font::body, input, scale) + (length ? 1 + scale : 0);
        ui::fill(x, y + 1, 2, 8 * scale - 2, ui::red);
        break;
    }
    case Theme::net:
    {
        int scale = ui::text_width(font::body, input, 2) + 12 + 8 <= bw - 12 ? 2 : 1;
        int x = bx + 6, y = by + (bh - 7 * scale) / 2;
        ui::text(font::body, input, x, y, ui::net_text, ui::no_shadow, scale);
        x += ui::text_width(font::body, input, scale) + (length ? 1 + scale : 0);
        ui::fill(x, by + (bh - 14) / 2, 7, 14, ui::net_green);
        break;
    }
    case Theme::lcd:
    {
        constexpr int y = by + 5;
        ui::pattern_hline(bx + 6, by + 14, 128, 7, 8, ui::lcd_light);
        ui::text(font::body, input, bx + 6, y, ui::lcd_dark);
        ui::fill(bx + 6 + ui::text_width(font::body, input) + (length ? 1 : 0), y, 8, 8, ui::lcd_dark);
        break;
    }
    case Theme::amber:
    default:
    {
        constexpr int y = by + 6;
        ui::pattern_hline(bx + 21, by + 16, 128, 6, 8, ui::amber_frame);
        ui::text(font::body, "> ", bx + 5, y, ui::amber_text);
        int x = bx + 5 + ui::text_width(font::body, "> ") + 1;
        ui::text(font::body, input, x, y, ui::amber_text);
        ui::fill(x + ui::text_width(font::body, input) + (length ? 1 : 0), y, 8, 8, ui::amber_text);
        break;
    }
    }
}
void keyboard(const Controller& app)
{
    bool name = app.screen == Screen::name;
    int length = int(std::strlen(app.input));
    char counter[24];
    std::snprintf(
        counter, sizeof(counter), "%d/%d", length, name ? name_limit : address_limit(Network(app.setup_network)));
    char canonical[address_capacity];
    bool invalid = app.message
        || (!name && length && validate_address(Network(app.setup_network), app.input, canonical));
    if (name)
    {
        // AuthBoy's name keyboard: counter in the header (left of the step pips), input box, a
        // "SEL case" pill with the case tabs on the right, the key panel, and a two-row footer.
        header(app.edit_index == app.book.count ? "New wallet" : "Edit wallet");
        step_pips(1);
        header_counter(counter, invalid, 184);
        input_box(4, 28, 232, 22);
        name_input(app.input);
        if (theme == Theme::field)
        {
            int w = ui::pill(8, 54, "SEL", ui::white, ui::ink);
            ui::text(font::body, "case", 8 + w + 3, 55, ui::ink);
        }
        else
        {
            int w = skin_pill(8, 55, "SEL");
            ui::text(font::body, "case", 8 + w + 3, 55, role.text);
        }
        key_panel(4, 63, 232, 67);
        case_tabs(236 - case_tabs_width(), 53, app.lowercase);
        for (int i = 0; i < 40; ++i)
            keyboard_key(app, i);
        const ui::hint hints[] = { { "A", "type" }, { "B", "delete" }, { "STA", "next" } };
        const ui::hint row2[] = { { "R", "space" }, { "L", "cancel" } };
        footer(hints, 3, app.message, 134, row2, 2);
        return;
    }
    else
    {
        char title[20];
        std::snprintf(title, sizeof(title), "%s address", tickers[app.setup_network]);
        header(title);
        // As on AuthBoy's keyboards, the counter sits in the header.
        header_counter(counter, invalid);
        // Three lines of 30 digits cells, so all 66 characters remain visible.
        constexpr int per_line = 30, text_x = 10, text_y = 28;
        input_box(4, 25, 232, 32);
        for (int row = 0; row * per_line < length; ++row)
        {
            int start = row * per_line, amount = length - start < per_line ? length - start : per_line;
            ui::text(font::digits, bn::string_view(app.input + start, amount), text_x, text_y + row * 9, role.text);
        }
        int row = length / per_line, col = length % per_line;
        if (row < 3)
            ui::fill(text_x + col * 7 - 1, text_y + row * 9, 2, 8, role.accent);
        constexpr const char* hints[] = { "BTC MAINNET / CHECKSUM", "0x + 40 HEX / EIP-55", "BASE58 / 32-44",
            "0x + 64 HEX", "0x + 1-64 HEX" };
        text(hints[app.setup_network], 7, 64, role.dim, 165, font::digits);
    }
    // Tabs right-aligned like AuthBoy's, beside the format hint.
    key_panel(4, 76, 232, 64);
    case_tabs(236 - case_tabs_width(), 66, app.lowercase);
    for (int i = 0; i < 36; ++i)
        keyboard_key(app, i);
    if (app.message)
    {
        box(4, 108, 232, 32);
        text(app.message, 12, 116, role.danger, 216);
        text("Correct the address, then STA", 12, 128, role.dim, 216);
    }
    const ui::hint hints[] = { { "A", "type" }, { "B", "del" }, { "SEL", "case" }, { "STA", "done" } };
    footer(hints, 4);
}
void confirmation(const Controller& app, const char* title, const char* detail, const char* action)
{
    header("AddrBoy");
    box(8, 45, 224, 86);
    text(title, 18, 57, role.danger, 204);
    text(detail, 18, 78, role.text, 204);
    const ui::hint hints[] = { { "A", action }, { "B", "cancel" } };
    footer(hints, 2, app.message);
}
// ---- Welcome (AuthBoy's title screen, with the "Book Boy" logo of addrboy_handoff_logo) ----
constexpr int welcome_x = 4, welcome_y = 100, welcome_w = 232, welcome_h = 56;
// The 32x32 logo, drawn into the canvas: its palette indices 1-6 (outline, spine rings, body, body
// shade, screen, page tab) take the theme's colors from the logo handoff's table.
void logo32(int x, int y)
{
    uint8_t colors[7] = { };
    switch (theme)
    {
    case Theme::field:
        colors[1] = ui::ink, colors[2] = ui::bevel, colors[3] = ui::red, colors[4] = ui::header_shadow,
        colors[5] = ui::info, colors[6] = ui::selection;
        break;
    case Theme::net:
        colors[1] = ui::net_deep, colors[2] = ui::net_secondary, colors[3] = ui::net_chip,
        colors[4] = ui::net_chip_dark, colors[5] = ui::net_cyan, colors[6] = ui::net_green;
        break;
    case Theme::lcd:
        colors[1] = ui::lcd_dark, colors[2] = ui::lcd_light, colors[3] = ui::lcd_mid, colors[4] = ui::lcd_dark,
        colors[5] = ui::lcd_light, colors[6] = ui::lcd_screen;
        break;
    case Theme::amber:
    default:
        colors[1] = ui::amber_track, colors[2] = ui::amber_dim, colors[3] = ui::amber_frame,
        colors[4] = ui::amber_track, colors[5] = ui::amber_text, colors[6] = ui::amber_text;
        break;
    }
    for (int t = 0; t < 16; ++t)
        for (int row = 0; row < 8; ++row)
        {
            uint32_t bits = adb::logo::tiles_32[t].data[row];
            for (int column = 0; column < 8; ++column)
                if (int index = int((bits >> (column * 4)) & 15))
                    ui::fill(x + (t % 4) * 8 + column, y + (t / 4) * 8 + row, 1, 1, colors[index]);
        }
}
// Icon, 8 px gap, then the title, centered as a group at y 28 (Pocket LCD and Phosphor).
void logo_title_row(const char* title, int scale, uint8_t color)
{
    int title_w = ui::text_width(font::body, title, scale);
    int x = (240 - (32 + 8 + title_w)) / 2;
    logo32(x, 28);
    ui::text(font::body, title, x + 40, 28 + (32 - 7 * scale) / 2, color, ui::no_shadow, scale);
}
int welcome_border()
{
    return theme == Theme::field ? 0 : theme == Theme::amber ? 3 : 2;
}
// "(A) continue" with the blinking down arrow, bottom right in the box.
void welcome_continue(bool arrow)
{
    constexpr const char* label = "continue";
    if (theme == Theme::field)
    {
        int right = welcome_x + welcome_w - 8, y = welcome_y + 38;
        int x = right - 5 - 4 - ui::text_width(font::body, label) - 3 - 10;
        ui::round_button(x, y - 1, "A", ui::ink, ui::white);
        ui::text(font::body, label, x + 13, y, ui::ink, ui::light_shadow);
        ui::fill(right - 5, y + 2, 5, 3, ui::white);
        if (arrow)
            ui::cursor_down(right - 5, y + 2, ui::red);
        return;
    }
    int border = welcome_border();
    int right = welcome_x + welcome_w - border - 8, label_y = welcome_y + welcome_h - border - 12;
    int x = right - (skin_pill_width("A") + 4 + ui::text_width(font::body, label) + 6 + 7);
    int w = skin_pill(x, label_y, "A");
    ui::text(font::body, label, x + w + 4, label_y, role.text);
    uint8_t fill = theme == Theme::net ? ui::net_chip : theme == Theme::lcd ? ui::lcd_screen : ui::amber_bg;
    ui::fill(right - 7, label_y + 2, 7, 4, fill);
    if (arrow)
        for (int r = 0; r < 4; ++r)
            ui::fill(right - 7 + r, label_y + 2 + r, 7 - 2 * r, 1, role.accent);
}
void welcome(const Controller& app)
{
    switch (theme)
    {
    case Theme::field:
    {
        // Red band; the title at 4x with the logo to its right, centered as a group.
        ui::fill(0, 0, 240, 76, ui::red);
        line(0, 1, 240, ui::header_light);
        ui::fill(0, 64, 240, 4, ui::hero_accent);
        ui::fill(0, 76, 240, 2, ui::header_shadow);
        ui::text(font::body, "Welcome to", 120, 14, ui::header_subtext, ui::no_shadow, 1, ui::align::center);
        int title_w = ui::text_width(font::body, "AddrBoy", 4);
        int x = (240 - (title_w + 8 + 32)) / 2;
        ui::text(font::body, "AddrBoy", x, 30, ui::white, ui::header_shadow, 4, ui::align::left, 2);
        logo32(x + title_w + 8, 28);
        break;
    }
    case Theme::net:
        // Deep band, the title in a cyan tab, the logo on the band right of it.
        ui::fill(0, 0, 240, 76, ui::net_deep);
        line(0, 76, 240, ui::net_cyan);
        ui::text(font::body, "Welcome to", 10, 13, ui::net_secondary);
        parallelogram(0, 26, 176, 36, 14, ui::net_cyan, true);
        ui::text(font::body, "AddrBoy", 10, 33, ui::net_deep, ui::no_shadow, 3);
        logo32(192, 28);
        break;
    case Theme::lcd:
        ui::fill(0, 0, 240, 76, ui::lcd_dark);
        ui::text(font::body, "Welcome to", 120, 16, ui::lcd_light, ui::no_shadow, 1, ui::align::center);
        logo_title_row("ADDRBOY", 3, ui::lcd_screen);
        for (int i = 0; i < 168; i += 6)
            ui::fill(36 + i, 64, 4, 4, ui::lcd_mid);
        break;
    case Theme::amber:
    default:
        ui::text(font::body, "WELCOME TO", 120, 18, ui::amber_dim, ui::no_shadow, 1, ui::align::center);
        logo_title_row("ADDRBOY", 3, ui::amber_text);
        ui::pattern_hline(4, 66, 232, 2, 4, ui::amber_frame);
        break;
    }
    // Two lines, as AuthBoy's welcome ("Set a password..." / "Welcome back!").
    const char* line_1 = "Welcome back!";
    char line_2[40];
    if (app.damaged_save)
        std::strcpy(line_2, "Your save needs attention");
    else if (!app.book.count)
    {
        line_1 = "Keep your receiving addresses";
        std::strcpy(line_2, "and show them as QR codes");
    }
    else
        std::snprintf(line_2, sizeof(line_2), "%d wallet%s in your address book", app.book.count,
            app.book.count == 1 ? "" : "s");
    box(welcome_x, welcome_y, welcome_w, welcome_h);
    if (theme == Theme::field)
    {
        ui::text(font::body, line_1, welcome_x + 6, welcome_y + 8, ui::ink, ui::light_shadow);
        ui::text(font::body, line_2, welcome_x + 6, welcome_y + 20, ui::ink, ui::light_shadow);
    }
    else
    {
        int border = welcome_border();
        ui::text(font::body, line_1, welcome_x + border + 8, welcome_y + border + 7, role.text);
        ui::text(font::body, line_2, welcome_x + border + 8, welcome_y + border + 19, role.text);
    }
    welcome_continue(true);
}
void settings(const Controller& app)
{
    header("Settings");
    char count[28];
    std::snprintf(count, sizeof(count), "%d / 64 wallets stored", app.book.count);
    info_box(count);
    // As AuthBoy's settings: Sort, then Theme, 18 px apart.
    box(4, 54, 232, 9 + 2 * 18 + 1);
    menu_item(4, 232, 59, "Sort", app.settings_item == 0, false, false, sort_names[int(app.book.sort)]);
    menu_item(4, 232, 77, "Theme", app.settings_item == 1, false, false, theme_names[int(theme)]);
    text("AddrBoy 0.1.0", 8, 110, role.dim, 224);
    text("Public wallet addresses / local save", 8, 125, role.dim, 224);
    const ui::hint hints[] = { { ui::dpad_up_down, "choose" }, { "A", "change" }, { "B", "back" } };
    footer(hints, 3, app.message);
}
}
void draw(const Controller& app)
{
    static Screen previous = Screen::damaged;
    static int previous_key = 0;
    static bool previous_case = false;
    static bool previous_message = false;
    static char previous_input[address_capacity] = { };
    bool key_only = (app.screen == Screen::name || app.screen == Screen::address) && app.screen == previous
        && theme == app.book.theme && previous_case == app.lowercase && !previous_message && !app.message
        && !std::strcmp(previous_input, app.input);
    previous = app.screen;
    previous_case = app.lowercase;
    previous_message = app.message;
    std::strcpy(previous_input, app.input);
    int old_key = previous_key;
    previous_key = app.key;
    theme = app.book.theme;
    role = roles_by_theme[int(theme)];
    if (key_only)
    {
        keyboard_key(app, old_key);
        keyboard_key(app, app.key);
        return;
    }
    background();
    switch (app.screen)
    {
    case Screen::welcome:
        welcome(app);
        break;
    case Screen::wallets:
        home(app, false);
        break;
    case Screen::networks:
        home(app, true);
        break;
    case Screen::receive:
        receive(app);
        break;
    case Screen::menu:
        menu(app);
        break;
    case Screen::name:
    case Screen::address:
        keyboard(app);
        break;
    case Screen::type:
        type(app);
        break;
    case Screen::setup:
        setup(app);
        break;
    case Screen::settings:
        settings(app);
        break;
    case Screen::remove_wallet:
        confirmation(app, "Delete wallet?", app.book.wallets[app.wallet_index()].name, "delete");
        break;
    case Screen::discard:
        confirmation(app, "Discard changes?", "Your saved wallet will stay as it is.", "discard");
        break;
    case Screen::remove_address:
        confirmation(app, "Remove this address?", network_names[app.setup_network], "remove");
        break;
    default:
    case Screen::damaged:
        header("Save needs attention");
        panel(4, 39, 232, 92);
        text("Save could not be read.", 12, 51, role.danger, 216);
        text("No data has been overwritten.", 12, 69, role.text, 216);
        text("Back up your .sav file, then restore", 12, 88, role.dim, 216);
        text("a backup or use a fresh save file.", 12, 104, role.dim, 216);
        break;
    }
}
void welcome_blink(const Controller& app, bool visible)
{
    theme = app.book.theme;
    role = roles_by_theme[int(theme)];
    welcome_continue(visible);
}
}
