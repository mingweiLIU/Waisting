// 文件缓冲区管理，用于批量写入文件
#pragma  once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <cmath>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <fcntl.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <sstream>

// TBB库
#include <tbb/task_group.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/global_control.h>
#include <tbb/concurrent_queue.h>

namespace fs = std::filesystem;
namespace WT {
    /**
 * @brief 文件缓冲区管理，用于批量写入文件
 */
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

        tbb::concurrent_queue<FileBuffer> buffer_queue;
        std::atomic<bool> running;
        std::thread writer_thread;
        size_t max_queue_size;
        std::atomic<size_t> bytes_written;
        std::atomic<size_t> files_written;

        void writer_function();

    public:
        FileBufferManager(size_t queue_size = 5000)
            : running(true), max_queue_size(queue_size), bytes_written(0), files_written(0) {
            writer_thread = std::thread(&FileBufferManager::writer_function, this);
        }

        ~FileBufferManager();

        void write_file(const std::string& path, std::vector<unsigned char>&& data);

        // 获取写入字节数
        size_t get_bytes_written() const { return bytes_written;}

        // 获取写入文件数
        size_t get_files_written() const {return files_written;}

        // 等待所有文件写入完成
        void wait_completion();
    };
};
