#pragma once

#include "ThreadCache.h"

void *ThreadCache::Allocate(size_t size)
{
    assert(size <= MAX_BYTES);
    size_t alignSize = SizeClass::RoundUp(size); // 确定对齐后的要申请的空间大小
    
}
void ThreadCache::DeAllocate(void *ptr, size_t size)
{
}