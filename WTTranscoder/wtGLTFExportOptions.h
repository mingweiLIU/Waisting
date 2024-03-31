#ifndef WTGLTFEXPORTOPTIONS_H
#define WTGLTFEXPORTOPTIONS_H
#include "WTTranscoderDefines.h"
#include "TranscodeOptions.h"

WTNAMESPACESTART
TRANSCODERNAMESPACESTART

constexpr const char* kTranscodingModeHelp =
R"(Determines the export format for Materials:
    Passthrough       Materials are MR (if present) + SG (if present) [default]

    MrOnly            Materials are MR (forced)
    SgOnly            Materials are SG (forced)
    MrSg              Materials are MR+SG (forced)
    SgLods            Materials are SG (forced) + MSFT_lod
    MrSgLods          Materials are MR+SG (forced) + MSFT_lod

    MsftPackingNrm    Materials are MR (forced, no Occlusion/Emissive) + MSFT_packing_normalRoughnessMetallic

    Default           Alias for 'Passthrough'
    Standard          Alias for 'MrOnly'
    Legacy            Alias for 'SgLods'
    Remix             Alias for 'SgLods')";

constexpr const char* kTextureCompressionHelp =
R"(Determines the type of texture compression to export with:
    PNG            Textures are lossless PNG [default]
    PNG256         Textures are 256-color PNG
    JPEGPNG        Textures are JPEG where allowed, otherwise lossless PNG
    JPEGPNG256     Textures are JPEG where allowed, otherwise 256-color PNG)";

enum class TranscodingMode
{
	Passthrough,     // MR (if present) + SG (if present)

	MrOnly,          // MR (forced)
	SgOnly,          //                   SG (forced)
	MrSg,            // MR (forced)     + SG (forced)

	SgLods,          //                   SG (forced)      + MSFT_lod
	MrSgLods,        // MR (forced)     + SG (forced)      + MSFT_lod

	MsftPackingNrm,  // MR (forced, no Occlusion/Emissive) + MSFT_packing_normalRoughnessMetallic
};

enum class ExportTextureMode
{
	// TODO: Passthrough
	PNG,
	PNG256,
	JPEGPNG,
	JPEGPNG256
};

class WTAPI wtGLTFExportOptions :public TranscodeOptions
{
public:
    wtGLTFExportOptions();
    static wtGLTFExportOptions ParseOptions(const std::unordered_map<std::string, std::string>& options);
    void SetTranscodingMode(const std::string& value);
    void SetTextureCompression(const std::string& value);

	TranscodingMode transcodingMode = TranscodingMode::Passthrough;
	ExportTextureMode exportTextureMode = ExportTextureMode::PNG;

};

TRANSCODERNAMESPACEEND
WTNAMESPACEEND
#endif // WTGLTFEXPORTOPTIONS_H
