#pragma once
#include<iostream>
#include <string>
#include <vector>
#include <memory>
#include "IProgressInfo.h"

namespace WT {
	struct IOFileInfo
	{
	public:
		IOFileInfo(std::string filePath, unsigned char* data, size_t dataSize, std::string memoryPoolInstanceName)
		:filePath(filePath),data(data),dataSize(dataSize), memoryPoolInstanceName(memoryPoolInstanceName){}

		~IOFileInfo();
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
		//单个输出
		virtual bool output(const IOFileInfo* fileInfo) = 0;
		//批量同步输出
		virtual bool outputBatch(const std::vector<IOFileInfo*> files) = 0;
		// 批量输出（完全异步） 不强制要求
		virtual bool outputBatchAsync(const std::vector<IOFileInfo*> files) { return false; };

		virtual bool finalize() = 0;
		virtual std::string type() const = 0;

		virtual void setProgressCallback(std::shared_ptr< IProgressInfo> progressInfo) { this->progressInfo = progressInfo; }
		void cancel() { mCancel = true; }

	protected:
		bool mCancel = false;//用于控制停止输出
		std::shared_ptr< IProgressInfo> progressInfo;
	};
};