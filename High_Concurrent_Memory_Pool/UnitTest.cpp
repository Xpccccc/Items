#include "concurrentAlloc.h"
#include <thread>
#include <vector>

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
    for (int i = 0; i < 1024; ++i) {
        void *p1 = ConcurrentAlloc(6);
    }
    void *p1 = ConcurrentAlloc(8);

}


void TestConCurrent3() {
    void *p1 = ConcurrentAlloc(6);
    void *p2 = ConcurrentAlloc(8);
    void *p3 = ConcurrentAlloc(1);
    void *p4 = ConcurrentAlloc(7);
    void *p5 = ConcurrentAlloc(8);
    void *p6 = ConcurrentAlloc(8);
    void *p7 = ConcurrentAlloc(8);

    std::cout << p1 << std::endl;
    std::cout << p2 << std::endl;
    std::cout << p3 << std::endl;
    std::cout << p4 << std::endl;
    std::cout << p5 << std::endl;

    ConcurrentFree(p1, 6);
    ConcurrentFree(p2, 8);
    ConcurrentFree(p3, 1);
    ConcurrentFree(p4, 7);
    ConcurrentFree(p5, 8);
    ConcurrentFree(p6, 8);
    ConcurrentFree(p7, 8);

}


void MultiThread1() {
    std::vector<void *> v;
    for (int i = 0; i < 5; ++i) {
        void *p = ConcurrentAlloc(6);
        v.push_back(p);
    }
    for (auto &e: v) {
        ConcurrentFree(e, 6);
    }
}

void MultiThread2() {
    std::vector<void *> v;
    for (int i = 0; i < 5; ++i) {
        void *p = ConcurrentAlloc(7);
        v.push_back(p);
    }
    for (auto &e: v) {
        ConcurrentFree(e, 7);
    }
}

void TestMultiThread() {
    std::thread t1(MultiThread1);
    std::thread t2(MultiThread2);

    t1.join();
    t2.join();
}

int main() {
//    TestTLS();
//    TestConCurrent1();
//    TestConCurrent2();
//    TestConCurrent3();
    TestMultiThread();
    return 0;

}