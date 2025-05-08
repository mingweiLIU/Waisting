#pragma once
// TBB��
#include "tbb/task_group.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"
#include "tbb/global_control.h"
#include "tbb/concurrent_queue.h"

// jemallocͷ�ļ�
#include "jemalloc/jemalloc.h"

namespace WT {
    /**
 * @brief jemalloc�ڴ�����װ��
 */
    class JemallocAllocator {
    public:
        JemallocAllocator() {
            // ����jemalloc
            // ���ú�̨�߳�
            int background_thread = 1;
            je_mallctl("background_thread", NULL, NULL, &background_thread, sizeof(background_thread));

            // �����ڴ��ͷŵ�����ϵͳ
            int enable_purge = 1;
            je_mallctl("opt.retain", NULL, NULL, &enable_purge, sizeof(enable_purge));

            // �����̻߳���
            size_t tcache_max = 16 * 1024; // 16KB
            je_mallctl("tcache.flush", NULL, NULL, NULL, 0);
            je_mallctl("tcache.max", NULL, NULL, &tcache_max, sizeof(tcache_max));
        }

        void* allocate(size_t size) {
            return je_malloc(size);
        }

        void deallocate(void* ptr) {
            if (ptr) {
                je_free(ptr);
            }
        }

        void* reallocate(void* ptr, size_t new_size) {
            return je_realloc(ptr, new_size);
        }

        // �����������䣬�ʺ�TBB���д���
        template<typename T>
        T* allocate_array(size_t count) {
            return static_cast<T*>(je_calloc(count, sizeof(T)));
        }

        // ǿ�ƻ����ڴ�
        void release_memory() {
            je_mallctl("arena.0.purge", NULL, NULL, NULL, 0);
        }
    };
};