/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#pragma once

#if !defined(COREUTILS_SMARTTHROW_H_INCLUDED)
#define COREUTILS_SMARTTHROW_H_INCLUDED
#include "wtUlitsDefines.h"

#ifdef _WIN32

#if !defined(_INC_WINDOWS)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif // _INC_WINDOWS

#endif // _WIN32

#include <exception>
#include <string>
#include <stdexcept>


WTNAMESPACESTART
ULITSNAMESPACESTART

struct WTULITSAPI WTException : public std::runtime_error
{
	WTException(std::string const& message, bool assert = false);
#ifdef _WIN32
	WTException(DWORD win32Error, std::string const& message, bool assert = false);
	WTException(HRESULT hr, std::string const& message, bool assert = false);
#endif // _WIN32

	std::string const m_message;
#ifdef _WIN32
	DWORD const       m_win32Error;
	HRESULT const     m_hr;
#endif // _WIN32
};

struct WTULITSAPI WTInvalidArgException : public WTException
{
	WTInvalidArgException(std::string const& message, bool assert = false)
#ifdef _WIN32
		: WTException(E_INVALIDARG, message, assert)
#else
		: WTException(message, assert)
#endif
	{}
};

struct WTULITSAPI WTFailException : public WTException
{
	WTFailException(std::string const& message, bool assert = false)
#ifdef _WIN32
		: WTException(E_FAIL, message, assert)
#else
		: WTException(message, assert)
#endif
	{}
};

struct WTULITSAPI WTNotImplException : public WTException
{
	WTNotImplException(std::string const& message, bool assert = false)
#ifdef _WIN32
		: WTException(E_NOTIMPL, message, assert)
#else
		: WTException(message, assert)
#endif
	{}
};

template<class E>
inline std::exception_ptr GetExceptionPtr()
{
	std::exception_ptr ptr;
	try
	{
		throw E();
	}
	catch (...)
	{
		ptr = std::current_exception();
	}
	return ptr;
}

template<class E, class ... Args>
inline std::exception_ptr GetExceptionPtr(Args && ... args)
{
	std::exception_ptr ptr;
	try
	{
		throw E(std::forward<Args>(args) ...);
	}
	catch (...)
	{
		ptr = std::current_exception();
	}
	return ptr;
}

#ifdef _WIN32

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw WTException(hr, "Thrown from ThrowIfFailed.", false);
	}
}

inline void ThrowIfFailed(HRESULT hr, const char* customMessage)
{
	if (FAILED(hr))
	{
		throw WTException(hr, customMessage, false);
	}
}
#endif // _WIN32
ULITSNAMESPACEEND
WTNAMESPACEEND

#endif // COREUTILS_SMARTTHROW_H_INCLUDED
