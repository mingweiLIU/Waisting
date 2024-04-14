/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#pragma once

#if !defined(COREUTILS_STRINGUTILS_H_INCLUDED)
#define COREUTILS_STRINGUTILS_H_INCLUDED
#include "wtUlitsDefines.h"

#include <string>
#include <vector>

#ifndef _MSC_VER
#include <cstdarg>
#endif

WTNAMESPACESTART
ULITSNAMESPACESTART
/**
    * Converts an UTF-8 string to a wide character string.
    */
std::wstring StringToWString(const char* s, size_t size);

/**
    * Converts an UTF-8 string to a wide character string.
    */
std::wstring StringToWString(const std::string& s);

/**
    * Converts a wide character string to an UTF-8 string.
    */
std::string WStringToString(const wchar_t* w, size_t size);

/**
    * Converts a wide character string to an UTF-8 string.
    */
std::string WStringToString(const std::wstring& w);

/**
    * Trims leading whitespace from a string.
    */
std::string LTrim(std::string str);

/**
    * Trims leading whitespace from a string.
    */
std::wstring LTrim(std::wstring str);

/**
    * Trims trailing from a string.
    */
std::string RTrim(std::string str);

/**
    * Trims trailing from a string.
    */
std::wstring RTrim(std::wstring str);

/**
    * Trims leading and trailing whitespace from a string.
    */
std::string Trim(std::string str);

/**
    * Trims leading and trailing whitespace from a string.
    */
std::wstring Trim(std::wstring str);

/**
    * Splits up a string given a delimeter.
    */
std::vector<std::string> Split(const std::string& string, const std::string& delimiter);

/**
    * Splits up a string given a delimeter.
    */
std::vector<std::wstring> Split(const std::wstring& string, const std::wstring& delimiter);

/**
    * Splits up a string given a regex.
    */
std::vector<std::string> SplitRegex(const std::string& string, const std::string& regex);

/**
    * Splits up a string given a regex.
    */
std::vector<std::wstring> SplitRegex(const std::wstring& string, const std::wstring& regex);

/**
    * Replaces all instances of a substring with another in a string.
    */
std::string StringReplace(std::string const& string, std::string const& what, std::string const& with);

/**
    * Replaces all instances of a substring with another in a string.
    */
std::wstring StringReplace(std::wstring const& string, std::wstring const& what, std::wstring const& with);

bool IsPathAbsolute(const std::string& path);
bool IsPathAbsolute(const std::wstring& path);

bool IsPathRelative(const std::string& path);
bool IsPathRelative(const std::wstring& path);

std::string GetPathConsistentSlashes(std::string const& path);
std::wstring GetPathConsistentSlashes(std::wstring const& path);

std::string GetPathConcatenation(std::string const& first, std::string const& second);
std::wstring GetPathConcatenation(std::wstring const& first, std::wstring const& second);

std::string GetPathDirectory(std::string const& path);
std::wstring GetPathDirectory(std::wstring const& path);

std::string GetPathFileName(std::string const& path);
std::wstring GetPathFileName(std::wstring const& path);

std::string GetPathFileExtension(std::string const& path);
std::wstring GetPathFileExtension(std::wstring const& path);

std::string GetPathFileExtensionLower(std::string const& path);
std::wstring GetPathFileExtensionLower(std::wstring const& path);

bool BeginsWith(std::string const &a, char b);
bool BeginsWith(std::string const &a, std::string const &b);
bool BeginsWith(std::wstring const &a, wchar_t b);
bool BeginsWith(std::wstring const &a, std::wstring const &b);

bool EndsWith(std::string const &a, char b);
bool EndsWith(std::string const &a, std::string const &b);
bool EndsWith(std::wstring const &a, wchar_t b);
bool EndsWith(std::wstring const &a, std::wstring const &b);

void TransformToLower(std::string &str);
void TransformToLower(std::wstring &str);

std::string ToLower(const std::string& str);
std::wstring ToLower(const std::wstring& str);

#ifndef _MSC_VER
int BabylonVSNPrintf_s(char* const _Buffer, size_t const _BufferCount, size_t const _MaxCount, char const* const _Format, va_list _ArgList);

inline int BabylonSPrintf_s(char* const _Buffer, size_t const _BufferCount, char const* const _Format, ...)
{
    va_list args;
    va_start(args, _Format);
    int result = BabylonVSNPrintf_s(_Buffer, _BufferCount, _BufferCount - 1, _Format, args);
    va_end(args);
    return result;
}

template <size_t _Size> inline int BabylonSPrintf_s(char(&_Buffer)[_Size], char const* const _Format, ...)
{
    va_list args;
    va_start(args, _Format);
    int result = BabylonVSNPrintf_s(_Buffer, _Size, _Size - 1, _Format, args);
    va_end(args);
    return result;
}

static const size_t Babylon_TRUNCATE = static_cast<size_t>(-1);
#endif // #ifndef _MSC_VER


#ifndef _MSC_VER
#define _TRUNCATE Babylon::Utils::Babylon_TRUNCATE
#define sprintf_s Babylon::Utils::BabylonSPrintf_s
#define vsnprintf_s Babylon::Utils::BabylonVSNPrintf_s
#define strtok_s strtok_r
#endif

#endif // COREUTILS_STRINGUTILS_H_INCLUDED

ULITSNAMESPACEEND
WTNAMESPACEEND