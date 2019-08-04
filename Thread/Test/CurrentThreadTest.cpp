#include "CurrentThread.h"
#include "Thread.h"

#include <iostream>
#include <vector>
#include <unistd.h>

using namespace hxmmxh;
using namespace CurrentThread;
using namespace std;

void f()
{
    tid();
    cout << tidString() << std::endl;
    cout <<"is " <<(isMainThread()?"":"not")<<"MainThread"<<std::endl;
    if(!isMainThread())
        std::cout << "MainThread is" << ::getpid() << std::endl;
}

int main()
{
    for (int i = 0; i < 10 ; ++i)
    {
        Thread t(f);
        t.start();
        t.join();
    }
}


   