/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "wtStringUtils.h"

#include <algorithm>
#include <cctype>
#include <regex>

USINGULITSNAMESPACE

namespace Details
{
	template <typename TChar>
	std::basic_string<TChar> LTrim(std::basic_string<TChar> str)
	{
		str.erase(str.begin(), std::find_if_not(str.begin(), str.end(), [](TChar ch) { return std::isspace(ch); }));
		return str;
	}

	template <typename TChar>
	std::basic_string<TChar> RTrim(std::basic_string<TChar> str)
	{
		str.erase(std::find_if_not(str.rbegin(), str.rend(), [](TChar ch) { return std::isspace(ch); }).base(), str.end());
		return str;
	}

	template <typename TChar>
	std::basic_string<TChar> Trim(std::basic_string<TChar> str)
	{
		return LTrim(RTrim(std::move(str)));
	}

	template <typename TChar>
	void TransformToLower(std::basic_string<TChar>& str)
	{
		std::transform(str.begin(), str.end(), str.begin(),
			[](const TChar c) { return static_cast<TChar>(::tolower(c)); }
		);
	}

	template <typename TChar>
	std::basic_string<TChar> ToLower(std::basic_string<TChar> str)
	{
		auto strCopy = str;
		TransformToLower(strCopy);
		return strCopy;
	}

	template <typename TChar>
	std::vector<std::basic_string<TChar>> Split(const std::basic_string<TChar>& string, const std::basic_string<TChar>& delimiter)
	{
		typename std::basic_string<TChar>::size_type startPos = 0;
		std::vector<std::basic_string<TChar>> res;
		while (true)
		{
			typename std::basic_string<TChar>::size_type endPos = string.find(delimiter, startPos);
			if (endPos == std::basic_string<TChar>::npos)
			{
				typename std::basic_string<TChar>::size_type newEndPos = string.size();
				res.push_back(string.substr(startPos, newEndPos - startPos));
				return res;
			}
			res.push_back(string.substr(startPos, endPos - startPos));
			startPos = endPos + delimiter.size();
		}
	}

	template <typename TChar>
	std::vector<std::basic_string<TChar>> SplitRegex(const std::basic_string<TChar>& string, const std::basic_string<TChar>& delimiter)
	{
		std::vector<std::basic_string<TChar>> results;

		std::basic_regex<typename std::basic_string<TChar>::value_type> regex(delimiter);
		for (std::regex_token_iterator<typename std::basic_string<TChar>::const_iterator> i(begin(string), end(string), regex, -1), endI; i != endI; ++i)
		{
			results.push_back(*i);
		}

		return results;
	}

	template <typename TChar>
	std::basic_string<TChar> StringReplace(std::basic_string<TChar> const& string, std::basic_string<TChar> const& what, std::basic_string<TChar> const& with)
	{
		auto vals = WT::Ulits::Split(string, what);
		std::basic_string<TChar> res;
		if (!vals.empty())
		{
			res = std::move(vals[0]);
			for (auto iter = 1 + vals.begin(); iter != vals.end(); ++iter)
			{
				res += with + std::move(*iter);
			}
		}
		return res;
	}

	template <typename TChar>
	bool IsPathAbsolute(const std::basic_string<TChar>& path)
	{
		// https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats
		if (path.size() >= 1)
		{
			if (path[0] == '/' || path[0] == '\\')
			{
				return true;
			}

			if (path.size() >= 3
				&& path[1] == ':'
				&& (path[2] == '/' || path[2] == '\\'))
			{
				return true;
			}
		}

		return false;
	}

	template <typename TChar>
	std::basic_string<TChar> GetPathDirectory(std::basic_string<TChar> const& path)
	{
		size_t pos = path.find_last_of('/');
		if (pos == std::basic_string<TChar>::npos)
		{
			pos = path.find_last_of('\\');
		}

		if (pos == std::basic_string<TChar>::npos)
		{
			// path does not have a leading directory
			return {};
		}

		return path.substr(0, pos + 1);
	}

