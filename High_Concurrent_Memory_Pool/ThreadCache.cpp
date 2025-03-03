#include "ThreadCache.h"
#include "CentralCache.h"
#include <algorithm>

void *ThreadCache::Allocate(size_t size) {
    assert(size <= MAX_BYTES);
    size_t alignSize = SizeClass::RoundUp(size); // 确定对齐后的要申请的空间大小
    size_t index = SizeClass::Index(size);       // 获取哈希桶位置
    if (!_freeLists[index].Empty()) {
//        if(_freeLists[index]._freeList == nullptr){
//            int x = 0;
//        }
        return _freeLists[index].Pop();
    } else {
        // ThreadCache没有该大小的内存对象
        return FetchFromCentralCache(index, alignSize);
    }
}

void ThreadCache::ListTooLong(FreeList &list, size_t size) {
    void *start = nullptr;
    void *end = nullptr;
    list.PopRange(start, end, list.MaxSize());
    CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}

void ThreadCache::DeAllocate(void *ptr, size_t size) {
    assert(ptr);
    assert(size <= MAX_BYTES);
    size_t index = SizeClass::Index(size); // 获取哈希桶位置
    // 插入到哈希桶
    _freeLists[index].Push(ptr);

    // 如果此时threadcache的当前自由链表的内存个数超过一定的批量，就还一些给centralcache
    if (_freeLists[index].Size() >= _freeLists[index].MaxSize()) {
        ListTooLong(_freeLists[index], size);
    }

}

void *ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
    // 慢开始反馈算法
    // 1.最开始不会找centralcache一次批量要太多，可能用不完
    // 2.如果不断有size大小的需求，那么batchNum会增长，直到上限
    // 3.如果size越大，一次找centralcache要的batchNum越小，但是有下限
    // 3.如果size越小，一次找centralcache要的batchNum越大，但是有上限
    size_t batchNum = std::min(_freeLists[index].MaxSize(),
                               SizeClass::NumMoveSize(size)); // 从centralcache要的size大小的内存个数,取小的
    if (_freeLists[index].MaxSize() == batchNum) {
        _freeLists[index].MaxSize() *= 3; // 自增，慢开始
    }
    void *start = nullptr;
    void *end = nullptr;
    size_t acturalNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size); // 实际给的个数

    assert(acturalNum > 0);

    if (acturalNum == 1) {
        assert(start == end);
        return start;
    } else {
        _freeLists[index].PushRange(NextObj(start), end, acturalNum - 1);
        return start;
    }
}
