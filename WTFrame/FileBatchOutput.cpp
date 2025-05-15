#include "FileBatchOutput.h"
#include<iostream>

namespace WT {

	FileBatchOutput::FileBatchOutput(size_t memoryLimit)
		: mMemoryLimit(memoryLimit) {}

	bool FileBatchOutput::addFile(IOFileInfo& fileInfo)
	{
		std::lock_guard<std::mutex> lock(mutex);
		//std::cout << "\r\n������ļ����ļ���" << mFileCache.size() << std::endl;

		size_t newSize = mCurrentMemoryUsage + fileInfo.dataSize;
		if (newSize > mMemoryLimit) {

			//
			//std::cout << "\r\n��������������ļ���" << mFileCache.size() << std::endl;

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
			//std::cout << "\r\nɨβ����������ļ���" << mFileCache.size() << std::endl;
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
		//	std::cout << "\r\n����ɨβ����������ļ���" << std::endl;
		//}
		//else {
		//	std::cout << "\r\n��������������ļ���"  << std::endl;
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