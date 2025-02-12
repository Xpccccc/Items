#pragma once

#include "Comm.h"
#include "ThreadCache.h"
#include <thread>

static void *ConcurrentAlloc(size_t size)
{
    // 通过TLS，每个线程无锁的获取自己的专属ThreadCache对象
    if (tls_ptr == nullptr)
    {
        tls_ptr = new ThreadCache;
    }
    std::cout << std::this_thread::get_id() << " : " << tls_ptr << std::endl;
    return tls_ptr->Allocate(size);
}
static void ConcurrentFree(void *ptr, size_t size)
{
    assert(tls_ptr);
    tls_ptr->DeAllocate(ptr, size);
}
