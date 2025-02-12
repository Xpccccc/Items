#include "CentralCache.h"
#include "Comm.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

Span *CentralCache::GetOneSpan(SpanList &list, size_t size) {
    // 查看是否CentalCache是否有未分配的span
    Span *it = list.Begin();
    while (it != list.End()) {
        if (it->_freeList != nullptr) {
            return it;
        } else {
            it = it->_next;
        }
    }

    // 上面的while还是得锁住，因为找空闲span分配，得锁住
    // 先把centalcache的桶锁解了，这样如果其他线程用完内存还回来，不会阻塞
    list._mtx.unlock();

    // 没有，找PageCache
    PageCache::GetInstance()->_pageMtx.lock(); // 整体锁住
    Span *span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
    span->_isUse = true;
    PageCache::GetInstance()->_pageMtx.unlock();

    // 这里不需要加锁，因为还没有把span挂上cental cache
    // 当前的span的大块内存的起始地址和大小
    char *start = (char *) (span->_pageId << PAGE_SHIFT);
    size_t bytes = (span->_n) << PAGE_SHIFT;
    // 大块内存切分成size大小，用自由链表链接起来
    char *end = start + bytes;

    span->_freeList = start;
    start += size;
    void *tail = span->_freeList; // 用来尾插
    while (start < end) {
        NextObj(tail) = start;
        tail = NextObj(tail);
        start += size;
    }

    // 切完了
    list._mtx.lock();   // 要挂上桶里面需要加锁，防止混乱插入拿取
    list.PushFront(span); // 插入到centalcache里面

    return span;
}

size_t CentralCache::FetchRangeObj(void *&start, void *&end, size_t batchNum, size_t size) {
    // 先看要去那个桶拿
    size_t index = SizeClass::Index(size);

    // 访问共享资源要加锁
    _spanLists[index]._mtx.lock();
    Span *span = GetOneSpan(_spanLists[index], size);
    assert(span);
    assert(span->_freeList); // 该span下面有内存

    // 从span下面的自由链表中拿batchNum个size大小的内存块
    start = span->_freeList;
    end = start;
    size_t i = 0;
    size_t actualNum = 1;
    // 不够的话，有几个给几个
    while (i < batchNum - 1 && NextObj(end) != nullptr) {
        end = NextObj(end);
        ++i;
        ++actualNum;
    }
    span->_freeList = NextObj(end);
    NextObj(end) = nullptr;

    span->_useCount += actualNum;


    _spanLists[index]._mtx.unlock();

    return actualNum;
}

void CentralCache::ReleaseListToSpans(void *start, size_t size) {
    // 找到对应的桶
    size_t index = SizeClass::Index(size);

    _spanLists[index]._mtx.lock();

    while (start) {
        void *next = NextObj(start);
        Span *span = PageCache::GetInstance()->MapObjectToSpan(start);
        // 头插
        NextObj(start) = span->_freeList;
        span->_freeList = start;

        span->_useCount--;

        // 说明span切分出去的小块内存都回来了
        // 这个span就可以还给pagecache，pagecache再尝试前后页的合并
        if (span->_useCount == 0) {
            _spanLists[index].Erase(span);
            span->_freeList = nullptr;
            span->_next = nullptr;
            span->_prev = nullptr;


            // 把桶锁解掉，因为这里已经不会再动这个桶的span了
            _spanLists[index]._mtx.unlock();

            // 页号和页数得保留
            PageCache::GetInstance()->_pageMtx.lock();
            PageCache::GetInstance()->ReleaseSpanToPageCache(span);
            PageCache::GetInstance()->_pageMtx.unlock();

            _spanLists[index]._mtx.lock();
        }
        start = next;
        
    }

    _spanLists[index]._mtx.unlock();
}
