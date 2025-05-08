#include "FileBufferManager.h"
#include <filesystem>
namespace WT {

	void FileBufferManager::writer_function()
	{
		FileBuffer buffer;
		while (running || !buffer_queue.empty()) {
			if (buffer_queue.try_pop(buffer) && buffer.ready) {
				// 确保目录存在
				fs::path dir_path = fs::path(buffer.path).parent_path();
				if (!fs::exists(dir_path)) {
					try {
						fs::create_directories(dir_path);
					}
					catch (const std::exception& e) {
						std::cerr << "创建目录失败: " << dir_path << " - " << e.what() << std::endl;
						continue;
					}
				}

				// 写入文件
				try {
					std::ofstream file(buffer.path, std::ios::binary);
					if (file) {
						file.write(reinterpret_cast<const char*>(buffer.data.data()), buffer.data.size());
						file.close();

						// 更新统计信息
						bytes_written += buffer.data.size();
						files_written++;
					}
					else {
						std::cerr << "无法打开文件进行写入: " << buffer.path << std::endl;
					}
				}
				catch (const std::exception& e) {
					std::cerr << "写入文件失败: " << buffer.path << " - " << e.what() << std::endl;
				}
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
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

	void FileBufferManager::wait_completion()
	{
		while (!buffer_queue.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

};