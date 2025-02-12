#pragma once

#include <unordered_map>
#include "Comm.h"

class PageCache {
public:
    static PageCache *GetInstance() {
        return &_sInst;
    }

    // 给CentalCache的k页span
    Span *NewSpan(size_t k);

    // 根据地址算页号对应的Span的位置
    Span *MapObjectToSpan(void *obj);

    // 回收centralcache的内存给pagecache
    void ReleaseSpanToPageCache(Span *span);

private:
    PageCache() {}

    PageCache(const PageCache &) = delete;

    static PageCache _sInst; // 饿汉

    SpanList _spanLists[NPAGES]; // 分别有 1，2，3 ... 128 page 的span自由链表

    std::unordered_map<PageId, Span *> _idSpanMap; // 页号映射对应的centralcache的Span位置
public:
    std::mutex _pageMtx;
};