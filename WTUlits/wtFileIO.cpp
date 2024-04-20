#include "wtFileIO.h"
#include <fstream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <string>
#include <stack>
#include <direct.h>

#include "wtLexicalCast.h"
#include "wtStringUtils.h"
#include "wtSmartThrow.h"
USINGULITSNAMESPACE

#if !defined(S_ISDIR)
#  if defined( _S_IFDIR) && !defined( __S_IFDIR)
#    define __S_IFDIR _S_IFDIR
#  endif
#  define S_ISDIR(mode)    (mode&__S_IFDIR)
#endif

#define mkdir(x,y) _mkdir((x))

#define stat64 _stati64

bool wtFileIO::FileExists(const std::string& fileName)
{
	return FileExists(LexicalCast<std::wstring>(fileName));
}

bool wtFileIO::FileExists(const std::wstring& fileName)
{
	auto attr = ::GetFileAttributesW(fileName.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES);
}


std::vector<std::shared_ptr<WIN32_FIND_DATAW>> GetFileSystemEntries(std::wstring const& path)
{
	WIN32_FIND_DATAW fileData;
	HANDLE fileHandle = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;

	if (path.length() > (MAX_PATH - 3))
	{
		throw WTException(
			"Directory path is too long.", false);
	}

	std::wstring directorySearch = path;
	directorySearch.append(L"\\*");

	fileHandle = FindFirstFileW(directorySearch.c_str(), &fileData);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		dwError = GetLastError();
		throw WTException(dwError,
			"FindFirstFile returned INVALID_HANDLE_VALUE for " +
			LexicalCast<std::string>(path), false);
	}

	std::vector<std::shared_ptr<WIN32_FIND_DATAW>> result;
	do
	{
		WIN32_FIND_DATAW data = fileData;
		result.push_back(std::make_shared<WIN32_FIND_DATAW>(data));
	} while (FindNextFileW(fileHandle, &fileData) != NULL);

	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		throw WTException(dwError,
			"FindNextFile encountered an unexpected error wihle searching " +
			LexicalCast<std::string>(path), false);
	}

	FindClose(fileHandle);
	return result;
}

std::vector<std::wstring> WT::Ulits::wtFileIO::GetFilesInDirectory(std::wstring const& path, std::wstring const& filter /*= L""*/)
{
	std::vector<std::wstring> result;

	// Path normalisation
	std::wstring normPath = path;
	while (normPath.at(normPath.size() - 1) == '\\')
	{
		normPath.pop_back();
	}

	auto entries = GetFileSystemEntries(path);
	for (auto entry : entries)
	{
		if (!(entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			auto file = std::wstring(path + L"\\" + entry->cFileName);
			if (filter.length() != 0 && file.find(filter) != std::wstring::npos)
			{
				result.push_back(file);
			}
		}
	}

	return result;
}

std::vector<std::string> WT::Ulits::wtFileIO::GetFilesInDirectory(std::string const& path, std::string const& filter /*= ""*/)
{
	std::vector<std::wstring> wFiles = GetFilesInDirectory(wtStringUtils::StringToWString(path), wtStringUtils::StringToWString(filter));
	std::vector<std::string> files; files.reserve(wFiles.size());
	for (auto& oneFileName:wFiles)
	{
		std::string oneStr=wtStringUtils::WStringToString(oneFileName);
		files.push_back(oneStr);
	}
	return files;
}

bool WT::Ulits::wtFileIO::MakeDirectory(const std::string& path)
{
	if (path.empty())
	{
		return false;
	}

	struct stat64 stbuf;

	if (stat64(path.c_str(), &stbuf) == 0)
	{
		if (S_ISDIR(stbuf.st_mode))
			return true;
		else
		{
			return false;
		}
	}

	std::stack<std::string> paths;
	for (std::string dir = path;
		!dir.empty();
		dir = wtStringUtils::GetPathDirectory(dir))
	{

		if (stat64(dir.c_str(), &stbuf) < 0)
		{
			switch (errno)
			{
			case ENOENT:
			case ENOTDIR:
				paths.push(dir);
				break;

			default:
				return false;
			}
		}
	}

	while (!paths.empty())
	{
		std::string dir = paths.top();

#if defined(WIN32)
		//catch drive name
		if (dir.size() == 2 && dir.c_str()[1] == ':') {
			paths.pop();
			continue;
		}
#endif


		if (mkdir(dir.c_str(), 0755) < 0)
		{
			// Only return an error if the directory actually doesn't exist.  It's possible that the directory was created
			// by another thread or process
			if (!FileExists(dir))
			{
				return false;
			}
		}
		paths.pop();
	}
	return true;
}
