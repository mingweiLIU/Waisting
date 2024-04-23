#include "wtCancelation.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
using namespace WT::Frame;

class SimpleCancellationToken :public IWTCancellationToken 
{
public:
	SimpleCancellationToken();
	virtual void Cancel() override;
	virtual bool IsCancelled() const override;

	virtual void ClearAllCallbacks() override;

	virtual int AddSlaveToken(IWTCancellationTokenPtr const& slaveToken) override;

	virtual void CheckCancelledAndThrow() const override;
	virtual int SetCancelledCallback(std::function<void (void)> func) override;
	virtual void ClearCancelledCallback(int registrationToken) override;

private:
	mutable std::mutex mLock;
	int mCallbackIndex;
	bool mCancelled;
	std::map<int, std::function<void(void)>> mCancelledCallback;
};

SimpleCancellationToken::SimpleCancellationToken() :
	mCallbackIndex(0),
	mCancelled(false)
{}

void SimpleCancellationToken::Cancel()
{
	bool firstCancel = false;
	std::map<int, std::function<void(void)>>callbacks;
	{
		std::lock_guard<std::mutex> stackLock(mLock);
		firstCancel = (mCancelled == false);
		mCancelled = true;
		std::swap(callbacks, mCancelledCallback);
	}
	if (firstCancel)
	{
		for (auto& func:callbacks)
		{
			func.second();
		}
	}
}

bool SimpleCancellationToken::IsCancelled() const
{
	std::lock_guard<std::mutex> stackLock(mLock);
	return mCancelled;
}

void SimpleCancellationToken::ClearAllCallbacks()
{
	std::lock_guard<std::mutex> stackLock(mLock);
	mCancelledCallback.clear();
}

int SimpleCancellationToken::AddSlaveToken(IWTCancellationTokenPtr const& slaveToken)
{
	return SetCancelledCallback([slaveToken]()
		{
			slaveToken->Cancel();
		});
}

void SimpleCancellationToken::CheckCancelledAndThrow() const
{
	std::lock_guard<std::mutex> stackLock(mLock);
	if (mCancelled)
	{
		throw CancelledException();
	}
}

int SimpleCancellationToken::SetCancelledCallback(std::function<void(void)> func)
{
	auto cancelled = false;
	auto registrationToken = 0;
	{
		std::lock_guard<std::mutex> stackLock(mLock);

		registrationToken = ++mCallbackIndex;
		cancelled = mCancelled;

		if (!cancelled)
		{
			mCancelledCallback[registrationToken] = std::move(func);
		}
	}
	if (cancelled)
	{
		func();
	}
	return registrationToken;
}

void SimpleCancellationToken::ClearCancelledCallback(int registrationToken)
{
	std::lock_guard<std::mutex> stackLock(mLock);

	auto iter = mCancelledCallback.find(registrationToken);
	if (iter != mCancelledCallback.end())
	{
		mCancelledCallback.erase(iter);
	}
}

IWTCancellationTokenPtr WTFRAMEAPI MakeCancellationToken()
{
	return std::make_shared<SimpleCancellationToken>();
}




class NullCancellationToken : public IWTCancellationToken
{
public:
	NullCancellationToken()
	{
	}

public: // IWTCancellationToken
	virtual void Cancel() override
	{
	}

	virtual bool IsCancelled() const override
	{
		return false;
	}

	virtual void ClearAllCallbacks() override
	{
	}

	virtual int AddSlaveToken(IWTCancellationTokenPtr const& slaveToken) override
	{
		return 0;
	}

	virtual void CheckCancelledAndThrow() const override
	{
	}

	virtual int SetCancelledCallback(std::function<void (void)> func) override
	{
		return 0;
	}

	virtual void ClearCancelledCallback(int registrationToken) override
	{
	}
};

IWTCancellationTokenPtr WTFRAMEAPI MakeNullCancellationToken()
{
	return std::make_shared<NullCancellationToken>();
}
