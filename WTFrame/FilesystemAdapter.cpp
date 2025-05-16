#include "FilesystemAdapter.h"
#include <fstream>
#include <unordered_map>

namespace WT {
	FilesystemAdapter::FilesystemAdapter(const std::string& basePath, bool createDirs)
		: mBasePath(basePath), mCreateDirs(createDirs) {}

	bool FilesystemAdapter::initialize() {
		if (mCreateDirs) {
			std::filesystem::create_directories(mBasePath);
		}
		return std::filesystem::exists(mBasePath);
	}

	bool FilesystemAdapter::output(const IOFileInfo* file) {
		const auto fullPath = std::filesystem::path(mBasePath) / file->filePath;
		std::error_code ec; // 用于捕获文件系统错误而不抛出异常

		// 1. 创建目录（如果需要）
		if (mCreateDirs) {
			if (!std::filesystem::create_directories(fullPath.parent_path(), ec)) {
				if (ec) {
					// 记录或处理目录创建错误
					return false;
				}
			}
		}

		// 2. 打开文件
		std::ofstream out(fullPath.string(), std::ios::binary);
		if (!out.is_open()) {
			// 可以记录具体错误信息
			return false;
		}

		// 3. 写入数据
		out.write(reinterpret_cast<const char*>(file->data), file->dataSize);

		// 4. 确保所有操作成功
		const bool success = out.good();
		out.close(); // 显式关闭（析构函数会自动调用，但显式调用可以立即检查错误）

		return success;
	}

	bool FilesystemAdapter::outputBatch(const std::vector<IOFileInfo*> files) {
		// 使用路径和原始数据指针+大小的pair
		using filedata = std::pair<const void*, size_t>;
		std::unordered_map<std::string, std::vector<std::pair<std::string, filedata>>> dirgroups;

		// 按目录分组
		for ( auto file : files) {  // 正确解包三元组
			const auto fullpath = std::filesystem::path(mBasePath) / file->filePath;
			dirgroups[fullpath.parent_path().string()].emplace_back(
				fullpath.filename().string(),
				filedata{ file->data, file->dataSize }  // 存储原始指针和大小
			);
		}

		// 批量处理每个目录
		for (const auto& [dir, filelist] : dirgroups) {
			if (mCreateDirs) {
				std::error_code ec;
				if (!std::filesystem::create_directories(dir, ec) && ec) {
					return false;  // 目录创建失败
				}
			}

			for (const auto& [filename, datainfo] : filelist) {
				const auto& [data, size] = datainfo;
				std::ofstream out(std::filesystem::path(dir) / filename, std::ios::binary);
				if (!out) return false;

				// 安全地将void*转换为const char*写入
				out.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
				if (!out.good()) return false;
			}
		}

		return true;
	}

	bool FilesystemAdapter::finalize() {
		return true;
	}
};