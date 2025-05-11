#pragma once
#include "IOAdapter.h"
#include <filesystem>
namespace WT {
	class FilesystemAdapter : public IOAdapter {
	public:
		explicit FilesystemAdapter(const std::string& basePath, bool createDirs = true);

		bool initialize() override;
		bool output(const std::string& virtualPath, void* data, size_t dataSize) override;
		bool outputBatch(const std::vector<IOFileInfo> files) override;
		bool finalize() override;
		std::string type() const override { return "filesystem"; }
		
	private:
		std::string mBasePath;
		bool mCreateDirs;
	};
};