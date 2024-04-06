#include "wtExtensionsInWT.h"
//因为如果使用的rapidjson和gltfSDK的不一致会导致错误 这里就不对外暴露使用rapidjson 所以就在cpp中申明函数
#include <GLTFSDK/RapidJsonUtils.h>
#include <GLTFSDK/Document.h>
#include <GLTFSDK/ExtensionsKHR.h>

USINGTRANSCODERNAMESPACE

std::string SerializeMSFT_Lod(const MSFT_lod& msftLod, const Microsoft::glTF::Document& gltfDocument, const Microsoft::glTF::ExtensionSerializer&) {
	Microsoft::glTF::rapidjson::Document doc;
	auto& a = doc.GetAllocator();

	std::vector<size_t> lodIndices;
	lodIndices.reserve(msftLod.ids.size());
	for (const auto& lodID:msftLod.ids)
	{
		lodIndices.push_back(Microsoft::glTF::ToKnownSizeType(gltfDocument.materials.GetIndex(lodID));
	}

	Microsoft::glTF::rapidjson::Value MSFT_lod_Value(Microsoft::glTF::rapidjson::kObjectType);
	{
		MSFT_lod_Value.AddMember(Microsoft::glTF::RapidJsonUtils::ToStringValue(WT_MSTFLOD_IDS_KEY, a), Microsoft::glTF::RapidJsonUtils::ToJsonArray(lodIndices, a), a);
	}

	Microsoft::glTF::rapidjson::StringBuffer buffer;
	Microsoft::glTF::rapidjson::Writer<Microsoft::glTF::rapidjson::StringBuffer> writer(buffer);
	MSFT_lod_Value.Accept(writer);
	return buffer.GetString();
}


std::string SerializeMSFT_texture_dds(const MSFT_texture_dds& msftTextureDDS, const Microsoft::glTF::Document& gltfDocument, const Microsoft::glTF::ExtensionSerializer&) {
	Microsoft::glTF::rapidjson::Document doc;
	auto& a = doc.GetAllocator();

	auto source = Microsoft::glTF::RapidJsonUtils::ToStringValue("source", a);
	auto imageIndex = Microsoft::glTF::ToKnownSizeType(gltfDocument.images.GetIndex(msftTextureDDS.imageID));

	Microsoft::glTF::rapidjson::Value MSFT_texture_dds_Value(Microsoft::glTF::rapidjson::kObjectType);
	{
		MSFT_texture_dds_Value.AddMember(source, imageIndex, a);
	}

	Microsoft::glTF::rapidjson::StringBuffer buffer;
	Microsoft::glTF::rapidjson::Writer<Microsoft::glTF::rapidjson::StringBuffer>writer(buffer);
	MSFT_texture_dds_Value.Accept(writer);
	return buffer.GetString();
}

void SerializeTextureInfo(const Microsoft::glTF::Document& doc, const Microsoft::glTF::TextureInfo& textureInfo, Microsoft::glTF::rapidjson::Value& textureValue, Microsoft::glTF::rapidjson::Document::AllocatorType a) {
	Microsoft::glTF::RapidJsonUtils::AddOptionalMemberIndex("index", textureValue, textureInfo.textureId, doc.textures, a);
	if (textureInfo.texCoord!=0)
	{
		textureValue.AddMember("texCoord", Microsoft::glTF::ToKnownSizeType(textureInfo.texCoord), a);
	}
}

std::string SerializeMSFT_packing_normalRoughnessMetallic(const MSFT_packing_normalRoughnessMetallic& msftPackingNRM, const Microsoft::glTF::Document& gltfDocument, const Microsoft::glTF::ExtensionSerializer&) {

	// TODO: Recursive serialization (if this extension has more extensions on top of it)
	Microsoft::glTF::rapidjson::Document doc;
	auto& a = doc.GetAllocator();

	Microsoft::glTF::rapidjson::Value MSFT_packing_normalRoughnessMetallic_Value(Microsoft::glTF::rapidjson::kObjectType);
	{
		if (!msftPackingNRM.normalRoughnessMetallicTexture.textureId.empty())
		{
			Microsoft::glTF::rapidjson::Value normalRoughnessMetallicTexture_Value(Microsoft::glTF::rapidjson::kObjectType); {
				SerializeTextureInfo(gltfDocument, msftPackingNRM.normalRoughnessMetallicTexture, normalRoughnessMetallicTexture_Value, a);
				if (msftPackingNRM.normalRoughnessMetallicTexture.scale!=1.0f)
				{
					normalRoughnessMetallicTexture_Value.AddMember("scale", msftPackingNRM.normalRoughnessMetallicTexture.scale, a);
				}
				MSFT_packing_normalRoughnessMetallic_Value.AddMember("normalRoughnessMetallicTexture", normalRoughnessMetallicTexture, a);
			}
		}
	}
	Microsoft::glTF::rapidjson::StringBuffer buffer;
	Microsoft::glTF::rapidjson::Writer<Microsoft::glTF::rapidjson::StringBuffer> writer(buffer);
	MSFT_packing_normalRoughnessMetallic_Value.Accept(writer);
	return buffer->GetString();
}

std::string SerializeMSFT_packing_occlusionRoughnessMetallic(const MSFT_packing_occlusionRoughnessMetallic& msftPackingORM, const Microsoft::glTF::Document& gltfDocument, const Microsoft::glTF::ExtensionSerializer&) {

	// TODO: Recursive serialization (if this extension has more extensions on top of it)
	glTF::rapidjson::Document doc;
	auto& a = doc.GetAllocator();

	glTF::rapidjson::Value MSFT_packing_occlusionRoughnessMetallic(glTF::rapidjson::kObjectType);
	{
		if (!msftPackingORM.occlusionRoughnessMetallicTexture.textureId.empty())
		{
			glTF::rapidjson::Value occlusionRoughnessMetallicTexture(glTF::rapidjson::kObjectType);
			{
				SerializeTextureInfo(gltfDocument, msftPackingORM.occlusionRoughnessMetallicTexture, occlusionRoughnessMetallicTexture, a);
				if (msftPackingORM.occlusionRoughnessMetallicTexture.strength != 1.0f)
				{
					occlusionRoughnessMetallicTexture.AddMember("strength", msftPackingORM.occlusionRoughnessMetallicTexture.strength, a);
				}
				MSFT_packing_occlusionRoughnessMetallic.AddMember("occlusionRoughnessMetallicTexture", occlusionRoughnessMetallicTexture, a);
			}
		}

		if (!msftPackingORM.roughnessMetallicOcclusionTexture.textureId.empty())
		{
			glTF::rapidjson::Value roughnessMetallicOcclusionTexture(glTF::rapidjson::kObjectType);
			{
				SerializeTextureInfo(gltfDocument, msftPackingORM.roughnessMetallicOcclusionTexture, roughnessMetallicOcclusionTexture, a);
				if (msftPackingORM.roughnessMetallicOcclusionTexture.strength != 1.0f)
				{
					roughnessMetallicOcclusionTexture.AddMember("strength", msftPackingORM.roughnessMetallicOcclusionTexture.strength, a);
				}
				MSFT_packing_occlusionRoughnessMetallic.AddMember("roughnessMetallicOcclusionTexture", roughnessMetallicOcclusionTexture, a);
			}
		}

		if (!msftPackingORM.normalTexture.textureId.empty())
		{
			glTF::rapidjson::Value normalTexture(glTF::rapidjson::kObjectType);
			{
				SerializeTextureInfo(gltfDocument, msftPackingORM.normalTexture, normalTexture, a);
				if (msftPackingORM.normalTexture.scale != 1.0f)
				{
					normalTexture.AddMember("scale", msftPackingORM.normalTexture.scale, a);
				}
				MSFT_packing_occlusionRoughnessMetallic.AddMember("normalTexture", normalTexture, a);
			}
		}
	}

	glTF::rapidjson::StringBuffer buffer;
	glTF::rapidjson::Writer<glTF::rapidjson::StringBuffer> writer(buffer);
	MSFT_packing_occlusionRoughnessMetallic.Accept(writer);
	return buffer.GetString();
}

Microsoft::glTF::ExtensionSerializer WT::Transcoder::GetMSFTKHRExtensionSerializer()
{
	auto extensionSerializer = Microsoft::glTF::KHR::GetKHRExtensionSerializer();
	extensionSerializer.AddHandler<MSFT_lod, Microsoft::glTF::Material>(WT_MSFTLOD, SerializeMSFT_Lod);
	extensionSerializer.AddHandler<MSFT_texture_dds, Microsoft::glTF::Texture>(WT_MSFT_TEXTURE_DDS, SerializeMSFT_texture_dds);
	extensionSerializer.AddHandler<MSFT_packing_normalRoughnessMetallic, Microsoft::glTF::Material>(WT_MSFT_PACKING_NORMALROUGHNESSMETALLIC, SerializeMSFT_packing_normalRoughnessMetallic);
	extensionSerializer.AddHandler<MSFT_packing_occlusionRoughnessMetallic, Microsoft::glTF::Material>(WT_MSFT_PACKING_OCCLURSIONROUGHTNESSMETALLIC, SerializeMSFT_packing_occlusionRoughnessMetallic);
	return extensionSerializer;
}
