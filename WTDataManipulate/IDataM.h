#pragma once
#include <string>
#include <iostream>
#include "IProgressInfo.h"

namespace WT {
	class IDataOptions
	{
	public:
		IDataOptions() = default;
		virtual ~IDataOptions() = default;

	};

	class IDataProcessor
	{
	public:
		//处理数据
		virtual bool process(std::shared_ptr<IProgressInfo> progressInfo) = 0;
		//停止处理
		virtual void cancle() = 0;
		//获取处理器名称
		virtual std::string getName()const = 0;
	};
}