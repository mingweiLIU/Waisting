#include "wtGLTFExportOptions.h"

USINGTRANSCODERNAMESPACE

WT::Transcoder::wtGLTFExportOptions::wtGLTFExportOptions() :TranscodeOptions()
{
	AddOption({ "Mode", kTranscodingModeHelp,[this](const std::string& value) { SetTranscodingMode(value); } });
	AddOption({ "TextureCompression", kTextureCompressionHelp,[this](const std::string& value) { SetTextureCompression(value); } });
}

GLTFExportOptions wtGLTFExportOptions::ParseOptions(const std::unordered_map<std::string, std::string>& options)
{
	return WT::Transcoder::ParseOptions<wtGLTFExportOptions>(options);
}

void wtGLTFExportOptions::SetTranscodingMode(const std::string& value)
{
	if (value == "passthrough") { TranscodingMode = TranscodingMode::Passthrough; }
	else if (value == "mronly") { TranscodingMode = TranscodingMode::MrOnly; }
	else if (value == "sgonly") { TranscodingMode = TranscodingMode::SgOnly; }
	else if (value == "mrsg") { TranscodingMode = TranscodingMode::MrSg; }
	else if (value == "sglods") { TranscodingMode = TranscodingMode::SgLods; }
	else if (value == "mrsglods") { TranscodingMode = TranscodingMode::MrSgLods; }
	else if (value == "msftpackingnrm") { TranscodingMode = TranscodingMode::MsftPackingNrm; }
	// Aliases:
	else if (value == "default") { TranscodingMode = TranscodingMode::Passthrough; }
	else if (value == "standard") { TranscodingMode = TranscodingMode::MrOnly; }
	else if (value == "legacy") { TranscodingMode = TranscodingMode::SgLods; }
	else if (value == "remix") { TranscodingMode = TranscodingMode::SgLods; }
	else { throw Utils::BabylonException("Unrecognized value for 'Mode': " + value); }
}


void WT::Transcoder::wtGLTFExportOptions::SetTextureCompression(const std::string& value)
{
	if (value == "png") { TextureCompression = ExportTextureMode::PNG; }
	else if (value == "png256") { TextureCompression = ExportTextureMode::PNG256; }
	else if (value == "jpegpng") { TextureCompression = ExportTextureMode::JPEGPNG; }
	else if (value == "jpegpng256") { TextureCompression = ExportTextureMode::JPEGPNG256; }
	// Aliases:
	else if (value == "none") { TextureCompression = ExportTextureMode::PNG; }
	else { throw Utils::BabylonException("Unrecognized value for 'TextureCompression': " + value); }
}

