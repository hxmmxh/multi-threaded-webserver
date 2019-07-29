#include "TimeZone.h"
#include "Date.h"

namespace hxmmxh
{
namespace detail
{
struct Transition
{
    time_t gmttime;   //UTC时间
    time_t localtime; //本地时间
    int localtimeIdx;

    Transition(time_t t, time_t l, int localIdx) : gmttime(t), localtime(l), localtimeIdx(localIdx) {}
};

struct Comp
{
    bool compareGmt; //为真时比较UTC,为假时比较local

    Comp(bool gmt) : compareGmt(gmt) {}

    bool operator()(const Transition &lhs, const Transition &rhs) const
    {
        if (compareGmt)
            return lhs.gmttime < rhs.gmttime;
        else
            return lhs.localtime < rhs.localtime;
    }

    bool equal(const Transition &lhs, const Transition &rhs) const
    {
        if (compareGmt)
            return lhs.gmttime == rhs.gmttime;
        else
            return lhs.localtime == rhs.localtime;
    }
};
/////
struct Localtime
{
    time_t gmtOffset; //和格林威治时间差了多少秒
    bool isDst;       //是否夏令时
    int arrbIdx;

    Localtime(time_t offset, bool dst, int arrb) : gmtOffset(offset), isDst(dst), arrbIdx(arrb) {}
};

//用seconds填充utc时间结构的时分秒
inline void fillHMS(unsigned seconds, struct tm *utc)
{
    utc->tm_sec = seconds % 60;
    unsigned minutes = seconds / 60;
    utc->tm_min = minutes % 60;
    utc->tm_hour = minutes / 60;
}

} // namespace detail
const int kSecondsPerDay = 24 * 60 * 60;
} // namespace hxmmxh

using namespace muduo;
using namespace std;

struct TimeZone::Data
{
    vector<detail::Transition> transitions;
    vector<detail::Localtime> localtimes;
    vector<string> names;
    string abbreviation; //缩写
};

namespace hxmmxh
{
namespace detail
{
}
} // namespace hxmmxh