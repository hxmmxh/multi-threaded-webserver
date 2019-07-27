#include "CurrentThread.h"

#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>

using namespace hxmmxh;
using namespace CurrentThread;
using namespace std;

void f()
{
    tid();
    std::cout << tidString() << std::endl;
    std::cout <<"is " <<(isMainThread()?"":"not")<<"MainThread"<<std::endl;
    if(!isMainThread())
        std::cout << "MainThread is" << ::getpid() << std::endl;
}

int main()
{
    vector<thread> ts(10);
    for (int i = 0; i < 10 ; ++i)
    {
        ts[i] = thread(f);
        ts[i].join();
    }
}


   