// 文件缓冲区管理，用于批量写入文件
#pragma  once
#include <string>
#include <atomic>
#include <vector>
#include <iostream>
#include <fstream>

// TBB库
#include <tbb/task_group.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/global_control.h>
#include <tbb/concurrent_queue.h>

class FileBufferManager {
private:
	struct FileBuffer {
		std::string path;
		std::vector<unsigned char> data;
		bool ready;

		FileBuffer() : ready(false) {}
		FileBuffer(const std::string& p, std::vector<unsigned char>&& d)
			: path(p), data(std::move(d)), ready(true) {}
	};

public:
	FileBufferManager(size_t queue_size = 1000);
	~FileBufferManager();
	void write_file(const std::string& path, std::vector<unsigned char>&& data);

private:
	tbb::concurrent_queue<FileBuffer> buffer_queue;
	std::atomic<bool> running;
	std::thread writer_thread;
	size_t max_queue_size;

	void writer_function();
};
