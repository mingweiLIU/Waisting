#pragma once
#include "WTFrame/WTDefines.h"
#include <functional>

#define TRANSCODERNAMESPACESTART namespace Transcoder{
#define TRANSCODERNAMESPACEEND }
#define USINGTRANSCODERNAMESPACE using namespace WT::Transcoder;

using FractionalProgressCallback = std::function<void(float)>;

#define GLTFVENDER "LiuMingwei"