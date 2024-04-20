#ifndef WTFILEIO_H
#define WTFILEIO_H
#include "wtUlitsDefines.h"
#include <string>
#include <vector>


WTNAMESPACESTART
ULITSNAMESPACESTART

class WTAPI wtFileIO
{
public:
	static bool FileExists(const std::string& fileName);
	static bool FileExists(const std::wstring& fileName);

	static std::vector<std::wstring> GetFilesInDirectory(std::wstring const& path, std::wstring const& filter = L"");
	static std::vector<std::string> GetFilesInDirectory(std::string const& path, std::string const& filter = "");

	static bool MakeDirectory(const std::string& path);
};

ULITSNAMESPACEEND
WTNAMESPACEEND
#endif // WTFILEIO_H
