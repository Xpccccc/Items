#pragma once

#include "Comm.h"

#ifdef _WIN32
#include <windows.h>
#else

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#endif

// 定长内存池
//#include <stdexcept> // for std::bad_alloc
//
//inline static void *SystemAlloc(size_t kpage)
//{
//#ifdef _WIN32
//    void *ptr = VirtualAlloc(0, kpage * 8 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//#else
//    // Linux
//    void *ptr = mmap(nullptr, kpage * 8 * 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
//    if (ptr == MAP_FAILED)
//    {
//        // 错误处理
//        perror("mmap failed");
//        exit(EXIT_FAILURE);
//    }
//#endif
//
//    if (ptr == nullptr)
//    {
//        throw std::bad_alloc();
//    }
//    return ptr;
//}

template<class T>
class ObjectPool {
public:
    T *New() {
        std::lock_guard<std::mutex> lock(_mtx); // 加锁，确保线程安全
        T *obj = nullptr;
        // 先用自由链表的内存
        if (_freelist) {
            void *next = *((void **) _freelist);
            obj = (T *) _freelist;
            _freelist = next;
        } else {
            // 剩余字节不够一个对象大小，需要开辟
            if (_remainBytes < sizeof(T)) {
                if (sizeof(T) > 128 * 1024) {
                    _remainBytes = sizeof(T);
                } else {
                    _remainBytes = 128 * 1024;
                }
                // _memory = (char *)malloc(_remainBytes); // 128字节
                _memory = (char *) SystemAlloc(_remainBytes >> PAGE_SHIFT); // 申请的页数
                if (_memory == nullptr) {
                    throw std::bad_alloc();
                }
            }
            obj = (T *) _memory;

            size_t objSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T); // 防止存不下指针
            _memory += objSize;
            _remainBytes -= objSize;
        }

        // 定位new，显式调用构造

        new(obj) T;
        return obj;
    }

    void Delete(T *obj) {
        std::lock_guard<std::mutex> lock(_mtx); // 加锁，确保线程安全
        // 显式调用析构
        obj->~T();
        // 头插
        *(void **) obj = _freelist;
        _freelist = obj;
    }

private:
    char *_memory = nullptr; // 指向大块内存指针 共享资源
    size_t _remainBytes = 0; // 剩余字节
    void *_freelist = nullptr;
    std::mutex _mtx;
};
