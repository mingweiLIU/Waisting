#pragma once
#include <string>
#include <vector>
#include <memory>
#include "IProgressInfo.h"

namespace WT {
	struct IOFileInfo
	{
	public:
		std::string filePath;
		unsigned char* data= nullptr;//数据
		size_t dataSize;
	};
	class IOAdapter {
	public:
		virtual ~IOAdapter() = default;

		virtual bool initialize() = 0;
		virtual bool output(const IOFileInfo fileInfo) = 0;
		virtual bool outputBatch(const std::vector<IOFileInfo> files) = 0;
		virtual bool finalize() = 0;
		virtual std::string type() const = 0;

		virtual void setProgressCallback(std::shared_ptr< IProgressInfo> progressInfo) { this->progressInfo = progressInfo; }
		void cancel() { mCancel = true; }

	protected:
		bool mCancel = false;//用于控制停止输出
		std::shared_ptr< IProgressInfo> progressInfo;
	};
};