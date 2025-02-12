#include "PageCache.h"

PageCache PageCache::_sInst;

Span *PageCache::NewSpan(size_t k) {
    assert(k > 0 && k < NPAGES);
    if (!_spanLists[k].Empty()) {
        return _spanLists[k].PopFront();
    }

    // 检查后面的桶有没有span，有的话进行切分
    for (size_t i = k + 1; i < NPAGES; ++i) {
        if (!_spanLists[i].Empty()) {
            Span *nSpan = _spanLists[i].PopFront();
            Span *kSpan = new Span;

            // 切分,从头切
            // kSpan返回，剩余的挂到n-k的spanlist里面
            kSpan->_pageId = nSpan->_pageId;
            kSpan->_n = k;

            nSpan->_pageId += k;
            nSpan->_n -= k;

            _spanLists[nSpan->_n].PushFront(nSpan);
            // 这里得存nSpan的首尾pageid对应的span，以便于后面合并页使用
            _idSpanMap[nSpan->_pageId] = nSpan;
            _idSpanMap[nSpan->_pageId + nSpan->_n - 1] = nSpan;

            // 分配给centralcache的k页span的页号映射好对应位置
            for (PageId i = 0; i < kSpan->_n; ++i) {
                _idSpanMap[kSpan->_pageId + i] = kSpan;
            }

            return kSpan;
        }
    }

    // 走到这里说明后面没有大页的Span
    // 找堆要128页的span
    Span *bigSpan = new Span;
    void *ptr = SystemAlloc(NPAGES - 1);
    bigSpan->_pageId = (PageId) ptr >> PAGE_SHIFT;
    bigSpan->_n = NPAGES - 1;

    _spanLists[bigSpan->_n].PushFront(bigSpan);
    return NewSpan(k);
}

Span *PageCache::MapObjectToSpan(void *obj) {
    PageId id = ((PageId) obj >> PAGE_SHIFT);
    auto ret = _idSpanMap.find(id);
    if (ret != _idSpanMap.end()) {
        return ret->second;// 返回位置
    } else {
        assert(false);
//        return nullptr;
    }
}

void PageCache::ReleaseSpanToPageCache(Span *span) {
    // 往前找是否有空闲的页可以合并
    while (1) {
        PageId prevId = span->_pageId - 1;
        auto ret = _idSpanMap.find(prevId);

        // 没找到
        if (ret == _idSpanMap.end()) {
            break;
        }

        // centralcache用掉了
        Span *prevSpan = ret->second;
        if (prevSpan->_isUse) {
            break;
        }

        // 合并超过Pagecache可以管理的大小
        if (span->_n + prevSpan->_n > NPAGES - 1) {
            break;
        }

        // 可以合并了
        span->_pageId = prevSpan->_pageId;
        span->_n += prevSpan->_n;

        // 移除pagecache的prevSpan
        _spanLists[prevSpan->_n].Erase(prevSpan);
        delete prevSpan;
    }
    // 往后找是否有空闲的页可以合并
    while (1) {
        PageId postId = span->_pageId + span->_n;
        auto ret = _idSpanMap.find(postId);

        // 没找到
        if (ret == _idSpanMap.end()) {
            break;
        }

        // centralcache用掉了
        Span *postSpan = ret->second;
        if (postSpan->_isUse) {
            break;
        }

        // 合并超过Pagecache可以管理的大小
        if (span->_n + postSpan->_n > NPAGES - 1) {
            break;
        }

        // 可以合并了
        span->_n += postSpan->_n;

        // 移除pagecache的prevSpan
        _spanLists[postSpan->_n].Erase(postSpan);
        delete postSpan;
    }
    _spanLists[span->_n].PushFront(span);
    span->_isUse = false;
    _idSpanMap[span->_pageId] = span;
    _idSpanMap[span->_pageId + span->_n - 1] = span;
}


