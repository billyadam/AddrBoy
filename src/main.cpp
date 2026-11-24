#include "bn_core.h"
#include "bn_keypad.h"
#include "controller.h"
#include "screens.h"
#include "ui.h"

namespace
{
BN_DATA_EWRAM addr::Controller app;
}
int main()
{
    bn::core::init();
    ui::init();
    app.boot();
    addr::draw(app);
    int held_frames = 0, last_direction = -1, blink_counter = 0;
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
        if (direction >= 0)
        {
            if (held_frames == 0 || (held_frames >= 18 && (held_frames - 18) % 5 == 0))
            {
                app.press(addr::Button(direction));
                changed = true;
            }
            ++held_frames;
        }
        const bool presses[] = { bn::keypad::a_pressed(), bn::keypad::b_pressed(), bn::keypad::l_pressed(),
            bn::keypad::r_pressed(), bn::keypad::start_pressed(), bn::keypad::select_pressed() };
        for (int i = 0; i < 6; ++i)
            if (presses[i])
            {
                app.press(addr::Button(i + 4));
                changed = true;
                break;
            }
        if (changed)
            addr::draw(app);
    }
}
