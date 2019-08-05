#include "Date.h"

#include <iostream>

using namespace std;
using namespace hxmmxh;

static int test_count = 0;
static int test_pass = 0;

#define CHECKEQUAL(expect, actual)                                                            \
    do                                                                                        \
    {                                                                                         \
        test_count++;                                                                         \
        if ((expect) == (actual))                                                             \
            test_pass++;                                                                      \
        else                                                                                  \
        {                                                                                     \
            std::cout << "line: " << __LINE__ << "error:   " << #expect << " != " << #actual; \
            std::cout << "    " #expect << " = " << expect << std::endl;                      \
        }                                                                                     \
    } while (0)

const int kMonthsOfYear = 12;

//判断是否闰年
int isLeapYear(int year)
{
    if (year % 400 == 0)
        return 1;
    else if (year % 100 == 0)
        return 0;
    else if (year % 4 == 0)
        return 1;
    else
        return 0;
}

int daysOfMonth(int year, int month)
{
    static int days[2][kMonthsOfYear + 1] =
        {
            {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
            {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        };
    return days[isLeapYear(year)][month];
}

void passByConstReference(const Date &x)
{
    cout << x.toIsoString() << endl;
}

void passByValue(Date x)
{
    cout << x.toIsoString() << endl;
}

int main()
{
    time_t now = time(NULL);
    struct tm t1 = *gmtime(&now);    //UTC时间，无时区转换
    struct tm t2 = *localtime(&now); //本地时间，有时区转换
    Date someDay(2008, 9, 10);
    cout << someDay.toIsoString() << endl;
    passByValue(someDay);          //结果应为2008-9-10
    passByConstReference(someDay); //结果应为2008-9-10
    Date todayUtc(t1);
    cout << todayUtc.toIsoString() << endl;
    Date todayLocal(t2);
    cout << todayLocal.toIsoString() << endl;

    int julianDayNumber = 2415021; //1900-1-1
    int weekDay = 1;               // Monday

    for (int year = 1900; year < 2500; ++year)
    {
        //如果不是闰年，则3月1号和2月29是同一天，用来判断儒略日数的计算是否正确考虑到了闰年
        CHECKEQUAL(Date(year, 3, 1).julianDayNumber() - Date(year, 2, 29).julianDayNumber(),isLeapYear(year));
        for (int month = 1; month <= kMonthsOfYear; ++month)
        {
            for (int day = 1; day <= daysOfMonth(year, month); ++day)
            {
                Date d(year, month, day);
                // printf("%s %d\n", d.toString().c_str(), d.weekDay());
                CHECKEQUAL(year, d.year());
                CHECKEQUAL(month, d.month());
                CHECKEQUAL(day, d.day());
                CHECKEQUAL(weekDay, d.weekDay());
                CHECKEQUAL(julianDayNumber, d.julianDayNumber());

                Date d2(julianDayNumber);
                CHECKEQUAL(year, d2.year());
                CHECKEQUAL(month, d2.month());
                CHECKEQUAL(day, d2.day());
                CHECKEQUAL(weekDay, d2.weekDay());
                CHECKEQUAL(julianDayNumber, d2.julianDayNumber());

                ++julianDayNumber;
                weekDay = (weekDay + 1) % 7;
            }
        }
    }
    std::cout << test_pass << "/" << test_count << " passed\n";
}