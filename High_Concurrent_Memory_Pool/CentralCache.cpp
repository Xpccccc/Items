#include "CentralCache.h"
#include "Comm.h"
#include "PageCache.h"

CentalCache CentalCache::_sInst;

Span *CentalCache::GetOneSpan(SpanList &list, size_t size)
{
    // 查看是否CentalCache是否有未分配的span
    Span *it = list.Begin();
    while (it != list.End())
    {
        if (it->_freeList != nullptr)
        {
            return it;
        }
        else
        {
            it = it->_next;
        }
    }

    // 没有，找PageCache
    Span *span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
    // 当前的span的大块内存的起始地址和大小
    char *start = (char *)(span->_pageId << PAGE_SHIFT);
    size_t bytes = (span->_n) << PAGE_SHIFT;
    // 大块内存切分成size大小，用自由链表链接起来
    char *end = start + bytes;

    span->_freeList = start;
    start += size;
    void *tail = span->_freeList; // 用来尾插
    while (start < end)
    {
        NextObj(tail) = start;
        tail = NextObj(tail);
        start += size;
    }

    // 切完了
    list.PushFront();// 插入到centalcache里面

    return nullptr;
}

size_t CentalCache::FetchRangeObj(void *start, void *end, size_t batchNum, size_t size)
{
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
    while (i < batchNum - 1 && NextObj(end) != nullptr)
    {
        end = NextObj(end);
        ++i;
        ++actualNum;
    }
    span->_freeList = NextObj(end);
    NextObj(end) = nullptr;

    _spanLists[index]._mtx.unlock();

    return actualNum;
}
