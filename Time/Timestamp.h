
#ifndef HXMMXH_TIMESTAMP_H
#define HXMMXH_TIMESTAMP_H

#include "Comparable.h"

#include <cinttypes> //int64_t
#include <string>
#include <sys/time.h> //time_t

namespace hxmmxh
{
//提供更精确的Unix Timestamp
//系统时间只精确到秒，现在精确到微妙
//其含义是1970年01月01日00时00分00秒起至现在的的总微秒数
class Timestamp : public equality_comparable_hxmmxh<Timestamp>,
                  public less_than_comparable_hxmmxh<Timestamp>
{
public:
    Timestamp() : microSecondsSinceEpoch_(0) {}
    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
    {
    }

    void swap(Timestamp &that)
    {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }
    //转换成"seconds.microseconds"的格式小数点后保留6位，例如”123.213122“
    std::string toString() const;
    //转换成格式化的时间
    //
    std::string toFormattedString(bool showMicroseconds = true) const;

    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    time_t secondsSinceEpoch() const
    {
        return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    }

    static Timestamp now();
    static Timestamp invalid()
    {
        return Timestamp();
    }
    static Timestamp fromUnixTime(time_t t)
    {
        return fromUnixTime(t, 0);
    }
    static Timestamp fromUnixTime(time_t t, int microseconds)
    {
        return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
    }
    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

//返回秒
inline double timeDifference(Timestamp high, Timestamp low)
{
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

//传入的参数是秒
inline Timestamp addTime(Timestamp timestamp, double seconds)
{
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

} // namespace hxmmxh

#endif