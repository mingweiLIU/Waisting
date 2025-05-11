#pragma once
#include "IOAdapter.h"
#include <atomic>
#include <mutex>
#include <memory>
#include <future>

namespace WT {
	class FileBatchOutput {
	public:
		explicit FileBatchOutput(size_t memoryLimit = 100 * 1024 * 1024);

		bool addFile(IOFileInfo fileInfo);

		void setAdapter(std::unique_ptr<IOAdapter> adapter);
		std::future<bool> outputAsync();
		bool output();
		void cancel();

		size_t memoryUsage() const { return mCurrentMemoryUsage; }

	private:
		std::vector<IOFileInfo> mFileCache;
		std::mutex mutex;
		std::unique_ptr<IOAdapter> mAdapter;
		std::atomic<size_t> mCurrentMemoryUsage{0};
		std::atomic<size_t> mMemoryLimit;
		std::atomic<bool> mCancellationRequested{false};

		bool outputInternal();
	};
};