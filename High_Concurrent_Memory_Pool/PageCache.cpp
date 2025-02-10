#include "PageCache.h"

PageCache PageCache::_sInst;

Span *PageCache::NewSpan(size_t k)
{
    assert(k > 0 && k < NPAGES);
    if (!_spanLists[k].Empty())
    {
        return _spanLists[k].PopFront();
    }

    // 检查后面的桶有没有span，有的话进行切分
    for (int i = k + 1; k < NPAGES; ++k)
    {
        if (!_spanLists[i].Empty())
        {
            Span *nSpan = _spanLists[i].PopFront();
            Span *kSpan = new Span;

            // 切分,从头切
            // kSpan返回，剩余的挂到n-k的spanlist里面
            kSpan->_pageId = nSpan->_pageId;
            kSpan->_n = k;

            nSpan->_pageId += k;
            nSpan->_n -= k;

            _spanLists[nSpan->_n].PushFront(nSpan);
            return kSpan;
        }
    }

    // 走到这里说明后面没有大页的Span
    // 找堆要128页的span
    Span *bigSpan = new Span;
    void *ptr = SystemAlloc(NPAGES - 1);
    bigSpan->_pageId = (PageId)ptr >> PAGE_SHIFT;
    bigSpan->_n = NPAGES - 1;

    _spanLists[bigSpan->_n].PushFront(bigSpan);
    return NewSpan(k);
}
