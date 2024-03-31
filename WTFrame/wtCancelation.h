#ifndef WTCANCELATION_H
#define WTCANCELATION_H

#include "WTDefines.h"

WTNAMESPACESTART
FRAMENAMESPACESTART

WT_SMART_POINTER(IWTCancellationToken)

struct CancelledException:public std::runtime_error
{
    CancelledException():std::runtime_error("操作被取消."){}
};

class WTAPI IWTCancellationToken
{
public:
    virtual ~IWTCancellationToken() {};

    //请求取消操作
    virtual void Cancel() = 0;

    /**
     * 检查是否已经取消.
     *
     * \已取消返回true 否则false
     */
    virtual bool IsCancelled()const = 0;

    /**
    * 检查是否已经取消，如果取消了则抛出异常
    *
    * \如果IsCancelled为true，则抛出CancelledException异常
    */
    virtual void CheckCancelledAndThrow() const = 0;

    /**
	 * 给该对象添加一个取消令牌，该对象的取消会引起其所有的从对象取消
	 *
	 * \param[in] slaveToken 添加的从对象.
	 *
	 * \return 返回的一个int句柄，可通过ClearCancelledCallback来取消对应对象的注册
	 */
    virtual int AddSlaveToken(IWTCancellationTokenPtr const& slaveToken) = 0;

    /**
     * 添加一个取消回调函数
     *
     * \param[in] cancelledCallback 取消回调函数（无参数，无返回）
     *
     * \return 返回的一个int句柄，可通过ClearCancelledCallback来取消对应从对象的注册
     */
    virtual int SetCancelledCallback(std::function<void(void)> cancelledCallback) = 0;

    /**
    * 通过AddSlaveToken 或者 SetCancelledCallback返回的注册句柄来移除一个从对象的注册.
    *
    * \param[in] 从对象的注册句柄
    */
    virtual void ClearCancelledCallback(int token) = 0;

    /**
    * 移出所有通过SetCancelledCallback注册的回调函数
    */
    virtual void ClearAllCallbacks() = 0;
};


//--------------------------------------------------------------------------------

/**
 * 创建一个取消令牌.
 */
IWTCancellationTokenPtr WTAPI MakeCancellationToken();

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

/**
* 创建一个空的取消令牌，在做空参数时使用
*/
IWTCancellationTokenPtr WTAPI MakeNullCancellationToken();

//--------------------------------------------------------------------------------
FRAMENAMESPACEEND
WTNAMESPACEEND
#endif // WTCANCELATION_H
