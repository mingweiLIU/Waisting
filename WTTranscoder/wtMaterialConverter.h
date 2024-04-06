#ifndef WTMATERIALCONVERTER_H
#define WTMATERIALCONVERTER_H

#include <map>
#include <assimp/scene.h>
#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/Document.h>

#include "wtGLTFExportOptions.h"

class wtMaterialConverter
{
public:
	wtMaterialConverter(Microsoft::glTF::Document& glTFDoc, IGLTFWriter& writer, aiScene* oneAiScene, const wtGLTFExportOptions mOptions, const uint8_t scale);
    Microsoft::glTF::Material GetGLTFMat();
protected:
    Microsoft::glTF::Material aiMat2glTFMat(int matIndex,const aiMaterial& aiMat);
    Microsoft::glTF::AlphaMode toGLTFAlphaMode(aiMaterial* oneAiMaterial);
	bool ApplyMaterialMR(glTF::Material& material, IGLTFWriter& writer,aiMaterial* oneAiMaterial,const uint8_t scale);
    void GetMatTex(const aiMaterial& mat, TextureInfo& prop, aiTextureType tt, unsigned int slot = 0);
    void GetMatTex(const aiMaterial& mat, Microsoft::glTF::Texture& texture, unsigned int& texCoord, aiTextureType tt, unsigned int slot = 0);
private:
    Microsoft::glTF::Document& glTFDoc;
    aiScene* oneAiScene;
    IGLTFWriter& writer;
	const uint8_t scale;
    const wtGLTFExportOptions mOptions;
    std::map<std::string, unsigned int> mTexturesByPath;
};

#endif // WTMATERIALCONVERTER_H
