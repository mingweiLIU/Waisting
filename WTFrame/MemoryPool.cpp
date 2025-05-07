#include "MemoryPool.h"

void* MemoryPool::allocate(size_t size)
{
	std::lock_guard<std::mutex> lock(mutex);

	// 尝试查找可重用的块
	for (auto& block : blocks) {
		if (!block->used && block->size >= size) {
			block->used = true;
			return block->data;
		}
	}

	// 创建新块
	size_t alloc_size = std::max(size, default_block_size);
	auto block = std::make_unique<Block>(alloc_size);
	if (!block->data) {
		return nullptr;
	}

	block->used = true;
	void* result = block->data;
	blocks.push_back(std::move(block));

	return result;
}

void MemoryPool::deallocate(void* ptr)
{
	if (!ptr) return;

	std::lock_guard<std::mutex> lock(mutex);
	for (auto& block : blocks) {
		if (block->data == ptr) {
			block->used = false;
			return;
		}
	}
}

void MemoryPool::clear()
{
	std::lock_guard<std::mutex> lock(mutex);
	blocks.clear();
}
