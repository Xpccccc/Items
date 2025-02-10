#pragma once

#include <iostream>
#include <assert.h>
#include <mutex>

static const size_t MAX_BYTES = 256 * 1024; // ThreadCache能申请的最大空间256KB
static const size_t NFREE_LIST = 208;       // 自由链表个数
static const size_t NPAGES = 129;           // PageCache里最大的页个数
static const size_t PAGE_SHIFT = 13;        // P一页的大小是2^PAGE_SHIFT字节，即8K

#ifdef _WIN64
typedef unsigned long long PageId; // 64-bit Windows系统
#elif _WIN32
typedef size_t PageId; // 32-bit Windows系统
#elif __linux__
typedef unsigned long PageId; // Linux系统，可以使用 unsigned long 或 size_t，具体取决于你的系统架构
#else
#error "Unknown platform"
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <stdexcept> // for std::bad_alloc

inline static void *SystemAlloc(size_t kpage)
{
#ifdef _WIN32
    void *ptr = VirtualAlloc(0, kpage * 8 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    // Linux
    void *ptr = mmap(nullptr, kpage << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
    {
        // 错误处理
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
#endif

    if (ptr == nullptr)
    {
        throw std::bad_alloc();
    }
    return ptr;
}

// 取对象的前4/8个字节的位置
static void *&NextObj(void *obj)
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
    void *Pop()
    {
        assert(_freeList);
        // 头删
        void *obj = _freeList;
        _freeList = NextObj(obj);
        return obj;
    }

    // 头插一段范围内存
    void PushRange(void *start, void *end)
    {
        NextObj(end) = _freeList;
        _freeList = start;
    }
    bool Empty()
    {
        return _freeList == nullptr;
    }

    size_t &MaxSize()
    {
        return _maxSize;
    }

private:
    void *_freeList;
    size_t _maxSize = 1; // 每个桶都维护一个上一次要该内存大小的个数
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
        return -2;
    }

    static size_t _Index(size_t bytes, size_t align_shift)
    {
        return ((bytes + (1 << align_shift - 1) >> align_shift) - 1);
    }

    static size_t Index(size_t bytes)
    {
        // 计算在哪个大小的哈希桶
        int group[] = {16, 56, 56, 56};
        if (bytes <= 128)
        {
            return _Index(bytes, 3);
        }
        else if (bytes <= 1024)
        {
            return _Index(bytes - 128, 4) + group[0]; // 16号之前的都用掉了
        }
        else if (bytes <= 8 * 1024)
        {
            return _Index(bytes - 1024, 7) + group[0] + group[1];
        }
        else if (bytes <= 64 * 1024)
        {
            return _Index(bytes - 8 * 1024, 10) + group[0] + group[1] + group[2];
        }
        else if (bytes <= 256 * 1024)
        {
            return _Index(bytes - 64 * 1024, 13) + group[0] + group[1] + group[2] + group[3];
        }
        else
        {
            return -1;
        }
    }

    // 一次threadcache从centralcache中要多少个
    static size_t NumMoveSize(size_t size)
    {
        assert(size > 0);
        // [2,512],下限2，上限512

        int num = MAX_BYTES / size;
        if (num < 2)
        {
            num = 2;
        }
        if (num > 512)
        {
            num = 512;
        }
        return num;
    }

    // 一次centalcache向pagecache要的页数
    static size_t NumMovePage(size_t size)
    {
        size_t num = NumMoveSize(size);       // 先算本来要的size的个数
        size_t nbytes = num * size;           // 要的总大小
        size_t npages = nbytes >> PAGE_SHIFT; // PAGE_SHIFT即一页的大小是2^PAGE_SHIFT字节
        if (npages == 0)
            npages = 1;
        return npages;
    }
};

// 管理多个连续页大块内存跨度结构
struct Span
{
    PageId _pageId = 0; // 页号
    size_t _n = 0;      // 页的数量

    // 双向循环链表结构
    Span *_next = nullptr;
    Span *_prev = nullptr;

    size_t _useCount = 0;      // 切好的小块内存，分配给ThreadCache的个数
    void *_freeList = nullptr; // 切好的小块内存的自由链表
};

// 带头双向循环链表
class SpanList
{
public:
    SpanList()
    {
        _head = new Span;
        _head->_next = _head;
        _head->_next = _head;
    }

    void PushFront(Span *span)
    {
        Insert(Begin(), span);
    }

    Span *Begin()
    {
        return _head->_next;
    }

    Span *End()
    {
        return _head;
    }

    bool Empty()
    {
        return _head->_next == _head;
    }

    Span *PopFront()
    {
        Span *front = _head->_next;
        Erase(front);
        return front;
    }

    void Insert(Span *pos, Span *newSpan)
    {
        assert(pos);
        assert(newSpan);

        // prev newSpan pos
        Span *prev = pos->_prev;

        prev->_next = newSpan;
        newSpan->_prev = prev;
        newSpan->_next = pos;
        pos->_prev = newSpan;
    }

    void Erase(Span *pos)
    {
        assert(pos);
        assert(pos != _head);

        Span *prev = pos->_prev;
        Span *next = pos->_next;

        prev->_next = next;
        next->_prev = prev;
    }

private:
    Span *_head;

public:
    std::mutex _mtx; // 桶锁
};