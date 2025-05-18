#pragma once
#include <iostream>
#include <jemalloc/jemalloc.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <mutex>
#include <vector>
#include <cassert>
#include <functional>
#include <map>

namespace WT {
    /**
     * @brief 基于jemalloc和TBB的内存池实现
     *
     * 这个内存池使用jemalloc作为底层内存分配器，并使用TBB的并发容器来管理内存块。
     * 它支持不同大小的内存块分配，并通过内存块复用来提高性能。
     */
    class MemoryPool {
    public:
        static MemoryPool* GetInstance(std::string poolName) {
            auto it= namedMemoryPools.find(poolName);
            if (it != namedMemoryPools.end()) {
                return it->second;
            }
            else {
                //新建一个
                MemoryPool* oneMPool = new MemoryPool();
                namedMemoryPools.insert(std::make_pair(poolName,oneMPool));
                return oneMPool;
            }
        }
        static void releaseInstance(std::string poolName) {
            auto it = namedMemoryPools.find(poolName);
            if (it != namedMemoryPools.end()) {
                delete it->second;
                namedMemoryPools.erase(it);
            }
        }

    protected:
        /**
         * @brief 构造函数
         * @param minBlockSize 最小内存块大小
         * @param maxBlockSize 最大内存块大小（超过此大小的内存块将直接分配不进行池化）
         * @param blockSizeStep 内存块大小的步进值
         */
        MemoryPool(size_t minBlockSize = 8, size_t maxBlockSize = 4096, size_t blockSizeStep = 8)
            : m_minBlockSize(minBlockSize), m_maxBlockSize(maxBlockSize), m_blockSizeStep(blockSizeStep) {

            // 初始化内存块池
            for (size_t size = m_minBlockSize; size <= m_maxBlockSize; size += m_blockSizeStep) {
                m_pools[size] = ::tbb::concurrent_queue<void*>();
            }
        }

        /**
         * @brief 析构函数，释放所有未归还的内存块
         */
        ~MemoryPool() {
            // 释放所有内存池中的内存块
            for (auto& pair : m_pools) {
                void* ptr = nullptr;
                while (pair.second.try_pop(ptr)) {
                    if (ptr) {
                        je_free(ptr);
                    }
                }
            }
        }
    public:
        /**
         * @brief 分配指定大小的内存块
         * @param size 请求的内存块大小
         * @return 分配的内存块指针
         */
        void* allocate(size_t size) {
            // 将请求的大小对齐到合适的内存块大小
            size_t alignedSize = alignSize(size);

            // 如果请求的内存块大小超过最大池化内存块大小，则直接使用jemalloc分配
            if (alignedSize > m_maxBlockSize) {
                void* ptr = je_malloc(size);
                if (ptr) {
                    m_allocatedLargeBlocks[ptr] = size;
                }
                return ptr;
            }

            // 尝试从对应大小的内存池中获取内存块
            void* ptr = nullptr;
            auto it = m_pools.find(alignedSize);
            if (it != m_pools.end() && it->second.try_pop(ptr)) {
                // 找到可复用的内存块
                return ptr;
            }

            // 内存池中没有可用的内存块，使用jemalloc直接分配
            return je_malloc(size);
        }

        /**
         * @brief 释放内存块
         * @param ptr 要释放的内存块指针
         * @param size 内存块的大小
         */
        void deallocate(void* ptr, size_t size) {
            if (!ptr) return;

            // 检查是否是大内存块
            auto largeIt = m_allocatedLargeBlocks.find(ptr);
            if (largeIt != m_allocatedLargeBlocks.end()) {
                je_free(ptr);
                m_allocatedLargeBlocks.unsafe_erase(largeIt);
                return;
            }

            // 小内存块，将其放回对应的内存池中
            size_t alignedSize = alignSize(size);
            if (alignedSize <= m_maxBlockSize) {
                auto it = m_pools.find(alignedSize);
                if (it != m_pools.end()) {
                    it->second.push(ptr);
                    return;
                }
            }

            // 如果不是来自内存池的内存块，则直接释放
            je_free(ptr);
        }

        /**
         * @brief 获取当前内存池中的内存块数量
         * @return 内存池中的内存块数量
         */
        size_t getPoolSize() const {
            size_t totalSize = 0;
            for (const auto& pair : m_pools) {
                totalSize += pair.second.unsafe_size();
            }
            return totalSize;
        }

        /**
         * @brief 分配具有类型T的对象
         * @tparam T 对象类型
         * @tparam Args 构造函数参数类型
         * @param args 构造函数参数
         * @return 分配的对象指针
         */
        template<typename T, typename... Args>
        T* allocateObject(Args&&... args) {
            void* memory = allocate(sizeof(T));
            if (!memory) return nullptr;
            return new(memory) T(std::forward<Args>(args)...);
        }

        /**
         * @brief 释放具有类型T的对象
         * @tparam T 对象类型
         * @param ptr 对象指针
         */
        template<typename T>
        void deallocateObject(T* ptr) {
            if (!ptr) return;
            ptr->~T();
            deallocate(ptr, sizeof(T));
        }

