#ifndef WTCANCELATION_H
#define WTCANCELATION_H

#include "WTDefines.h"

WTNAMESPACESTART
FRAMENAMESPACESTART

WT_SMART_POINTER(IWTCancellationToken)

struct CancelledException:public std::runtime_error
{
    CancelledException():std::runtime_error("������ȡ��."){}
};

class WTAPI IWTCancellationToken
{
public:
    virtual ~IWTCancellationToken() {};

    //����ȡ������
    virtual void Cancel() = 0;

    /**
     * ����Ƿ��Ѿ�ȡ��.
     *
     * \��ȡ������true ����false
     */
    virtual bool IsCancelled()const = 0;

    /**
    * ����Ƿ��Ѿ�ȡ�������ȡ�������׳��쳣
    *
    * \���IsCancelledΪtrue�����׳�CancelledException�쳣
    */
    virtual void CheckCancelledAndThrow() const = 0;

    /**
	 * ���ö������һ��ȡ�����ƣ��ö����ȡ�������������еĴӶ���ȡ��
	 *
	 * \param[in] slaveToken ��ӵĴӶ���.
	 *
	 * \return ���ص�һ��int�������ͨ��ClearCancelledCallback��ȡ����Ӧ�����ע��
	 */
    virtual int AddSlaveToken(IWTCancellationTokenPtr const& slaveToken) = 0;

    /**
     * ���һ��ȡ���ص�����
     *
     * \param[in] cancelledCallback ȡ���ص��������޲������޷��أ�
     *
     * \return ���ص�һ��int�������ͨ��ClearCancelledCallback��ȡ����Ӧ�Ӷ����ע��
     */
    virtual int SetCancelledCallback(std::function<void(void)> cancelledCallback) = 0;

    /**
    * ͨ��AddSlaveToken ���� SetCancelledCallback���ص�ע�������Ƴ�һ���Ӷ����ע��.
    *
    * \param[in] �Ӷ����ע����
    */
    virtual void ClearCancelledCallback(int token) = 0;

    /**
    * �Ƴ�����ͨ��SetCancelledCallbackע��Ļص�����
    */
    virtual void ClearAllCallbacks() = 0;
};


//--------------------------------------------------------------------------------

/**
 * ����һ��ȡ������.
 */
IWTCancellationTokenPtr WTAPI MakeCancellationToken();

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

/**
* ����һ���յ�ȡ�����ƣ������ղ���ʱʹ��
*/
IWTCancellationTokenPtr WTAPI MakeNullCancellationToken();

//--------------------------------------------------------------------------------
FRAMENAMESPACEEND
WTNAMESPACEEND
#endif // WTCANCELATION_H
