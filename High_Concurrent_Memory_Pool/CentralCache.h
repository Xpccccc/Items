#pragma once

#include "Comm.h"

// 整个进程只有一个centalcache，使用单例模式
class CentalCache
{
public:
    static CentalCache *GetInstance()
    {
        return &_sInst;
    }

    // 获取一个非空span
    Span *GetOneSpan(SpanList &list, size_t size);

    size_t FetchRangeObj(void *start, void *end, size_t batchNum, size_t size);

private:
    SpanList _spanLists[NFREE_LIST];
    CentalCache() {}
    CentalCache(const CentalCache &) = delete;

private:
    static CentalCache _sInst;
};