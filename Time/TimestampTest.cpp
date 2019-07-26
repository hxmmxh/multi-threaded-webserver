#include "Timestamp.h"
#include "Timestamp.cpp"

#include <vector>
#include <iostream>

using namespace std;

void passByConstReference(const Timestamp &x)
{
    cout << x.toString() << std::endl;
}

void passByValue(Timestamp x)
{
    cout << x.toString() << std::endl;
}

//benchmark:标准测试程序
void benchmark()
{
    const int kNumber = 1000 * 1000;

    std::vector<Timestamp> stamps;
    stamps.reserve(kNumber);
    for (int i = 0; i < kNumber; ++i)
    {
        stamps.push_back(Timestamp::now());
    }
    cout << stamps.front().toString() << '\n';
    cout << stamps.back().toString() << '\n';
    //返回的是秒数
    cout << timeDifference(stamps.back(), stamps.front()) << '\n';
    //统计创建两个时间戳之间的不同时间间隔的次数
    int increments[1000] = {0};
    int64_t start = stamps.front().microSecondsSinceEpoch();
    for (int i = 1; i < kNumber; ++i)
    {
        int64_t next = stamps[i].microSecondsSinceEpoch();
        //stamps中相邻两个元素创建的时间间隔
        int64_t inc = next - start;
        start = next;
        if (inc < 0)
        {
            cout << "reverse!\n";
        }
        else if (inc < 1000)
        {
            ++increments[inc];
        }
        else
        {
            cout << "big gap " << static_cast<int>(inc)<<'\n';
        }
    }

    for (int i = 0; i < 1000; ++i)
    {
        if(increments[i]>0)
            cout << i << " : " << increments[i]<<'\n';
    }
}

int main()
{
    Timestamp now(Timestamp::now());
    cout << now.toString() << std::endl;
    //cout << now.toFormattedString() << std::endl;
    passByValue(now);
    passByConstReference(now);
    benchmark();
}
