#pragma once

#include "Comm.h"
#include "ThreadCache.h"
#include <thread>
#include "PageCache.h"

static void *ConcurrentAlloc(size_t size) {
    if (size > MAX_BYTES) {
        // centralcache没有这么大的，找Pagecache
        size_t alignSize = SizeClass::RoundUp(size);
        size_t kpage = alignSize >> PAGE_SHIFT;
        PageCache::GetInstance()->_pageMtx.lock();
        Span *span = PageCache::GetInstance()->NewSpan(kpage);
        span->_objSize = size;
        PageCache::GetInstance()->_pageMtx.unlock();
        void *ptr = (void *) (span->_pageId << PAGE_SHIFT); // 根据页号找到地址
        return ptr;
    } else {
        // 通过TLS，每个线程无锁的获取自己的专属ThreadCache对象
        if (tls_ptr == nullptr) {
            static ObjectPool<ThreadCache> _tcPool;
            tls_ptr = _tcPool.New();
//            tls_ptr = new ThreadCache;
        }
//        std::cout << std::this_thread::get_id() << " : " << tls_ptr << std::endl;
        return tls_ptr->Allocate(size);
    }

}

//static void ConcurrentFree(void *ptr, size_t size) {
//
//    if (size > MAX_BYTES) {
//        // 找Pagecache释放
//        Span *span = PageCache::GetInstance()->MapObjectToSpan(ptr);
//
//        // 吧Span还给Pagecache或者还给堆
//        PageCache::GetInstance()->_pageMtx.lock();
//        PageCache::GetInstance()->ReleaseSpanToPageCache(span);
//        PageCache::GetInstance()->_pageMtx.unlock();
//
//    } else {
//        assert(tls_ptr);
//        tls_ptr->DeAllocate(ptr, size);
//    }
//
//}

static void ConcurrentFree(void *ptr) {
    Span *span = PageCache::GetInstance()->MapObjectToSpan(ptr);
    size_t size = span->_objSize;
    if (size > MAX_BYTES) {
        // 找Pagecache释放


        // 吧Span还给Pagecache或者还给堆
        PageCache::GetInstance()->_pageMtx.lock();
        PageCache::GetInstance()->ReleaseSpanToPageCache(span);
        PageCache::GetInstance()->_pageMtx.unlock();

    } else {
        assert(tls_ptr);
        tls_ptr->DeAllocate(ptr, size);
    }

}
