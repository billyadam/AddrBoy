#ifndef ADDRBOY_DATES_H
#define ADDRBOY_DATES_H
#include <cstdint>
namespace addr
{
// 0 means unknown; 1 is 2000-01-01. Fits every date through 2099-12-31.
constexpr uint16_t last_day = 36525;
uint16_t day_from_ymd(int year, int month, int day);
void ymd_from_day(uint16_t value, int& year, int& month, int& day);
// -1 means that the date or RTC is unavailable, or the saved date is in the future.
int age_days(uint16_t today, uint16_t then);
}
#endif
