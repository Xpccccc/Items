#pragma once

#include "Comm.h"

class ThreadCache
{
public:
    // 申请和释放内存对象
    void *Allocate(size_t size);

    void DeAllocate(void *ptr, size_t size);

    // 从Central Cache获取内存对象
    void *FetchFromCentralCache(size_t index, size_t size);

private:
    FreeList _freeLists[NFREE_LIST];
};

static thread_local ThreadCache* tls_ptr = nullptr; // 使用 thread_local 关键字声明一个指向 TLS 变量的指针
