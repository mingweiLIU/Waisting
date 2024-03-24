#pragma once

#ifndef NANOID_NAMESPACE
#define NANOID_NAMESPACE nanoid
#endif

#ifdef WTTOOL_Export
#define WTTOOLAPI _declspec(dllexport)
#else
#define WTTOOLAPI _declspec(dllimport)
#endif