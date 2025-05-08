#include "FileBufferManager.h"
#include <filesystem>
namespace WT {

	void FileBufferManager::writer_function()
	{
		FileBuffer buffer;
		while (running || !buffer_queue.empty()) {
			if (buffer_queue.try_pop(buffer) && buffer.ready) {
				// ȷ��Ŀ¼����
				fs::path dir_path = fs::path(buffer.path).parent_path();
				if (!fs::exists(dir_path)) {
					try {
						fs::create_directories(dir_path);
					}
					catch (const std::exception& e) {
						std::cerr << "����Ŀ¼ʧ��: " << dir_path << " - " << e.what() << std::endl;
						continue;
					}
				}

				// д���ļ�
				try {
					std::ofstream file(buffer.path, std::ios::binary);
					if (file) {
						file.write(reinterpret_cast<const char*>(buffer.data.data()), buffer.data.size());
						file.close();

						// ����ͳ����Ϣ
						bytes_written += buffer.data.size();
						files_written++;
					}
					else {
						std::cerr << "�޷����ļ�����д��: " << buffer.path << std::endl;
					}
				}
				catch (const std::exception& e) {
					std::cerr << "д���ļ�ʧ��: " << buffer.path << " - " << e.what() << std::endl;
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
		// ������й��󣬵ȴ�һ��ʱ��
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