#include "concurrentAlloc.h"
#include <thread>

void Alloc1()
{
    for (int i = 0; i < 5; ++i)
    {
        void *p = ConcurrentAlloc(6);
    }
}

void Alloc2()
{
    for (int i = 0; i < 5; ++i)
    {
        void *p = ConcurrentAlloc(7);
    }
}

void TestTLS()
{
    std::thread t1(Alloc1);
    std::thread t2(Alloc2);

    t1.join();
    t2.join();
}

int main()
{
    TestTLS();
    return 0;
}