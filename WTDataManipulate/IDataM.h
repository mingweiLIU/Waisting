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
		//��������
		virtual bool process(std::shared_ptr<IProgressInfo> progressInfo) = 0;
		//ֹͣ����
		virtual void cancle() = 0;
		//��ȡ����������
		virtual std::string getName()const = 0;
	};
}