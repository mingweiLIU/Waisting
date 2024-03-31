#pragma once
#include "WTFrame/WTDefines.h"
#include <iostream>
#include <string>

WTNAMESPACESTART
class ProgressInfo
{
public:
	/*****************************************************************************
	* @brief : ��ʾ�������
	* @author : lmw
	* @date : 2020/9/3 9:55
	*****************************************************************************/
	virtual void showProgress(int allNum, int currentIndex, std::string currentFileName, std::string operats = "����ת��") {
		printf("%s:%f\t���ڴ���:%s", operats.c_str(), currentIndex / (allNum*1.0f) * 100, currentFileName.c_str());
		//std::cout<<operats<<":"<< currentIndex / (allNum*1.0f) * 100 << "%\t���ڴ���:" << currentFileName << "\n";
	}
	/*****************************************************************************
	* @brief : ��ʾ������ļ�
	* @author : lmw
	* @date : 2020/9/3 9:55
	*****************************************************************************/
	virtual void showHandleName(std::string currentFileName, std::string operats = "����ת��") const {
		printf("%s:\t���ڴ���\t%s\r\n",operats.c_str(),currentFileName.c_str());
		//std::cout<< ":" << "\t���ڴ���\t" << currentFileName << "\n";
	}
};
WTNAMESPACEEND