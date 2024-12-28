#include "ThreadCache.h"


void *ThreadCache::Allocate(size_t size)
{
    assert(size <= MAX_BYTES);
    size_t alignSize = SizeClass::RoundUp(size); // 确定对齐后的要申请的空间大小
    size_t index = SizeClass::Index(size);       // 获取哈希桶位置
    if (!_freeLists[index].Empty())
    {
        return _freeLists[index].Pop();
    }
    else
    {
        // ThreadCache没有该大小的内存对象
        return FetchFromCentarlCache(index, alignSize);
    }
}
void ThreadCache::DeAllocate(void *ptr, size_t size)
{
    assert(ptr);
    assert(size <=MAX_BYTES);
    size_t index = SizeClass::Index(size);       // 获取哈希桶位置
    //插入到哈希桶
    _freeLists[index].Push(ptr);
}

void* ThreadCache::FetchFromCentarlCache(size_t index, size_t size)
{
    return nullptr;
}


