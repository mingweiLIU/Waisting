/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#pragma once
#include "wtUlitsDefines.h"

#include <string>
#include <vector>


WTNAMESPACESTART
ULITSNAMESPACESTART

class WTULITSAPI wtStringUtils {
public:
    /**
    * Converts an UTF-8 string to a wide character string.
    */
    static std::wstring StringToWString(const char* s, size_t size);

    /**
    * Converts an UTF-8 string to a wide character string.
    */
    static std::wstring StringToWString(const std::string& s);

    /**
    * Converts a wide character string to an UTF-8 string.
    */
    static std::string WStringToString(const wchar_t* w, size_t size);

    /**
    * Converts a wide character string to an UTF-8 string.
    */
    static std::string WStringToString(const std::wstring& w);

    /**
    * Trims leading whitespace from a string.
    */
    static std::string LTrim(std::string str);


    /**
	* Trims leading whitespace from a string.
	*/
    static std::wstring LTrim(std::wstring str);


    /**
    * Trims trailing from a string.
    */
    static std::string RTrim(std::string str);

    /**
    * Trims trailing from a string.
    */
    static std::wstring RTrim(std::wstring str);

    /**
    * Trims leading and trailing whitespace from a string.
    */
    static std::string Trim(std::string str);

    /**
    * Trims leading and trailing whitespace from a string.
    */
    static std::wstring Trim(std::wstring str);

    /**
    * Splits up a string given a delimeter.
    */
    static std::vector<std::string> Split(const std::string& string, const std::string& delimiter);

    /**
    * Splits up a string given a delimeter.
    */
    static std::vector<std::wstring> Split(const std::wstring& string, const std::wstring& delimiter);

    /**
    * Splits up a string given a regex.
    */
    static std::vector<std::string> SplitRegex(const std::string& string, const std::string& regex);

    /**
    * Splits up a string given a regex.
    */
    static std::vector<std::wstring> SplitRegex(const std::wstring& string, const std::wstring& regex);

    /**
    * Replaces all instances of a substring with another in a string.
    */
    static std::string StringReplace(std::string const& string, std::string const& what, std::string const& with);

    /**
    * Replaces all instances of a substring with another in a string.
    */
    static std::wstring StringReplace(std::wstring const& string, std::wstring const& what, std::wstring const& with);

	static bool IsPathAbsolute(const std::string& path);
	static bool IsPathAbsolute(const std::wstring& path);

    static bool IsPathRelative(const std::string& path);
    static bool IsPathRelative(const std::wstring& path);

    static std::string GetPathConsistentSlashes(std::string const& path);
	static std::wstring GetPathConsistentSlashes(std::wstring const& path);

    static std::string GetPathConcatenation(std::string const& first, std::string const& second);
	static std::wstring GetPathConcatenation(std::wstring const& first, std::wstring const& second);

    static std::string GetPathDirectory(std::string const& path);
	static std::wstring GetPathDirectory(std::wstring const& path);

    static std::string GetPathFileName(std::string const& path);
	static std::wstring GetPathFileName(std::wstring const& path);

    static std::string GetPathFileExtension(std::string const& path);
	static std::wstring GetPathFileExtension(std::wstring const& path);

    static std::string GetPathFileExtensionLower(std::string const& path);
    static std::wstring GetPathFileExtensionLower(std::wstring const& path);


    static bool BeginsWith(std::string const& a, char b);
    static bool BeginsWith(std::string const& a, std::string const& b);
    static bool BeginsWith(std::wstring const& a, wchar_t b);
	static bool BeginsWith(std::wstring const& a, std::wstring const& b);

    static bool EndsWith(std::string const& a, char b);
    static bool EndsWith(std::string const& a, std::string const& b);
    static bool EndsWith(std::wstring const& a, wchar_t b);
	static bool EndsWith(std::wstring const& a, std::wstring const& b);

    static void TransformToLower(std::string& str);
    static void TransformToLower(std::wstring& str);

    static std::string ToLower(const std::string& str);
    static std::wstring ToLower(const std::wstring& str);
};

ULITSNAMESPACEEND
WTNAMESPACEEND