#pragma once
#include <string>

namespace WT {
	class IProgressInfo
	{
	protected:
		int totalNum;//总任务数
		int processedNum;//已经处理的数
	public:
		/*****************************************************************************
		* @brief : 设置总任务数
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void setTotalNum(int totalNum) {
			this->totalNum = totalNum;
		}
		/*****************************************************************************
		* @brief : 显示处理进度
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void showProgress(int currentIndex, std::string currentFileName, std::string operats = "数据转换") {
			processedNum = currentIndex;
			printf("%s:%f\t正在处理:%s", operats.c_str(), processedNum / (totalNum * 1.0f) * 100, currentFileName.c_str());
		}

		/*****************************************************************************
		* @brief : 显示处理进度 添加一个数值
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void addProgress(int addedIndex, std::string currentFileName, std::string operats = "数据转换") {
			processedNum += addedIndex;
			printf("%s:%f\t正在处理:%s", operats.c_str(), processedNum / (totalNum * 1.0f) * 100, currentFileName.c_str());
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
		virtual void finished(std::string label = "completed!") const {
			printf(label.c_str());
		}
	};
};