    private:
        /**
         * @brief 将请求的大小对齐到合适的内存块大小
         * @param size 请求的内存块大小
         * @return 对齐后的内存块大小
         */
        size_t alignSize(size_t size) {
            if (size < m_minBlockSize) {
                return m_minBlockSize;
            }

            // 将大小向上对齐到 m_blockSizeStep 的倍数
            size_t remainder = size % m_blockSizeStep;
            if (remainder != 0) {
                size = size + m_blockSizeStep - remainder;
            }

            return size;
        }

    private:
        // 最小内存块大小
        size_t m_minBlockSize;
        // 最大内存块大小（超过此大小的内存块将直接分配不进行池化）
        size_t m_maxBlockSize;
        // 内存块大小的步进值
        size_t m_blockSizeStep;

        // 内存池，key为内存块大小，value为内存块队列
        ::tbb::concurrent_unordered_map<size_t, ::tbb::concurrent_queue<void*>> m_pools;

        // 记录大内存块分配情况，用于析构时释放
        ::tbb::concurrent_unordered_map<void*, size_t> m_allocatedLargeBlocks;

    private:
        inline static std::map<std::string, MemoryPool*> namedMemoryPools;
    };
   //std::map<std::string, MemoryPool*> MemoryPool::namedMemoryPools;

    ///**
    // * @brief 使用示例
    // */
    //void memoryPoolExample() {
    //    // 创建内存池，最小块8字节，最大块4096字节，步长8字节
    //    MemoryPool pool(8, 4096, 8);

    //    // 分配不同大小的内存块
    //    void* ptr1 = pool.allocate(64);
    //    void* ptr2 = pool.allocate(128);
    //    void* ptr3 = pool.allocate(256);
    //    void* ptr4 = pool.allocate(8192); // 这个会直接使用je_malloc分配，因为超过了最大块大小

    //    std::cout << "内存池中的内存块数量: " << pool.getPoolSize() << std::endl;

    //    // 使用分配的内存
    //    strcpy((char*)ptr1, "测试字符串1");
    //    strcpy((char*)ptr2, "测试字符串2");
    //    strcpy((char*)ptr3, "测试字符串3");
    //    strcpy((char*)ptr4, "测试字符串4");

    //    std::cout << "ptr1: " << (char*)ptr1 << std::endl;
    //    std::cout << "ptr2: " << (char*)ptr2 << std::endl;
    //    std::cout << "ptr3: " << (char*)ptr3 << std::endl;
    //    std::cout << "ptr4: " << (char*)ptr4 << std::endl;

    //    // 归还内存块到池中
    //    pool.deallocate(ptr1, 64);
    //    pool.deallocate(ptr2, 128);
    //    pool.deallocate(ptr3, 256);
    //    pool.deallocate(ptr4, 8192);

    //    std::cout << "归还后内存池中的内存块数量: " << pool.getPoolSize() << std::endl;

    //    // 创建和释放对象示例
    //    struct TestStruct {
    //        int value;
    //        std::string name;

    //        TestStruct(int v, const std::string& n) : value(v), name(n) {
    //            std::cout << "构造 TestStruct: " << name << " 值: " << value << std::endl;
    //        }

    //        ~TestStruct() {
    //            std::cout << "析构 TestStruct: " << name << std::endl;
    //        }
    //    };

    //    // 分配并构造对象
    //    TestStruct* obj1 = pool.allocateObject<TestStruct>(1, "对象1");
    //    TestStruct* obj2 = pool.allocateObject<TestStruct>(2, "对象2");

    //    // 使用对象
    //    std::cout << "obj1: " << obj1->name << ", 值: " << obj1->value << std::endl;
    //    std::cout << "obj2: " << obj2->name << ", 值: " << obj2->value << std::endl;

    //    // 析构并释放对象
    //    pool.deallocateObject(obj1);
    //    pool.deallocateObject(obj2);

    //    std::cout << "对象释放后内存池中的内存块数量: " << pool.getPoolSize() << std::endl;

    //    // 测试批量分配和释放
    //    std::vector<void*> pointers;
    //    for (int i = 0; i < 1000; ++i) {
    //        pointers.push_back(pool.allocate(64));
    //    }

    //    std::cout << "批量分配后内存池中的内存块数量: " << pool.getPoolSize() << std::endl;

    //    // 释放一半的内存块
    //    for (size_t i = 0; i < pointers.size() / 2; ++i) {
    //        pool.deallocate(pointers[i], 64);
    //    }

    //    std::cout << "释放一半内存块后内存池中的内存块数量: " << pool.getPoolSize() << std::endl;

    //    // 再次分配，应该会优先使用已经释放的内存块
    //    for (size_t i = 0; i < pointers.size() / 2; ++i) {
    //        pointers[i] = pool.allocate(64);
    //    }

    //    std::cout << "再次分配后内存池中的内存块数量: " << pool.getPoolSize() << std::endl;

    //    // 释放所有内存块
    //    for (auto ptr : pointers) {
    //        pool.deallocate(ptr, 64);
    //    }

    //    std::cout << "最终内存池中的内存块数量: " << pool.getPoolSize() << std::endl;
    //}

    ///**
    // * @brief 主函数
    // */
    //int main() {
    //    std::cout << "开始内存池测试..." << std::endl;
    //    memoryPoolExample();
    //    std::cout << "内存池测试完成." << std::endl;
    //    return 0;
    //}

};