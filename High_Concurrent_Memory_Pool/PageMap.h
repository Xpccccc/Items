//
// Created by 徐鹏 on 2025/2/16.
//

#ifndef HIGH_CONCURRENT_MEMORY_POOL_PAGEMAP_H
#define HIGH_CONCURRENT_MEMORY_POOL_PAGEMAP_H

#endif //HIGH_CONCURRENT_MEMORY_POOL_PAGEMAP_H


#pragma once
#include <cstring>
#include"Comm.h"
#include "ObjectPool.h"

// Single-level array
// 单层数组映射，用于管理固定大小的内存映射
template<int BITS> // 32-13 = 19 ， 32 位可以存2^19页
class TCMalloc_PageMap1 {
private:
    static const int LENGTH = 1 << BITS;  // 数组长度 2^19页，即页号
    void **array_;                       // 数组指针，存储void*类型的数据

public:
    typedef uintptr_t Number;            // 定义Number类型为uintptr_t（无符号整数类型）

    // 构造函数，初始化数组
    explicit TCMalloc_PageMap1() {
        size_t size = sizeof(void *) << BITS;  // 计算数组所需的内存大小
        size_t alignSize = SizeClass::_RoundUp(size, 1 << PAGE_SHIFT); // 对齐内存大小
        array_ = (void **) SystemAlloc(alignSize >> PAGE_SHIFT); // 调用SystemAlloc分配内存
        memset(array_, 0, sizeof(void *) << BITS); // 初始化数组为0
    }

    // 获取键k对应的值，如果k超出范围或未设置，返回NULL
    void *get(Number k) const {
        if ((k >> BITS) > 0) {  // 检查k是否超出范围
            return nullptr;
        }
        return array_[k];       // 返回数组中k对应的值
    }

    // 设置键k的值为v，要求k必须在[0, 2^BITS-1]范围内
    void set(Number k, void *v) {
        array_[k] = v;          // 将数组中k对应的值设置为v
    }
};


// Two-level radix tree
// 两层基数树，用于管理更大的内存映射
template<int BITS> // 32 - 12 = 20
class TCMalloc_PageMap2 {
private:
    static const int ROOT_BITS = 5;              // 根节点的位数
    static const int ROOT_LENGTH = 1 << ROOT_BITS; // 根节点的长度（32）

    static const int LEAF_BITS = BITS - ROOT_BITS; // 叶子节点的位数 20 - 5 = 15
    static const int LEAF_LENGTH = 1 << LEAF_BITS; // 叶子节点的长度 2^15

    // 叶子节点结构
    struct Leaf {
        void *values[LEAF_LENGTH]; // 存储实际数据的数组
    };

    Leaf *root_[ROOT_LENGTH];      // 根节点数组，指向叶子节点

public:
    typedef uintptr_t Number;      // 定义Number类型为uintptr_t

    // 构造函数，初始化根节点
    explicit TCMalloc_PageMap2() {
        memset(root_, 0, sizeof(root_)); // 初始化根节点为NULL
        PreallocateMoreMemory();        // 预分配内存
    }

    // 获取键k对应的值，如果k超出范围或未设置，返回NULL
    void *get(Number k) const {
        const Number i1 = k >> LEAF_BITS; // 计算根节点索引
        const Number i2 = k & (LEAF_LENGTH - 1); // 计算叶子节点索引
        if ((k >> BITS) > 0 || root_[i1] == nullptr) { // 检查k是否超出范围或叶子节点未分配
            return nullptr;
        }
        return root_[i1]->values[i2]; // 返回叶子节点中对应的值
    }

    // 设置键k的值为v，要求k必须在[0, 2^BITS-1]范围内
    void set(Number k, void *v) {
        const Number i1 = k >> LEAF_BITS; // 计算根节点索引
        const Number i2 = k & (LEAF_LENGTH - 1); // 计算叶子节点索引
        assert(i1 < ROOT_LENGTH);         // 确保根节点索引有效
        root_[i1]->values[i2] = v;        // 设置叶子节点中对应的值
    }

    // 确保从start开始的n个键都有对应的叶子节点
    bool Ensure(Number start, size_t n) {
        for (Number key = start; key <= start + n - 1;) {
            const Number i1 = key >> LEAF_BITS; // 计算根节点索引

            // 检查是否溢出
            if (i1 >= ROOT_LENGTH)
                return false;

            // 如果叶子节点未分配，则分配
            if (root_[i1] == nullptr) {
                static ObjectPool<Leaf> leafPool; // 使用对象池分配叶子节点
                Leaf *leaf = (Leaf *) leafPool.New(); // 从对象池中获取叶子节点
                memset(leaf, 0, sizeof(*leaf));  // 初始化叶子节点
                root_[i1] = leaf;                // 将叶子节点挂载到根节点
            }

            // 跳过当前叶子节点覆盖的范围
            key = ((key >> LEAF_BITS) + 1) << LEAF_BITS;
        }
        return true;
    }

