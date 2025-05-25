#pragma once
#include <string>

namespace WT {
	class IProgressInfo
	{
	protected:
		int totalNum;//��������
		int processedNum;//�Ѿ��������
	public:
		/*****************************************************************************
		* @brief : ������������
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void setTotalNum(int totalNum) {
			this->totalNum = totalNum;
		}
		/*****************************************************************************
		* @brief : ��ʾ�������
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void showProgress(int currentIndex, std::string currentFileName, std::string operats = "����ת��") {
			processedNum = currentIndex;
			printf("%s:%f\t���ڴ���:%s", operats.c_str(), processedNum / (totalNum * 1.0f) * 100, currentFileName.c_str());
		}

		/*****************************************************************************
		* @brief : ��ʾ������� ���һ����ֵ
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void addProgress(int addedIndex, std::string currentFileName, std::string operats = "����ת��") {
			processedNum += addedIndex;
			printf("%s:%f\t���ڴ���:%s", operats.c_str(), processedNum / (totalNum * 1.0f) * 100, currentFileName.c_str());
		}
		/*****************************************************************************
		* @brief : ��ʾ������ļ�
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void showHandleName(std::string currentFileName, std::string operats = "����ת��") const {
			printf("%s:\t���ڴ���\t%s\r\n", operats.c_str(), currentFileName.c_str());
		}

		/*****************************************************************************
		* @brief : ��ɽ���
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void finished(std::string label = "completed!") const {
			printf(label.c_str());
		}
	};
};
