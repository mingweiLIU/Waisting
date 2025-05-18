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
     * @brief ����jemalloc��TBB���ڴ��ʵ��
     *
     * ����ڴ��ʹ��jemalloc��Ϊ�ײ��ڴ����������ʹ��TBB�Ĳ��������������ڴ�顣
     * ��֧�ֲ�ͬ��С���ڴ����䣬��ͨ���ڴ�鸴����������ܡ�
     */
    class MemoryPool {
    public:
        static MemoryPool* GetInstance(std::string poolName) {
            auto it= namedMemoryPools.find(poolName);
            if (it != namedMemoryPools.end()) {
                return it->second;
            }
            else {
                //�½�һ��
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
         * @brief ���캯��
         * @param minBlockSize ��С�ڴ���С
         * @param maxBlockSize ����ڴ���С�������˴�С���ڴ�齫ֱ�ӷ��䲻���гػ���
         * @param blockSizeStep �ڴ���С�Ĳ���ֵ
         */
        MemoryPool(size_t minBlockSize = 8, size_t maxBlockSize = 4096, size_t blockSizeStep = 8)
            : m_minBlockSize(minBlockSize), m_maxBlockSize(maxBlockSize), m_blockSizeStep(blockSizeStep) {

            // ��ʼ���ڴ���
            for (size_t size = m_minBlockSize; size <= m_maxBlockSize; size += m_blockSizeStep) {
                m_pools[size] = ::tbb::concurrent_queue<void*>();
            }
        }

        /**
         * @brief �����������ͷ�����δ�黹���ڴ��
         */
        ~MemoryPool() {
            // �ͷ������ڴ���е��ڴ��
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
         * @brief ����ָ����С���ڴ��
         * @param size ������ڴ���С
         * @return ������ڴ��ָ��
         */
        void* allocate(size_t size) {
            // ������Ĵ�С���뵽���ʵ��ڴ���С
            size_t alignedSize = alignSize(size);

            // ���������ڴ���С�������ػ��ڴ���С����ֱ��ʹ��jemalloc����
            if (alignedSize > m_maxBlockSize) {
                void* ptr = je_malloc(size);
                if (ptr) {
                    m_allocatedLargeBlocks[ptr] = size;
                }
                return ptr;
            }

            // ���ԴӶ�Ӧ��С���ڴ���л�ȡ�ڴ��
            void* ptr = nullptr;
            auto it = m_pools.find(alignedSize);
            if (it != m_pools.end() && it->second.try_pop(ptr)) {
                // �ҵ��ɸ��õ��ڴ��
                return ptr;
            }

            // �ڴ����û�п��õ��ڴ�飬ʹ��jemallocֱ�ӷ���
            return je_malloc(size);
        }

        /**
         * @brief �ͷ��ڴ��
         * @param ptr Ҫ�ͷŵ��ڴ��ָ��
         * @param size �ڴ��Ĵ�С
         */
        void deallocate(void* ptr, size_t size) {
            if (!ptr) return;

            // ����Ƿ��Ǵ��ڴ��
            auto largeIt = m_allocatedLargeBlocks.find(ptr);
            if (largeIt != m_allocatedLargeBlocks.end()) {
                je_free(ptr);
                m_allocatedLargeBlocks.unsafe_erase(largeIt);
                return;
            }

            // С�ڴ�飬����Żض�Ӧ���ڴ����
            size_t alignedSize = alignSize(size);
            if (alignedSize <= m_maxBlockSize) {
                auto it = m_pools.find(alignedSize);
                if (it != m_pools.end()) {
                    it->second.push(ptr);
                    return;
                }
            }

            // ������������ڴ�ص��ڴ�飬��ֱ���ͷ�
            je_free(ptr);
        }

        /**
         * @brief ��ȡ��ǰ�ڴ���е��ڴ������
         * @return �ڴ���е��ڴ������
         */
        size_t getPoolSize() const {
            size_t totalSize = 0;
            for (const auto& pair : m_pools) {
                totalSize += pair.second.unsafe_size();
            }
            return totalSize;
        }

        /**
         * @brief �����������T�Ķ���
         * @tparam T ��������
         * @tparam Args ���캯����������
         * @param args ���캯������
         * @return ����Ķ���ָ��
         */
        template<typename T, typename... Args>
        T* allocateObject(Args&&... args) {
            void* memory = allocate(sizeof(T));
            if (!memory) return nullptr;
            return new(memory) T(std::forward<Args>(args)...);
        }

        /**
         * @brief �ͷž�������T�Ķ���
         * @tparam T ��������
         * @param ptr ����ָ��
         */
        template<typename T>
        void deallocateObject(T* ptr) {
            if (!ptr) return;
            ptr->~T();
            deallocate(ptr, sizeof(T));
        }

    private:
        /**
         * @brief ������Ĵ�С���뵽���ʵ��ڴ���С
         * @param size ������ڴ���С
         * @return �������ڴ���С
         */
        size_t alignSize(size_t size) {
            if (size < m_minBlockSize) {
                return m_minBlockSize;
            }

            // ����С���϶��뵽 m_blockSizeStep �ı���
            size_t remainder = size % m_blockSizeStep;
            if (remainder != 0) {
                size = size + m_blockSizeStep - remainder;
            }

            return size;
        }

    private:
        // ��С�ڴ���С
        size_t m_minBlockSize;
        // ����ڴ���С�������˴�С���ڴ�齫ֱ�ӷ��䲻���гػ���
        size_t m_maxBlockSize;
        // �ڴ���С�Ĳ���ֵ
        size_t m_blockSizeStep;

        // �ڴ�أ�keyΪ�ڴ���С��valueΪ�ڴ�����
        ::tbb::concurrent_unordered_map<size_t, ::tbb::concurrent_queue<void*>> m_pools;

        // ��¼���ڴ������������������ʱ�ͷ�
        ::tbb::concurrent_unordered_map<void*, size_t> m_allocatedLargeBlocks;

    private:
        inline static std::map<std::string, MemoryPool*> namedMemoryPools;
    };
   //std::map<std::string, MemoryPool*> MemoryPool::namedMemoryPools;

    ///**
    // * @brief ʹ��ʾ��
    // */
    //void memoryPoolExample() {
    //    // �����ڴ�أ���С��8�ֽڣ�����4096�ֽڣ�����8�ֽ�
    //    MemoryPool pool(8, 4096, 8);

    //    // ���䲻ͬ��С���ڴ��
    //    void* ptr1 = pool.allocate(64);
    //    void* ptr2 = pool.allocate(128);
    //    void* ptr3 = pool.allocate(256);
    //    void* ptr4 = pool.allocate(8192); // �����ֱ��ʹ��je_malloc���䣬��Ϊ�����������С

    //    std::cout << "�ڴ���е��ڴ������: " << pool.getPoolSize() << std::endl;

    //    // ʹ�÷�����ڴ�
    //    strcpy((char*)ptr1, "�����ַ���1");
    //    strcpy((char*)ptr2, "�����ַ���2");
    //    strcpy((char*)ptr3, "�����ַ���3");
    //    strcpy((char*)ptr4, "�����ַ���4");

    //    std::cout << "ptr1: " << (char*)ptr1 << std::endl;
    //    std::cout << "ptr2: " << (char*)ptr2 << std::endl;
    //    std::cout << "ptr3: " << (char*)ptr3 << std::endl;
    //    std::cout << "ptr4: " << (char*)ptr4 << std::endl;

    //    // �黹�ڴ�鵽����
    //    pool.deallocate(ptr1, 64);
    //    pool.deallocate(ptr2, 128);
    //    pool.deallocate(ptr3, 256);
    //    pool.deallocate(ptr4, 8192);

    //    std::cout << "�黹���ڴ���е��ڴ������: " << pool.getPoolSize() << std::endl;

    //    // �������ͷŶ���ʾ��
    //    struct TestStruct {
    //        int value;
    //        std::string name;

    //        TestStruct(int v, const std::string& n) : value(v), name(n) {
    //            std::cout << "���� TestStruct: " << name << " ֵ: " << value << std::endl;
    //        }

    //        ~TestStruct() {
    //            std::cout << "���� TestStruct: " << name << std::endl;
    //        }
    //    };

    //    // ���䲢�������
    //    TestStruct* obj1 = pool.allocateObject<TestStruct>(1, "����1");
    //    TestStruct* obj2 = pool.allocateObject<TestStruct>(2, "����2");

    //    // ʹ�ö���
    //    std::cout << "obj1: " << obj1->name << ", ֵ: " << obj1->value << std::endl;
    //    std::cout << "obj2: " << obj2->name << ", ֵ: " << obj2->value << std::endl;

    //    // �������ͷŶ���
    //    pool.deallocateObject(obj1);
    //    pool.deallocateObject(obj2);

    //    std::cout << "�����ͷź��ڴ���е��ڴ������: " << pool.getPoolSize() << std::endl;

    //    // ��������������ͷ�
    //    std::vector<void*> pointers;
    //    for (int i = 0; i < 1000; ++i) {
    //        pointers.push_back(pool.allocate(64));
    //    }

    //    std::cout << "����������ڴ���е��ڴ������: " << pool.getPoolSize() << std::endl;

    //    // �ͷ�һ����ڴ��
    //    for (size_t i = 0; i < pointers.size() / 2; ++i) {
    //        pool.deallocate(pointers[i], 64);
    //    }

    //    std::cout << "�ͷ�һ���ڴ����ڴ���е��ڴ������: " << pool.getPoolSize() << std::endl;

    //    // �ٴη��䣬Ӧ�û�����ʹ���Ѿ��ͷŵ��ڴ��
    //    for (size_t i = 0; i < pointers.size() / 2; ++i) {
    //        pointers[i] = pool.allocate(64);
    //    }

    //    std::cout << "�ٴη�����ڴ���е��ڴ������: " << pool.getPoolSize() << std::endl;

    //    // �ͷ������ڴ��
    //    for (auto ptr : pointers) {
    //        pool.deallocate(ptr, 64);
    //    }

    //    std::cout << "�����ڴ���е��ڴ������: " << pool.getPoolSize() << std::endl;
    //}

    ///**
    // * @brief ������
    // */
    //int main() {
    //    std::cout << "��ʼ�ڴ�ز���..." << std::endl;
    //    memoryPoolExample();
    //    std::cout << "�ڴ�ز������." << std::endl;
    //    return 0;
    //}

};