#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_date.h"
#include "bn_optional.h"
#include "controller.h"
#include "dates.h"
#include "screens.h"
#include "ui.h"

namespace
{
BN_DATA_EWRAM addr::Controller app;
uint16_t rtc_today()
{
    if (auto date = bn::date::current())
        return addr::day_from_ymd(2000 + date->year(), date->month(), date->month_day());
    return 0;
}
void press(addr::Button button)
{
    app.today = rtc_today();
    app.press(button);
}
}
int main()
{
    bn::core::init();
    ui::init();
    app.today = rtc_today();
    app.boot();
    addr::draw(app);
    int held_frames = 0, last_direction = -1, blink_counter = 0, clock_frames = 0;
    while (true)
    {
        bn::core::update();
        // Welcome screen: the "continue" arrow blinks every 30 frames, as in AuthBoy.
        if (app.screen == addr::Screen::welcome && ++blink_counter % 30 == 0)
            addr::welcome_blink(app, (blink_counter / 30) % 2 == 0);
        int direction = bn::keypad::up_held() ? 0
            : bn::keypad::down_held()         ? 1
            : bn::keypad::left_held()         ? 2
            : bn::keypad::right_held()        ? 3
                                              : -1;
        if (direction != last_direction)
        {
            held_frames = 0;
            last_direction = direction;
        }
        bool changed = false;
        if (++clock_frames == 3600) // Refresh the reminder if the date rolls over while the ROM stays open.
        {
            clock_frames = 0;
            uint16_t today = rtc_today();
            if (app.today != today)
            {
                app.today = today;
                changed = true;
            }
        }
        if (direction >= 0)
        {
            if (held_frames == 0 || (held_frames >= 18 && (held_frames - 18) % 5 == 0))
            {
                press(addr::Button(direction));
                changed = true;
            }
            ++held_frames;
        }
        const bool presses[] = { bn::keypad::a_pressed(), bn::keypad::b_pressed(), bn::keypad::l_pressed(),
            bn::keypad::r_pressed(), bn::keypad::start_pressed(), bn::keypad::select_pressed() };
        for (int i = 0; i < 6; ++i)
            if (presses[i])
            {
                press(addr::Button(i + 4));
                changed = true;
                break;
            }
        if (changed)
            addr::draw(app);
    }
}
