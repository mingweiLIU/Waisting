/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "wtStringUtils.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <Windows.h>

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
		auto vals = Split(string, what);
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
		WT::Ulits::wtStringUtils::TransformToLower(extension);
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

std::wstring WT::Ulits::wtStringUtils::StringToWString(const char* s, size_t size)
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

std::wstring WT::Ulits::wtStringUtils::StringToWString(const std::string& s)
{
	return StringToWString(s.data(), s.size());
}

std::string WT::Ulits::wtStringUtils::WStringToString(const wchar_t* w, size_t size)
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

std::string WT::Ulits::wtStringUtils::WStringToString(const std::wstring& w)
{
	return WStringToString(w.data(), w.size());
}

std::string WT::Ulits::wtStringUtils::LTrim(std::string str)
{
	return Details::LTrim(std::move(str));
}

std::wstring WT::Ulits::wtStringUtils::LTrim(std::wstring str)
{
	return Details::LTrim(std::move(str));
}

std::string WT::Ulits::wtStringUtils::RTrim(std::string str)
{
	return Details::RTrim(std::move(str));
}

std::wstring WT::Ulits::wtStringUtils::RTrim(std::wstring str)
{
	return Details::RTrim(std::move(str));
}

std::string WT::Ulits::wtStringUtils::Trim(std::string str)
{
	return Details::Trim(std::move(str));
}

std::wstring WT::Ulits::wtStringUtils::Trim(std::wstring str)
{
	return Details::Trim(std::move(str));
}

std::vector<std::string> WT::Ulits::wtStringUtils::Split(const std::string& string, const std::string& delimiter)
{
	return Details::Split(string, delimiter);
}

std::vector<std::wstring> WT::Ulits::wtStringUtils::Split(const std::wstring& string, const std::wstring& delimiter)
{
	return Details::Split(string, delimiter);
}

std::vector<std::string> WT::Ulits::wtStringUtils::SplitRegex(const std::string& string, const std::string& regex)
{
	return Details::SplitRegex(string, regex);
}

std::vector<std::wstring> WT::Ulits::wtStringUtils::SplitRegex(const std::wstring& string, const std::wstring& regex)
{
	return Details::SplitRegex(string, regex);
}

std::string WT::Ulits::wtStringUtils::StringReplace(std::string const& string, std::string const& what, std::string const& with)
{
	return Details::StringReplace(string, what, with);
}

std::wstring WT::Ulits::wtStringUtils::StringReplace(std::wstring const& string, std::wstring const& what, std::wstring const& with)
{
	return Details::StringReplace(string, what, with);
}

bool WT::Ulits::wtStringUtils::IsPathAbsolute(const std::string& path)
{
	return Details::IsPathAbsolute(path);
}

bool WT::Ulits::wtStringUtils::IsPathAbsolute(const std::wstring& path)
{
	return Details::IsPathAbsolute(path);
}

bool WT::Ulits::wtStringUtils::IsPathRelative(const std::string& path)
{
	return !Details::IsPathAbsolute(path);
}

bool WT::Ulits::wtStringUtils::IsPathRelative(const std::wstring& path)
{
	return !Details::IsPathAbsolute(path);
}

std::string WT::Ulits::wtStringUtils::GetPathConsistentSlashes(std::string const& path)
{
	// TODO: Use a regexp, in cases where we have many many consecutive slashes.
	auto tmp = path;
	tmp = StringReplace(tmp, "\\", "/");
	tmp = StringReplace(tmp, "//", "/");
	return tmp;
}

std::wstring WT::Ulits::wtStringUtils::GetPathConsistentSlashes(std::wstring const& path)
{
	// TODO: Use a regexp, in cases where we have many many consecutive slashes.
	auto tmp = path;
	tmp = StringReplace(tmp, L"\\", L"/");
	tmp = StringReplace(tmp, L"//", L"/");
	return tmp;

}

std::string WT::Ulits::wtStringUtils::GetPathConcatenation(std::string const& first, std::string const& second)
{
	return first + '/' + second;
}

std::wstring WT::Ulits::wtStringUtils::GetPathConcatenation(std::wstring const& first, std::wstring const& second)
{
	return first + L'/' + second;
}

std::string WT::Ulits::wtStringUtils::GetPathDirectory(std::string const& path)
{
	return Details::GetPathDirectory(path);
}

std::wstring WT::Ulits::wtStringUtils::GetPathDirectory(std::wstring const& path)
{
	return Details::GetPathDirectory(path);
}

std::string WT::Ulits::wtStringUtils::GetPathFileName(std::string const& path)
{
	return Details::GetPathFileName(path);
}

std::wstring WT::Ulits::wtStringUtils::GetPathFileName(std::wstring const& path)
{
	return Details::GetPathFileName(path);
}

std::string WT::Ulits::wtStringUtils::GetPathFileExtension(std::string const& path)
{
	return Details::GetPathFileExtension(path);
}

std::wstring WT::Ulits::wtStringUtils::GetPathFileExtension(std::wstring const& path)
{
	return Details::GetPathFileExtension(path);
}

std::string WT::Ulits::wtStringUtils::GetPathFileExtensionLower(std::string const& path)
{
	return Details::GetPathFileExtensionLower(path);
}

std::wstring WT::Ulits::wtStringUtils::GetPathFileExtensionLower(std::wstring const& path)
{
	return Details::GetPathFileExtension(path);
}

bool WT::Ulits::wtStringUtils::BeginsWith(std::string const& a, char b)
{
	return Details::BeginsWith(a, b);
}

bool WT::Ulits::wtStringUtils::BeginsWith(std::string const& a, std::string const& b)
{
	return Details::BeginsWith(a, b);
}

bool WT::Ulits::wtStringUtils::BeginsWith(std::wstring const& a, wchar_t b)
{
	return Details::BeginsWith(a, b);
}

bool WT::Ulits::wtStringUtils::BeginsWith(std::wstring const& a, std::wstring const& b)
{
	return Details::BeginsWith(a, b);
}

bool WT::Ulits::wtStringUtils::EndsWith(std::string const& a, char b)
{
	return Details::EndsWith(a, b);
}

bool WT::Ulits::wtStringUtils::EndsWith(std::string const& a, std::string const& b)
{
	return Details::EndsWith(a, b);
}

bool WT::Ulits::wtStringUtils::EndsWith(std::wstring const& a, wchar_t b)
{
	return Details::EndsWith(a, b);
}

bool WT::Ulits::wtStringUtils::EndsWith(std::wstring const& a, std::wstring const& b)
{
	return Details::EndsWith(a, b);
}

void WT::Ulits::wtStringUtils::TransformToLower(std::string& str)
{
	return Details::TransformToLower(str);
}

void WT::Ulits::wtStringUtils::TransformToLower(std::wstring& str)
{
	return Details::TransformToLower(str);
}

std::string WT::Ulits::wtStringUtils::ToLower(const std::string& str)
{
	return Details::ToLower(str);
}

std::wstring WT::Ulits::wtStringUtils::ToLower(const std::wstring& str)
{
	return Details::ToLower(str);
}
