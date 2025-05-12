#pragma once
#include "IOAdapter.h"
#include <filesystem>
namespace WT {
	class FilesystemAdapter : public IOAdapter {
	public:
		explicit FilesystemAdapter(const std::string& basePath, bool createDirs = true);

		bool initialize() override;
		bool output(const IOFileInfo file) override;
		bool outputBatch(const std::vector<IOFileInfo> files) override;
		bool finalize() override;
		std::string type() const override { return "filesystem"; }
		
	private:
		std::string mBasePath;
		bool mCreateDirs;
	};
};