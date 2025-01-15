#pragma once

#include "Comm.h"

class PageCache
{
public:
    static PageCache *GetInstance()
    {
        return &_sInst;
    }

    // 给CentalCache的k页span
    Span *NewSpan(size_t k);

private:
    PageCache() {}

    PageCache(const PageCache &) = delete;

    static PageCache _sInst; // 饿汉

    SpanList _spanLists[NPAGES]; // 分别有 1，2，3 ... 128 page 的span自由链表
    std::mutex _pageMtx;
};