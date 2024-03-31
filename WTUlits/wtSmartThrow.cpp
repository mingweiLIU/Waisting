/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.   *
*                                                       *
********************************************************/

#include "wtSmartThrow.h"
#include <string>
#include "WTFrame/wtLog.h"


USINGULITSNAMESPACE

WTException::WTException(std::string const& message, bool assert)
	: std::runtime_error(message)
	, m_message(message)
#ifdef _WIN32
	, m_win32Error(0)
	, m_hr(S_OK)
#endif
{
 	wtLOG_ERROR("Throwing WTException with message '%s'", message.c_str());	
}

#ifdef _WIN32

WTException::WTException(DWORD win32Error, std::string const& message, bool assert) :
	std::runtime_error(message.c_str()),
	m_message(message),
	m_win32Error(win32Error),
	m_hr(HRESULT_FROM_WIN32(win32Error))
{
	wtLOG_ERROR("Throwing WTException with message '%s' and win32 error %u", message.c_str(), m_win32Error);

}

WTException::WTException(HRESULT hr, std::string const& message, bool assert) :
	std::runtime_error(message.c_str()),
	m_message(message),
	m_win32Error(0),
	m_hr(hr)
{
	wtLOG_ERROR("Throwing WTException with message '%s' and HRESULT error 0x%.8x", message.c_str(), hr);
}

#endif
