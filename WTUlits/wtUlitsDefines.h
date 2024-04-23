#pragma once
#include "WTFrame/WTDefines.h"

#ifndef NANOID_NAMESPACE
#define NANOID_NAMESPACE nanoid
#endif

#define ULITSNAMESPACESTART namespace Ulits{
#define ULITSNAMESPACEEND }

#define USINGULITSNAMESPACE using namespace WT::Ulits;

#ifdef WTULITS_Export
#define WTULITSAPI _declspec(dllexport)
#else
#define WTULITSAPI _declspec(dllimport)
#endif