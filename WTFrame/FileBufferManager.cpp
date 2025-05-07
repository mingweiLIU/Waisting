#include "FileBufferManager.h"

void FileBufferManager::writer_function()
{
	FileBuffer buffer;
	while (running || !buffer_queue.empty()) {
		if (buffer_queue.try_pop(buffer) && buffer.ready) {
			std::ofstream file(buffer.path, std::ios::binary);
			if (file) {
				file.write(reinterpret_cast<const char*>(buffer.data.data()), buffer.data.size());
			}
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

FileBufferManager::FileBufferManager(size_t queue_size /*= 1000*/)
	: running(true), max_queue_size(queue_size) {
	writer_thread = std::thread(&FileBufferManager::writer_function, this);
}

FileBufferManager::~FileBufferManager()
{
	running = false;
	if (writer_thread.joinable()) {
		writer_thread.join();
	}
}

void FileBufferManager::write_file(const std::string& path, std::vector<unsigned char>&& data)
{
	// 如果队列过大，等待一段时间
	while (buffer_queue.unsafe_size() > max_queue_size) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	buffer_queue.push(FileBuffer(path, std::move(data)));
}