	template <typename TChar>
	std::basic_string<TChar> GetPathFileName(std::basic_string<TChar> const& path)
	{
		size_t pos = path.find_last_of('/');
		if (pos == std::basic_string<TChar>::npos)
		{
			pos = path.find_last_of('\\');
		}
		if (pos == std::basic_string<TChar>::npos)
		{
			return path;
		}

		return path.substr(pos + 1);
	}

	template <typename TChar>
	std::basic_string<TChar> GetPathFileExtension(std::basic_string<TChar> const& path)
	{
		auto filename = GetPathFileName(path);

		size_t pos = path.find_last_of('.');
		if (pos == std::basic_string<TChar>::npos)
		{
			// No extension
			return std::basic_string<TChar>();
		}

		return path.substr(pos + 1);
	}

	template <typename TChar>
	std::basic_string<TChar> GetPathFileExtensionLower(std::basic_string<TChar> const& path)
	{
		auto extension = GetPathFileExtension(path);
		WT::Ulits::TransformToLower(extension);
		return extension;
	}

	template <typename TChar>
	bool BeginsWith(std::basic_string<TChar> const& a, TChar b)
	{
		if (!a.empty())
		{
			return a.front() == b;
		}

		return false;
	}

	template <typename TChar>
	bool BeginsWith(std::basic_string<TChar> const& a, std::basic_string<TChar> const& b)
	{
		if (a.length() >= b.length())
		{
			return (0 == a.compare(0, b.length(), b));
		}
		return false;
	}

	template <typename TChar>
	bool EndsWith(std::basic_string<TChar> const& a, TChar b)
	{
		if (!a.empty())
		{
			return a.back() == b;
		}

		return false;
	}

	template <typename TChar>
	bool EndsWith(std::basic_string<TChar> const& a, std::basic_string<TChar> const& b)
	{
		if (a.length() >= b.length())
		{
			return (0 == a.compare(a.length() - b.length(), b.length(), b));
		}
		return false;
	}
}

#ifdef _WIN32

// For MultiByteToWideChar and WideCharToMultiByte
// The code can be ported to cross-platform versions but it is much slower,
// so we should also maintain the Windows implementations.
#if !defined(_INC_WINDOWS)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif // _INC_WINDOWS

std::wstring WT::Ulits::StringToWString(const char* s, size_t size)
{
	if (size == 0)
	{
		return std::wstring();
	}

	int length = ::MultiByteToWideChar(
		CP_UTF8,
		0,
		s,
		static_cast<int>(size),
		nullptr,
		0
	);

	std::wstring retval;
	if (length > 0)
	{
		retval.resize(length);
		::MultiByteToWideChar(
			CP_UTF8,
			0,
			s,
			static_cast<int>(size),
			&retval[0],
			length
		);
	}
	else
	{
		__fastfail(FAST_FAIL_FATAL_APP_EXIT);
	}

	return retval;
}

std::string WT::Ulits::WStringToString(const wchar_t* w, size_t size)
{
	if (size == 0)
	{
		return std::string();
	}

	INT len = ::WideCharToMultiByte(
		CP_UTF8,
		0,
		w,
		static_cast<int>(size),
		nullptr,
		0,
		nullptr,
		nullptr);

	std::string ret;
	if (len > 0)
	{
		ret.resize(len);
		::WideCharToMultiByte(
			CP_UTF8,
			0,
			w,
			static_cast<int>(size),
			&ret[0],
			len,
			nullptr,
			nullptr);
	}
	else
	{
		__fastfail(FAST_FAIL_FATAL_APP_EXIT);
	}

	return ret;
}

#else // _WIN32

#include <codecvt>

std::wstring WT::Ulits::StringToWString(const char* s, size_t size)
{
	std::wstring result;

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convertor;
	result = convertor.from_bytes(s, s + size);

	return result;
}

