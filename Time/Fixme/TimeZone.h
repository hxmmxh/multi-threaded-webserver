#ifndef HXMMXH_TIMEZONE_H
#define HXMMXH_TIMEZONE_H

#include <memory>
#include <time.h>

namespace hxmmxh
{
class TimeZone
{
public:
    explicit TimeZone(const char *zonefile);
    TimeZone(int eastOfUtc, const char *tzname);
    TimeZone() = default;

    //本地时间与秒数的转换
    struct tm toLocalTime(time_t secondsSinceEpoch) const;
    time_t fromLocalTime(const struct tm &) const;

    //UTC时间与秒数的转换
    static struct tm toUtcTime(time_t secondsSinceEpoch, bool yday = false);
    static time_t fromUtcTime(const struct tm &);
    static time_t fromUtcTime(int year, int month, int day,
                              int hour, int minute, int seconds);
    struct Data;

private:
    std::shared_ptr<Data> data_;
};
} // namespace hxmmxh

#endif
