#include "wtMaterialConverter.h"

wtMaterialConverter::wtMaterialConverter(Microsoft::glTF::Document& glTFDoc, IGLTFWriter& writer, aiScene* oneAiScene,  const wtGLTFExportOptions mOptions, const uint8_t scale)
	: oneAiScene(oneAiScene)
	, scale(scale)
	,writer(writer)
	, mOptions(mOptions)
	,glTFDoc(glTFDoc)
{

	for (size_t i = 0; i < oneAiScene->mNumMaterials; ++i)
	{
		if (oneAiScene->mMaterialsp[i]==nullptr) continue;

		const aiMaterial& aiMat = *(oneAiScene->mMaterials[i]);
		aiMat2glTFMat(aiMat);
	}

	
}


Microsoft::glTF::Material wtMaterialConverter::aiMat2glTFMat(int matIndex, const aiMaterial& aiMat)
{
	Microsoft::glTF::Material material;
	glTFMat.id = std::to_string(matIndex);
	if (scale != 1)
	{
		glTFMat.id += "_" + std::to_string(scale);
	}
	glTFMat.name = oneAiMaterial->GetName().C_Str();

	//检查是否是混合
	glTFMat.alphaMode = toGLTFAlphaMode(oneAiMaterial);

	oneAiMaterial->Get(AI_MATKEY_TWOSIDED, glTFMat.doubleSided);//双面渲染
	oneAiMaterial->Get(AI_MATKEY_GLTF_ALPHACUTOFF, glTFMat.alphaCutoff);

	switch (mOptions.transcodingMode)
	{
	case TranscodingMode::Passthrough:
	{
		bool mr = ApplyMaterialMR(material, writer,
			materialDesc->GetLayer(LayerType::kBaseColor), materialDesc->GetLayer(LayerType::kMetallicRoughness), scale);
		bool sg = ApplyMaterialSG(material, writer,
			materialDesc->GetLayer(LayerType::kDiffuse), materialDesc->GetLayer(LayerType::kSpecularGlossiness), scale);

		if (sg)
		{
			// SG provided, therefore KHR_materials_pbrSpecularGlossiness goes in extensionsUsed
			m_gltfDocument->extensionsUsed.insert(glTF::KHR::Materials::PBRSPECULARGLOSSINESS_NAME);

			if (!mr)
			{
				// MR not provided, therefore KHR_materials_pbrSpecularGlossiness goes in extensionsRequired
				m_gltfDocument->extensionsRequired.insert(glTF::KHR::Materials::PBRSPECULARGLOSSINESS_NAME);
			}
		}
		break;
	}

	case TranscodingMode::MrOnly:
		ApplyOrGenerateMaterialMR(material, writer, materialDesc, scale);
		break;

	case TranscodingMode::SgOnly:
	case TranscodingMode::SgLods:
		ApplyOrGenerateMaterialSG(material, writer, materialDesc, scale);

		// Only SG provided, therefore KHR_materials_pbrSpecularGlossiness goes in extensionsUsed & extensionsRequired
		m_gltfDocument->extensionsUsed.insert(glTF::KHR::Materials::PBRSPECULARGLOSSINESS_NAME);
		m_gltfDocument->extensionsRequired.insert(glTF::KHR::Materials::PBRSPECULARGLOSSINESS_NAME);
		break;

	case TranscodingMode::MrSg:
	case TranscodingMode::MrSgLods:
		ApplyOrGenerateMaterialMR(material, writer, materialDesc, scale);
		ApplyOrGenerateMaterialSG(material, writer, materialDesc, scale);

		// Both MR & SG provided, therefore KHR_materials_pbrSpecularGlossiness goes in extensionsUsed
		m_gltfDocument->extensionsUsed.insert(glTF::KHR::Materials::PBRSPECULARGLOSSINESS_NAME);
		break;

	case TranscodingMode::MsftPackingNrm:
		ApplyMsftPackingNrm(material, writer, materialDesc);
		break;

	default:
		throw WT::Ulits::WTException("Unrecognized TranscodingMode");
	}
}

Microsoft::glTF::AlphaMode wtMaterialConverter::toGLTFAlphaMode(aiMaterial* oneAiMaterial) {
	float opacity;
	aiString alphaMode;

	if (oneAiMaterial->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
		if (opacity < 1) {
			return Microsoft::glTF::ALPHA_BLEND;
		}
	}

	if (oneAiMaterial->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
		std::unordered_map<std::string, Microsoft::glTF::AlphaMode> gltfMatAlphaMode = {
			{"OPAQUE",Microsoft::glTF::ALPHA_OPAQUE},
			{"BLEND",Microsoft::glTF::ALPHA_BLEND},
			{"MASK",Microsoft::glTF::ALPHA_MASK},
		};
		return gltfMatAlphaMode[alphaMode.C_Str()];
	}

	return Microsoft::glTF::ALPHA_OPAQUE;//默认
}

bool wtMaterialConverter::ApplyMaterialMR(glTF::Material& material, IGLTFWriter& writer, aiMaterial* oneAiMaterial, const uint8_t scale)
{

}

void wtMaterialConverter::GetMatTex(const aiMaterial& mat, TextureInfo& prop, aiTextureType tt, unsigned int slot /*= 0*/)
{
	Ref<Texture>& texture = prop.texture;
	GetMatTex(mat, texture, prop.texCoord, tt, slot);
}

void wtMaterialConverter::GetMatTex(const aiMaterial& mat, Microsoft::glTF::Texture& texture, unsigned int& texCoord, aiTextureType tt, unsigned int slot /*= 0*/)
{
	if (mat.GetTextureCount(tt) == 0) return;

	aiString tex; 
	mat.Get(AI_MATKEY_UVWSRC(tt, slot), texCoord);

	if (mat.Get(AI_MATKEY_TEXTURE(tt,slot),tex)==AI_SUCCESS)
	{
		std::string path = tex.C_Str();

		if (path.size()>0)
		{
			bool hasKey = glTFDoc.textures.Has(path);
			if (hasKey)
			{
				texture = glTFDoc.textures.Get(path);
			}

			bool useBasisUniversal = false;
			if (!texture)
			{
				std::string texID=
			}
		}
	}
}