std::string WT::Ulits::WStringToString(const wchar_t* w, size_t size)
{
	std::string result;

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convertor;
	result = convertor.to_bytes(w, w + size);

	return result;
}

#endif

std::wstring WT::Ulits::StringToWString(const std::string& s)
{
	return StringToWString(s.data(), s.size());
}

std::string WT::Ulits::WStringToString(const std::wstring& w)
{
	return WStringToString(w.data(), w.size());
}

std::string WT::Ulits::LTrim(std::string str)
{
	return Details::LTrim(std::move(str));
}

std::wstring WT::Ulits::LTrim(std::wstring str)
{
	return Details::LTrim(std::move(str));
}

std::string WT::Ulits::RTrim(std::string str)
{
	return Details::RTrim(std::move(str));
}

std::wstring WT::Ulits::RTrim(std::wstring str)
{
	return Details::RTrim(std::move(str));
}

std::string WT::Ulits::Trim(std::string str)
{
	return Details::Trim(std::move(str));
}

std::wstring WT::Ulits::Trim(std::wstring str)
{
	return Details::Trim(std::move(str));
}

std::vector<std::string> WT::Ulits::Split(const std::string& string, const std::string& delimiter)
{
	return Details::Split(string, delimiter);
}

std::vector<std::wstring> WT::Ulits::Split(const std::wstring& string, const std::wstring& delimiter)
{
	return Details::Split(string, delimiter);
}

std::vector<std::string> WT::Ulits::SplitRegex(const std::string& string, const std::string& regex)
{
	return Details::SplitRegex(string, regex);
}

std::vector<std::wstring> WT::Ulits::SplitRegex(const std::wstring& string, const std::wstring& regex)
{
	return Details::SplitRegex(string, regex);
}

std::string WT::Ulits::StringReplace(std::string const& string, std::string const& what, std::string const& with)
{
	return Details::StringReplace(string, what, with);
}

std::wstring WT::Ulits::StringReplace(std::wstring const& string, std::wstring const& what, std::wstring const& with)
{
	return Details::StringReplace(string, what, with);
}

std::string WT::Ulits::GetPathConsistentSlashes(std::string const& path)
{
	// TODO: Use a regexp, in cases where we have many many consecutive slashes.
	auto tmp = path;
	tmp = WT::Ulits::StringReplace(tmp, "\\", "/");
	tmp = WT::Ulits::StringReplace(tmp, "//", "/");
	return tmp;
}

std::wstring WT::Ulits::GetPathConsistentSlashes(std::wstring const& path)
{
	// TODO: Use a regexp, in cases where we have many many consecutive slashes.
	auto tmp = path;
	tmp = WT::Ulits::StringReplace(tmp, L"\\", L"/");
	tmp = WT::Ulits::StringReplace(tmp, L"//", L"/");
	return tmp;
}

std::string WT::Ulits::GetPathConcatenation(std::string const& first, std::string const& second)
{
	return first + '/' + second;
}

std::wstring WT::Ulits::GetPathConcatenation(std::wstring const& first, std::wstring const& second)
{
	return first + L'/' + second;
}

bool WT::Ulits::IsPathAbsolute(const std::string& path)
{
	return Details::IsPathAbsolute(path);
}

bool WT::Ulits::IsPathAbsolute(const std::wstring& path)
{
	return Details::IsPathAbsolute(path);
}

bool WT::Ulits::IsPathRelative(const std::string& path)
{
	return !Details::IsPathAbsolute(path);
}

bool WT::Ulits::IsPathRelative(const std::wstring& path)
{
	return !Details::IsPathAbsolute(path);
}

std::string WT::Ulits::GetPathDirectory(std::string const& path)
{
	return Details::GetPathDirectory(path);
}

std::wstring WT::Ulits::GetPathDirectory(std::wstring const& path)
{
	return Details::GetPathDirectory(path);
}

std::string WT::Ulits::GetPathFileName(std::string const& path)
{
	return Details::GetPathFileName(path);
}

