#pragma once

#include <iostream>
#include <assert.h>

const size_t MAX_BYTES = 256 * 1024; // ThreadCache能申请的最大空间256KB

// 取对象的前4/8个字节的位置
void *&NextObj(void *obj)
{
    return *(void **)obj;
}

// 管理自由链表
class FreeList
{
public:
    void Push(void *obj)
    {
        assert(obj);
        // 可能是32位，有可能是64位
        // 头插
        NextObj(obj) = _freeList; // 内存空间的4字节或者8字节直接存指针
        _freeList = obj;
    }
    void Pop()
    {
        assert(_freeList);
        // 头删
        void *obj = _freeList;
        _freeList = NextObj(obj);
    }

private:
    void *_freeList;
};

// 计算对象大小的对齐映射规则
class SizeClass
{
public:
    static size_t _RoundUp(size_t size, size_t alignNum)
    {
        return ((size + alignNum - 1) & ~(alignNum - 1));
    }

    static size_t RoundUp(size_t size)
    {
        // 整体控制在10%左右的空间浪费
        // [1,128]                  8byte对齐               freeList[0,16)
        // [128+1,1024]             16byte对齐              freeList[16,72)
        // [1024+1,8*1024]          128byte对齐             freeList[72,128)
        // [8*1024+1,64*1024]       1024byte对齐            freeList[128,184)
        // [64*1024+1,256*1024]     8*1024byte对齐          freeList[184,208)
        if (size <= 128)
        {
            _RoundUp(size, 8);
        }
        else if (size <= 1024)
        {
            _RoundUp(size, 16);
        }
        else if (size <= 8 * 1024)
        {
            _RoundUp(size, 128);
        }
        else if (size <= 64 * 1024)
        {
            _RoundUp(size, 1024);
        }
        else if (size <= 256 * 1024)
        {
            _RoundUp(size, 8 * 1024);
        }
        else
        {
            return -1;
        }
    }

private:
};