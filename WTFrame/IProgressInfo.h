#pragma once
#if defined(emit)
	#undef emit
	#include <oneapi/tbb/task_group.h>
	#define emit
#else
	#include <oneapi/tbb/task_group.h>
#endif
#include <string>

namespace WT {
	class IProgressInfo
	{
		//现在为单线程 多线程请加mutex
	protected:
		int totalNum=0;//总任务数
		int processedNum=0;//已经处理的数
		int ticNum=0;//每执行ticNum任务 更新一次进度条或者报信息
		bool canceled = false;//是否取消处理
		::tbb::task_group_context context;
	public:
		//获取上下问
		::tbb::task_group_context& getContext() {
			return context;
		}

		//获取是否为停止了
		bool isCanceled() {
			return canceled;
		}

		/*****************************************************************************
		* @brief : 设置总任务数
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void setTotalNum(int totalNum) {
			this->totalNum = totalNum;
			this->ticNum = totalNum % 100;
		}
		/*****************************************************************************
		* @brief : 为了防止在实际应用中需要更高频的数据进度显示 可以调用该函数 修改默认100次更新的设置
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void setTicNum(int ticNum) {
			this->ticNum = ticNum;
		}
		/*****************************************************************************
		* @brief : 显示处理进度 返回值是展示表达是否取消
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual bool showProgress(int currentIndex, std::string currentFileName, std::string operats = "数据转换") {
			processedNum = currentIndex;
			if (processedNum % ticNum == 0) {
				printf("%s:%f\t正在处理:%s", operats.c_str(), processedNum / (totalNum * 1.0f) * 100, currentFileName.c_str());
			}
			if (processedNum == totalNum)
			{
				finished();
			}
			return canceled;
		}

		/*****************************************************************************
		* @brief : 显示处理进度 添加一个数值 返回值是展示表达是否取消
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual bool addProgress(int addedIndex, std::string currentFileName, std::string operats = "数据转换") {
			processedNum += addedIndex;
			if (processedNum % ticNum == 0) {
				printf("%s:%f\t正在处理:%s", operats.c_str(), processedNum / (totalNum * 1.0f) * 100, currentFileName.c_str());
			}
			if (processedNum== totalNum)
			{
				finished();
			}
			return canceled;
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
		virtual void cancel() {
			context.cancel_group_execution();
			canceled = true;
		}

		/*****************************************************************************
		* @brief : 完成进度
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void finished(std::string label = "completed!") const {
			printf(label.c_str());
		}
	};
};
