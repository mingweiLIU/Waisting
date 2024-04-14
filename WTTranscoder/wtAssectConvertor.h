#ifndef WTASSECTCONVERTOR_H
#define WTASSECTCONVERTOR_H
#include "WTTranscoderDefines.h"
#include <assimp/scene.h>

WTNAMESPACESTART
TRANSCODERNAMESPACESTART

class WTAPI wtAssectConvertor
{
public:
    wtAssectConvertor(aiScene* assets );

private:
    aiScene* mAssets;
};

TRANSCODERNAMESPACEEND
WTNAMESPACEEND
#endif // WTASSECTCONVERTOR_H