std::wstring WT::Ulits::GetPathFileName(std::wstring const& path)
{
	return Details::GetPathFileName(path);
}

std::string WT::Ulits::GetPathFileExtension(std::string const& path)
{
	return Details::GetPathFileExtension(path);
}

std::wstring WT::Ulits::GetPathFileExtension(std::wstring const& path)
{
	return Details::GetPathFileExtension(path);
}

std::string WT::Ulits::GetPathFileExtensionLower(std::string const& path)
{
	return Details::GetPathFileExtensionLower(path);
}

std::wstring WT::Ulits::GetPathFileExtensionLower(std::wstring const& path)
{
	return Details::GetPathFileExtension(path);
}

bool WT::Ulits::BeginsWith(std::string const& a, char b)
{
	return Details::BeginsWith(a, b);
}

bool WT::Ulits::BeginsWith(std::string const& a, std::string const& b)
{
	return Details::BeginsWith(a, b);
}

bool WT::Ulits::BeginsWith(std::wstring const& a, wchar_t b)
{
	return Details::BeginsWith(a, b);
}

bool WT::Ulits::BeginsWith(std::wstring const& a, std::wstring const& b)
{
	return Details::BeginsWith(a, b);
}

bool WT::Ulits::EndsWith(std::string const& a, char b)
{
	return Details::EndsWith(a, b);
}

bool WT::Ulits::EndsWith(std::string const& a, std::string const& b)
{
	return Details::EndsWith(a, b);
}

bool WT::Ulits::EndsWith(std::wstring const& a, wchar_t b)
{
	return Details::EndsWith(a, b);
}

bool WT::Ulits::EndsWith(std::wstring const& a, std::wstring const& b)
{
	return Details::EndsWith(a, b);
}

void WT::Ulits::TransformToLower(std::string& str)
{
	return Details::TransformToLower(str);
}

void WT::Ulits::TransformToLower(std::wstring& str)
{
	return Details::TransformToLower(str);
}

std::string WT::Ulits::ToLower(const std::string& str)
{
	return Details::ToLower(str);
}

std::wstring WT::Ulits::ToLower(const std::wstring& str)
{
	return Details::ToLower(str);
}

#ifndef _MSC_VER

// NOTE: _MaxCount does not include the terminating nul, so it may write out _MaxCount+1 bytes.
int WT::Ulits::BabylonVSNPrintf_s(char* const _Buffer, size_t const _BufferCount, size_t const _MaxCount, char const* const _Format, va_list _ArgList)
{
	// Early out with bad parameters.
	if (_Buffer == nullptr || _Format == nullptr || _BufferCount == 0)
	{
		errno = EINVAL;
		return -1;
	}

	// vsnprintf_s _MaxCount does not include the terminator (whereas vsnprintf does), so set count to be compatible with vsnprintf
	size_t count = _MaxCount + 1;
	bool truncate = false;

	// If _MaxCount is set to Babylon_TRUNCATE then reset it to just large enough to fit in _BufferCount with a null terminator
	if (_MaxCount == Babylon_TRUNCATE)
	{
		count = _BufferCount;
		truncate = true;
	}

	// Technically it might be fine for _BufferCount to be smaller than count if the string itself is shorter,
	// but that would be bad practice to do so and should be disallowed even if not specifically outlined in the docs.
	// This also makes the rest of the code easier to implement since we can rely on _BufferCount always being big enough.
	if (count > _BufferCount)
	{
		_Buffer[0] = '\0';
		errno = ERANGE;
		return -1;
	}

	int result = vsnprintf(_Buffer, count, _Format, _ArgList);

	// On error or non-truncated success return the same result from vsnprintf;
	if (result < (int)count)
	{
		return result;
	}

	// If we got this far then we truncated the string to fit in _MaxCount.
	// Add the nul terminator at the end of the string to be safe and return the number of bytes written (not including the nul terminator).
	_Buffer[count - 1] = '\0';
	return static_cast<int>(count) - 1;
}

#endif // #ifndef _MSC_VER
