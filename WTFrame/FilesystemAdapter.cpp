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
		std::error_code ec; // ���ڲ����ļ�ϵͳ��������׳��쳣

		// 1. ����Ŀ¼�������Ҫ��
		if (mCreateDirs) {
			if (!std::filesystem::create_directories(fullPath.parent_path(), ec)) {
				if (ec) {
					// ��¼����Ŀ¼��������
					return false;
				}
			}
		}

		// 2. ���ļ�
		std::ofstream out(fullPath.string(), std::ios::binary);
		if (!out.is_open()) {
			// ���Լ�¼���������Ϣ
			return false;
		}

		// 3. д������
		out.write(reinterpret_cast<const char*>(file->data), file->dataSize);

		// 4. ȷ�����в����ɹ�
		const bool success = out.good();
		out.close(); // ��ʽ�رգ������������Զ����ã�����ʽ���ÿ�������������

		return success;
	}

	bool FilesystemAdapter::outputBatch(const std::vector<IOFileInfo*> files) {
		// ʹ��·����ԭʼ����ָ��+��С��pair
		using filedata = std::pair<const void*, size_t>;
		std::unordered_map<std::string, std::vector<std::pair<std::string, filedata>>> dirgroups;

		// ��Ŀ¼����
		for ( auto file : files) {  // ��ȷ�����Ԫ��
			const auto fullpath = std::filesystem::path(mBasePath) / file->filePath;
			dirgroups[fullpath.parent_path().string()].emplace_back(
				fullpath.filename().string(),
				filedata{ file->data, file->dataSize }  // �洢ԭʼָ��ʹ�С
			);
		}

		// ��������ÿ��Ŀ¼
		for (const auto& [dir, filelist] : dirgroups) {
			if (mCreateDirs) {
				std::error_code ec;
				if (!std::filesystem::create_directories(dir, ec) && ec) {
					return false;  // Ŀ¼����ʧ��
				}
			}

			for (const auto& [filename, datainfo] : filelist) {
				const auto& [data, size] = datainfo;
				std::ofstream out(std::filesystem::path(dir) / filename, std::ios::binary);
				if (!out) return false;

				// ��ȫ�ؽ�void*ת��Ϊconst char*д��
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