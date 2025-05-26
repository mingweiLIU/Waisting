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
		//����Ϊ���߳� ���߳����mutex
	protected:
		int totalNum=0;//��������
		int processedNum=0;//�Ѿ��������
		int ticNum=0;//ÿִ��ticNum���� ����һ�ν��������߱���Ϣ
		bool canceled = false;//�Ƿ�ȡ������
		::tbb::task_group_context context;
	public:
		//��ȡ������
		::tbb::task_group_context& getContext() {
			return context;
		}

		//��ȡ�Ƿ�Ϊֹͣ��
		bool isCanceled() {
			return canceled;
		}

		/*****************************************************************************
		* @brief : ������������
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void setTotalNum(int totalNum) {
			this->totalNum = totalNum;
			this->ticNum = totalNum % 100;
		}
		/*****************************************************************************
		* @brief : Ϊ�˷�ֹ��ʵ��Ӧ������Ҫ����Ƶ�����ݽ�����ʾ ���Ե��øú��� �޸�Ĭ��100�θ��µ�����
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual void setTicNum(int ticNum) {
			this->ticNum = ticNum;
		}
		/*****************************************************************************
		* @brief : ��ʾ������� ����ֵ��չʾ����Ƿ�ȡ��
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual bool showProgress(int currentIndex, std::string currentFileName, std::string operats = "����ת��") {
			processedNum = currentIndex;
			if (processedNum % ticNum == 0) {
				printf("%s:%f\t���ڴ���:%s", operats.c_str(), processedNum / (totalNum * 1.0f) * 100, currentFileName.c_str());
			}
			if (processedNum == totalNum)
			{
				finished();
			}
			return canceled;
		}

		/*****************************************************************************
		* @brief : ��ʾ������� ���һ����ֵ ����ֵ��չʾ����Ƿ�ȡ��
		* @author : lmw
		* @date : 2020/9/3 9:55
		*****************************************************************************/
		virtual bool addProgress(int addedIndex, std::string currentFileName, std::string operats = "����ת��") {
			processedNum += addedIndex;
			if (processedNum % ticNum == 0) {
				printf("%s:%f\t���ڴ���:%s", operats.c_str(), processedNum / (totalNum * 1.0f) * 100, currentFileName.c_str());
			}
			if (processedNum== totalNum)
			{
				finished();
			}
			return canceled;
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
		virtual void cancel() {
			context.cancel_group_execution();
			canceled = true;
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
