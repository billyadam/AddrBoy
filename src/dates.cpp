#include "dates.h"
namespace addr
{
namespace
{
bool leap(int year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
int month_days(int year, int month)
{
    constexpr int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    return days[month - 1] + (month == 2 && leap(year));
}
}
uint16_t day_from_ymd(int year, int month, int day)
{
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > month_days(year, month))
        return 0;
    int value = 1;
    for (int y = 2000; y < year; ++y)
        value += 365 + leap(y);
    for (int m = 1; m < month; ++m)
        value += month_days(year, m);
    return uint16_t(value + day - 1);
}
void ymd_from_day(uint16_t value, int& year, int& month, int& day)
{
    if (!value || value > last_day)
    {
        year = month = day = 0;
        return;
    }
    int remaining = value - 1;
    year = 2000;
    while (remaining >= 365 + leap(year))
    {
        remaining -= 365 + leap(year);
        ++year;
    }
    month = 1;
    while (remaining >= month_days(year, month))
    {
        remaining -= month_days(year, month);
        ++month;
    }
    day = remaining + 1;
}
int age_days(uint16_t today, uint16_t then)
{
    return !today || !then || today > last_day || then > last_day || then > today ? -1 : today - then;
}
}