    // 预分配内存，确保所有可能的键都有对应的叶子节点
    void PreallocateMoreMemory() {
        Ensure(0, 1 << BITS); // 确保所有键都有叶子节点
    }
};

// 三级基数树
template<int BITS> // 64 - 12 = 52
class TCMalloc_PageMap3 {
private:
    // 在每一层内部节点消耗的位数
    static const int INTERIOR_BITS = (BITS + 2) / 3; // 向上取整 （54）/3 = 18
    static const int INTERIOR_LENGTH = 1 << INTERIOR_BITS; // 内部节点的长度 2^18

    // 在叶子节点消耗的位数
    static const int LEAF_BITS = BITS - 2 * INTERIOR_BITS; // 叶子节点的位数 52 - 2 * 18 = 52 - 36 = 16,就是说有两层18的
    static const int LEAF_LENGTH = 1 << LEAF_BITS; // 叶子节点的长度 2^16

    // 内部节点结构
    struct Node {
        Node *ptrs[INTERIOR_LENGTH]; // 指向子节点的指针数组
    };

    // 叶子节点结构
    struct Leaf {
        void *values[LEAF_LENGTH]; // 存储值的数组
    };

    Node *root_;                          // 基数树的根节点

    // 创建一个新的内部节点
    Node *NewNode() {
        static ObjectPool<Node> newPool;
        Node *result = newPool.New();
        if (result != NULL) {
            memset(result, 0, sizeof(Node)); // 初始化内存为0
        }
        assert(result);
        return result;
    }

public:
    typedef uintptr_t Number; // 定义Number类型为uintptr_t

    // 构造函数，初始化内存分配器和根节点
    explicit TCMalloc_PageMap3() {
        root_ = NewNode();
    }

    // 获取键k对应的值
    void *get(Number k) const {
        const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS); // 第一层索引
        const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1); // 第二层索引
        const Number i3 = k & (LEAF_LENGTH - 1); // 第三层索引
        if ((k >> BITS) > 0 ||
            root_->ptrs[i1] == nullptr || root_->ptrs[i1]->ptrs[i2] == nullptr) {
            return nullptr; // 如果键超出范围或节点不存在，返回NULL
        }
        return reinterpret_cast<Leaf *>(root_->ptrs[i1]->ptrs[i2])->values[i3]; // 返回叶子节点中的值
    }

    // 设置键k对应的值
    void set(Number k, void *v) {
        assert(k >> BITS == 0); // 确保键k在有效范围内

        // LEAF_BITS-->16  INTERIOR_BITS-->18
        const Number i1 = k >> (LEAF_BITS + INTERIOR_BITS);
        if (root_->ptrs[i1] == nullptr)
        {
            //bool Ensure(Number start, size_t  n)
            Ensure(k, 1);
        }
        const Number i2 = (k >> LEAF_BITS) & (INTERIOR_LENGTH - 1);
        if (root_->ptrs[i1]->ptrs[i2] == nullptr)
        {
            Ensure(k, 1);
        }
        const Number i3 = k & (LEAF_LENGTH - 1);

        reinterpret_cast<Leaf *>(root_->ptrs[i1]->ptrs[i2])->values[i3] = v; // 设置叶子节点中的值
    }

    // 确保从start开始的n个键对应的节点存在
    bool Ensure(Number start, size_t n) {
        for (Number key = start; key <= start + n - 1;) {
            const Number i1 = key >> (LEAF_BITS + INTERIOR_BITS); // 第一层索引
            const Number i2 = (key >> LEAF_BITS) & (INTERIOR_LENGTH - 1); // 第二层索引

            // 检查是否溢出
            if (i1 >= INTERIOR_LENGTH || i2 >= INTERIOR_LENGTH)
                return false;

            // 如果第二层节点不存在，则创建
            if (root_->ptrs[i1] == nullptr) {
                Node *node = NewNode();
                if (node == nullptr) return false;
                root_->ptrs[i1] = node;
            }

            // 如果叶子节点不存在，则创建
            if (root_->ptrs[i1]->ptrs[i2] == nullptr) {
//                Leaf *leaf = reinterpret_cast<Leaf *>((*allocator_)(sizeof(Leaf))); // 分配内存
                static ObjectPool<Leaf> leafPool; // 使用对象池分配叶子节点
                Leaf *leaf = (Leaf *) leafPool.New(); // 从对象池中获取叶子节点
                if (leaf == nullptr) return false;
//                memset(leaf, 0, sizeof(*leaf)); // 初始化内存为0
                root_->ptrs[i1]->ptrs[i2] = reinterpret_cast<Node *>(leaf);
            }

            // 跳过当前叶子节点覆盖的键范围
            key = ((key >> LEAF_BITS) + 1) << LEAF_BITS;

        }
        return true;
    }
};