#include "concurrentAlloc.h"
#include <thread>

void Alloc1() {
    for (int i = 0; i < 5; ++i) {
        void *p = ConcurrentAlloc(1 + i * 10);
    }
}

void Alloc2() {
    for (int i = 0; i < 5; ++i) {
        void *p = ConcurrentAlloc(7);
    }
}

void TestTLS() {
    std::thread t1(Alloc1);
    std::thread t2(Alloc2);

    t1.join();
    t2.join();
}


void TestConCurrent1() {
    void *p1 = ConcurrentAlloc(6);
    void *p2 = ConcurrentAlloc(8);
    void *p3 = ConcurrentAlloc(1);
    void *p4 = ConcurrentAlloc(7);
    void *p5 = ConcurrentAlloc(8);

    std::cout << p1 << std::endl;
    std::cout << p2 << std::endl;
    std::cout << p3 << std::endl;
    std::cout << p4 << std::endl;
    std::cout << p5 << std::endl;
}

void TestConCurrent2() {
    for(int i = 0; i<1024; ++i){
        void *p1 = ConcurrentAlloc(6);
    }
    void *p1 = ConcurrentAlloc(8);

}

int main() {
//    TestTLS();
    TestConCurrent1();
    return 0;
}