//
// Created by 徐鹏 on 2025/2/16.
//

#include "concurrentAlloc.h"

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdlib>

void BenchMarkAlloc(size_t nTimes, size_t nWorks, size_t rounds)
{
    std::vector<std::thread> vThread(nWorks);
    std::atomic<size_t> malloc_costtime;
    malloc_costtime = 0;
    std::atomic<size_t> free_costtime;
    free_costtime = 0;

    for (size_t k = 0; k < nWorks; ++k)
    {
        vThread[k] = std::thread([&]()
                                 {
            std::vector<void *> v;
            v.reserve(nTimes);
            for (size_t j = 0; j < rounds; ++j) {
                auto begin1 = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < nTimes; ++i) {
                    v.push_back(malloc(16));
                    // v.push_back(malloc((16 + i) % 8192 + 1));
                }
                auto end1 = std::chrono::high_resolution_clock::now();
                auto begin2 = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < nTimes; ++i) {
                    free(v[i]);
                }
                auto end2 = std::chrono::high_resolution_clock::now();
                v.clear();
                malloc_costtime += std::chrono::duration_cast<std::chrono::milliseconds>(end1 - begin1).count();
                free_costtime += std::chrono::duration_cast<std::chrono::milliseconds>(end2 - begin2).count();
            } });
    }

    for (auto &t : vThread)
    {
        t.join();
    }

    printf("%zu个线程并发执行%zu轮次，每轮malloc %zu 次，花费：%zu ms \n", nWorks, rounds, nTimes, malloc_costtime.load());
    printf("%zu个线程并发执行%zu轮次，每轮free %zu 次，花费：%zu ms \n", nWorks, rounds, nTimes, free_costtime.load());
    printf("%zu个线程并发执行 malloc & free %zu轮次，总花费：%zu ms \n", nWorks, rounds,
           malloc_costtime.load() + free_costtime.load());
}

void BenchMarkConcurrentAlloc(size_t nTimes, size_t nWorks, size_t rounds)
{
    std::vector<std::thread> vThread(nWorks);
    std::atomic<size_t> malloc_costtime;
    malloc_costtime = 0;
    std::atomic<size_t> free_costtime;
    free_costtime = 0;

    for (size_t k = 0; k < nWorks; ++k)
    {
        vThread[k] = std::thread([&]()
                                 {
            std::vector<void *> v;
            v.reserve(nTimes);
            for (size_t j = 0; j < rounds; ++j) {
                auto begin1 = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < nTimes; ++i) {
                    v.push_back(ConcurrentAlloc(16));
                    // v.push_back(ConcurrentAlloc((16 + i) % 8192 + 1));
                }
                auto end1 = std::chrono::high_resolution_clock::now();
                auto begin2 = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < nTimes; ++i) {
                    ConcurrentFree(v[i]);
                }
                auto end2 = std::chrono::high_resolution_clock::now();
                v.clear();
                malloc_costtime += std::chrono::duration_cast<std::chrono::milliseconds>(end1 - begin1).count();
                free_costtime += std::chrono::duration_cast<std::chrono::milliseconds>(end2 - begin2).count();
            } });
    }

    for (auto &t : vThread)
    {
        t.join();
    }

    printf("%zu个线程并发执行%zu轮次，每轮malloc %zu 次，花费：%zu ms \n", nWorks, rounds, nTimes, malloc_costtime.load());
    printf("%zu个线程并发执行%zu轮次，每轮free %zu 次，花费：%zu ms \n", nWorks, rounds, nTimes, free_costtime.load());
    printf("%zu个线程并发执行 malloc & free %zu轮次，总花费：%zu ms \n", nWorks, rounds,
           malloc_costtime.load() + free_costtime.load());
}

int main()
{
    printf("Alloc:\n");
    BenchMarkAlloc(100000, 4, 10); // 示例参数
    printf("============================\n");
    printf("ConcurrentAlloc:\n");
    BenchMarkConcurrentAlloc(100000, 4, 10); // 示例参数
    return 0;
}