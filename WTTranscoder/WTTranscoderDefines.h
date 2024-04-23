#pragma once
#include "WTFrame/WTDefines.h"



#define TRANSCODERNAMESPACESTART namespace Transcoder{
#define TRANSCODERNAMESPACEEND }

#define USINGTRANSCODERNAMESPACE using namespace WT::Transcoder;

#ifdef WTTRANSCODER_Export
#define WTTRANSCODERAPI _declspec(dllexport)
#else
#define WTTRANSCODERAPI _declspec(dllimport)
#endif