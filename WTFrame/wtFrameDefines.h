#ifndef WTFRAMEDEFINES_H
#define WTFRAMEDEFINES_H
#include "WTDefines.h"

#define FRAMENAMESPACESTART namespace Frame{
#define FRAMENAMESPACEEND }
#define USINGFRAMENAMESPACE using namespace WT::Frame;

#ifdef WTFRAME_Export
#define WTFRAMEAPI _declspec(dllexport)
#else
#define WTFRAMEAPI _declspec(dllimport)
#endif

#endif // WTFRAMEDEFINES_H
