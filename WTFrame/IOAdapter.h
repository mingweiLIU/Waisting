#pragma once
#include<iostream>
#include <string>
#include <vector>
#include <memory>
#include "MemoryPool.h"
#include "IProgressInfo.h"

namespace WT {
	struct IOFileInfo
	{
	public:
		IOFileInfo(std::string filePath, unsigned char* data, size_t dataSize, std::string memoryPoolInstanceName)
		:filePath(filePath),data(data),dataSize(dataSize), memoryPoolInstanceName(memoryPoolInstanceName){}

		~IOFileInfo() {
			//删除数据 这里的char* 使用malloc分配的
			if (memoryPoolInstanceName=="")
			{
				free(data);
				data = nullptr;
			}
			else {
				MemoryPool::GetInstance(memoryPoolInstanceName)->deallocate(data,dataSize);
			}
		}
	public:
		std::string filePath;
		unsigned char* data= nullptr;//数据
		size_t dataSize;
		std::string memoryPoolInstanceName = "";
	};
	class IOAdapter {
	public:
		virtual ~IOAdapter() = default;

		virtual bool initialize() = 0;
		virtual bool output(const IOFileInfo* fileInfo) = 0;
		virtual bool outputBatch(const std::vector<IOFileInfo*> files) = 0;
		virtual bool finalize() = 0;
		virtual std::string type() const = 0;

		virtual void setProgressCallback(std::shared_ptr< IProgressInfo> progressInfo) { this->progressInfo = progressInfo; }
		void cancel() { mCancel = true; }

	protected:
		bool mCancel = false;//用于控制停止输出
		std::shared_ptr< IProgressInfo> progressInfo;
	};
};