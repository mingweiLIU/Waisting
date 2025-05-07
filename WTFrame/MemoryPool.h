#pragma once
#include <vector>
#include <memory>
#include <mutex>

class MemoryPool {
    private:
		struct Block {
			void* data;
			size_t size;
			bool used;

			Block(size_t s) : size(s), used(false) {
				data = std::malloc(s);
			}

			~Block() {
				if (data) std::free(data);
			}
		};
        
        std::vector<std::unique_ptr<Block>> blocks;
        std::mutex mutex;
        size_t default_block_size;
        
    public:
        MemoryPool(size_t default_size = 1024 * 1024) : default_block_size(default_size) {}
        
        ~MemoryPool() {
            clear();
        }
        
        void* allocate(size_t size);
        
        void deallocate(void* ptr);
        
        void clear();
    };