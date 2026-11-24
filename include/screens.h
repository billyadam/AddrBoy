#ifndef ADDRBOY_SCREENS_H
#define ADDRBOY_SCREENS_H
#include "controller.h"
namespace addr
{
void draw(const Controller& app);
// Welcome screen: redraws only the blinking "continue" arrow.
void welcome_blink(const Controller& app, bool visible);
}
#endif
