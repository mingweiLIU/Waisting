#pragma once
#include "WTFrame/WTDefines.h"
#include <iostream>
#include <string>

WTNAMESPACESTART
class ProgressInfo
{
public:
	/*****************************************************************************
	* @brief : 显示处理进度
	* @author : lmw
	* @date : 2020/9/3 9:55
	*****************************************************************************/
	virtual void showProgress(int allNum, int currentIndex, std::string currentFileName, std::string operats = "数据转换") {
		printf("%s:%f\t正在处理:%s", operats.c_str(), currentIndex / (allNum*1.0f) * 100, currentFileName.c_str());
		//std::cout<<operats<<":"<< currentIndex / (allNum*1.0f) * 100 << "%\t正在处理:" << currentFileName << "\n";
	}
	/*****************************************************************************
	* @brief : 显示处理的文件
	* @author : lmw
	* @date : 2020/9/3 9:55
	*****************************************************************************/
	virtual void showHandleName(std::string currentFileName, std::string operats = "数据转换") const {
		printf("%s:\t正在处理\t%s\r\n",operats.c_str(),currentFileName.c_str());
		//std::cout<< ":" << "\t正在处理\t" << currentFileName << "\n";
	}
};
WTNAMESPACEEND