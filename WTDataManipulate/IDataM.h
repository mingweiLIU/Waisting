#pragma once
#include <string>
#include <iostream>

namespace WT {
	class IDataOptions
	{
	public:
		IDataOptions() = default;
		virtual ~IDataOptions() = default;

	};


	class IProgressInfo
	{
	protected:
		int totalNum;//总任务数
	public:
		/*****************************************************************************
		* @brief : 设置总任务数
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void setTotalNum(int totalNum) {
			this->totalNum=totalNum;
		}
		/*****************************************************************************
		* @brief : 显示处理进度
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void showProgress(int currentIndex, std::string currentFileName, std::string operats = "数据转换") {
			printf("%s:%f\t正在处理:%s", operats.c_str(), currentIndex / (totalNum * 1.0f) * 100, currentFileName.c_str());
		}
		/*****************************************************************************
		* @brief : 显示处理的文件
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void showHandleName(std::string currentFileName, std::string operats = "数据转换") const {
			printf("%s:\t正在处理\t%s\r\n", operats.c_str(), currentFileName.c_str());
		}

		/*****************************************************************************
		* @brief : 完成进度
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void finished(std::string label="处理完成") const {
			printf(label.c_str());
		}
	};

	class IDataProcessor
	{
	public:
		//处理数据
		virtual bool process(std::shared_ptr<IProgressInfo> progressInfo) = 0;
		//获取处理器名称
		virtual std::string getName()const = 0;
	};
}