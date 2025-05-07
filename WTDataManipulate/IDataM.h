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
		int totalNum;//��������
	public:
		/*****************************************************************************
		* @brief : ������������
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void setTotalNum(int totalNum) {
			this->totalNum=totalNum;
		}
		/*****************************************************************************
		* @brief : ��ʾ�������
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void showProgress(int currentIndex, std::string currentFileName, std::string operats = "����ת��") {
			printf("%s:%f\t���ڴ���:%s", operats.c_str(), currentIndex / (totalNum * 1.0f) * 100, currentFileName.c_str());
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
		virtual void finished(std::string label="�������") const {
			printf(label.c_str());
		}
	};

	class IDataProcessor
	{
	public:
		//��������
		virtual bool process(std::shared_ptr<IProgressInfo> progressInfo) = 0;
		//��ȡ����������
		virtual std::string getName()const = 0;
	};
}