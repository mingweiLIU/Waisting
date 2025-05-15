#include "FileBatchOutput.h"
#include<iostream>

namespace WT {

	FileBatchOutput::FileBatchOutput(size_t memoryLimit)
		: mMemoryLimit(memoryLimit) {}

	bool FileBatchOutput::addFile(IOFileInfo& fileInfo)
	{
		std::lock_guard<std::mutex> lock(mutex);
		//std::cout << "\r\n添加新文件，文件数" << mFileCache.size() << std::endl;

		size_t newSize = mCurrentMemoryUsage + fileInfo.dataSize;
		if (newSize > mMemoryLimit) {

			//
			//std::cout << "\r\n启动批量输出，文件数" << mFileCache.size() << std::endl;

			if (!outputInternal(false)) {
				return false;
			}
		}

		mFileCache.push_back(fileInfo);
		mCurrentMemoryUsage = mCurrentMemoryUsage + fileInfo.dataSize;
		return true;
	}

	bool FileBatchOutput::output() {
		return outputInternal(true);
	}

	std::future<bool> FileBatchOutput::outputAsync() {
		return std::async(std::launch::async, [this]() {
			return this->outputInternal();
			});
	}

	bool FileBatchOutput::outputInternal(bool shouldLock) {
		if (!mAdapter || mFileCache.empty()) {
			return false;
		}

		if (mCancellationRequested) return false;

		std::vector<IOFileInfo> filesToOutput;
	
		if (shouldLock) {
			//std::cout << "\r\n扫尾批量输出，文件数" << mFileCache.size() << std::endl;
			std::lock_guard<std::mutex> lock(mutex);
			filesToOutput.swap(mFileCache);
			mCurrentMemoryUsage = 0;
		}
		else {
			filesToOutput.swap(mFileCache);
			mCurrentMemoryUsage = 0;
		}		

		bool result = mAdapter->outputBatch(filesToOutput);

		//if (shouldLock) {
		//	std::cout << "\r\n结束扫尾批量输出，文件数" << std::endl;
		//}
		//else {
		//	std::cout << "\r\n结束批量输出，文件数"  << std::endl;
		//}

		return result && !mCancellationRequested;
	}

	void FileBatchOutput::cancel() {
		mCancellationRequested = true;
	}

	void FileBatchOutput::setAdapter(std::unique_ptr<IOAdapter> adapter) {
		mAdapter = std::move(adapter);
	}
};