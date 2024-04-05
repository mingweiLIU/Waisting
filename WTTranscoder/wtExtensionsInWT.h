#ifndef WTEXTENSIONSINWT_H
#define WTEXTENSIONSINWT_H
#include "WTTranscoderDefines.h"

#include <vector>

WTNAMESPACESTART
TRANSCODERNAMESPACESTART

constexpr const char* WT_MSFTLOD = "MSFT_lod";
constexpr const char* WT_MSTFLOD_IDS_KEY = "ids";

constexpr const char* WT_MSFT_TEXTURE_DDS = "MSFT_texture_dds";
constexpr const char* WT_FILE_EXT_DDS = "dds";
constexpr const char* WT_MIMETYPE_DDS = "image/vnd-ms.dds";

constexpr const char* WT_MSFT_PACKING_NORMALROUGHNESSMETALLIC = "MSFT_packing_normalRoughnessMetallic";
constexpr const char* WT_MSFT_PACKING_OCCLURSIONROUGHTNESSMETALLIC = "MSFT_packing_occlusionRoughnessMetallic";

Microsoft::glTF::ExtensionDeserializer GetMSFTKHRExtensionSerializer();

struct MSFT_lod : Microsoft::glTF::Extension
{
    std::vector<std::string> ids;

    std::unique_ptr<Extension> Clone()const override {
        return std::make_unique<MSFT_lod>(*this);
    }
    bool IsEqual(const Extension& rhs) const override {
        const auto other = dynamic_cast<const MSFT_lod*>(&rhs);
        return other != nullptr && this->ids == other->ids;
    }
};

struct MSFT_texture_dds:Microsoft::glTF::Extension
{
    MSFT_texture_dds(std::string imageID):imageID(std::move(imageID)){}
    std::string imageID;

    std::unique_ptr<Extension> Clone() const override {
        return std::make_unique<MSFT_texture_dds>(*this);
    }
    bool IsEqual(const Extension& other)const override {
        const auto other = dynamic_cast<const MSFT_texture_dds*>(&rhs);
        return other != nullptr && this->imageID == other->imageID;
    }
};

struct MSFT_packing_normalRoughnessMetallic:Microsoft::glTF::Extension
{
    Microsoft::glTF::Material::NormalTextureInfo normalRoughnessMetallicTexture;
    std::unique_ptr<Extension> Clone()const override {
        return std::make_unique<MSFT_packing_normalRoughnessMetallic>(*this);
    }

    bool IsEqual(const Extension& rhs)const override {
        const auto other = dynamic_cast<const MSFT_packing_normalRoughnessMetallic*> (&rhs);
        return other != nullptr && this->normalRoughnessMetallicTexture == other->normalRoughnessMetallicTexture;
    }
};

struct MSFT_packing_occlusionRoughnessMetallic:Microsoft::glTF::Extension
{
	Microsoft::glTF::Material::OcclusionTextureInfo occlusionRoughnessMetallicTexture;
	Microsoft::glTF::Material::OcclusionTextureInfo roughnessMetallicOcclusionTexture;
    Microsoft::glTF::Material::NormalTextureInfo normalTexture;

    std::unique_ptr<Extension> Clone()const override {
        return std::make_unique<MSFT_packing_occlusionRoughnessMetallic>(*this);
    }

    bool IsEqual(const Extension& rhs) const override {
        const auto other = dynamic_cast<const MSFT_packing_occlusionRoughnessMetallic*>(&rhs);
        return other != nullptr
            && this->occlusionRoughnessMetallicTexture == other->occlusionRoughnessMetallicTexture
            && this->roughnessMetallicOcclusionTexture == other->roughnessMetallicOcclusionTexture
            && this->normalTexture == other->normalTexture;
    }
};


TRANSCODERNAMESPACEEND
WTNAMESPACEEND
#endif // WTEXTENSIONSINWT_H
