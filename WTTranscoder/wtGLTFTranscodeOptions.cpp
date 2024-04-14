#include "wtGLTFTranscodeOptions.h"
#include "WTUlits/wtSmartThrow.h"


WT::Transcoder::wtGLTFTranscodeOptions::wtGLTFTranscodeOptions()
{
	AddOption({ "fileExt","设置输出格式",[this](const std::string& value) {setFileExt(value); } });
	AddOption({ "normalInfo","设置是否输出法线",[this](const std::string& value) {setNormalInfo(value); } });
	AddOption({ "textureCompression","设置纹理压缩格式",[this](const std::string& value) {setTextureCompressiong(value); } });
	AddOption({ "dracoUsing","设置是否使用法线",[this](const std::string& value) {setNormalInfo(value); } });
	AddOption({ "doubleSide","设置是否双面渲染",[this](const std::string& value) {setDoubleSide(value); } });
}

WT::Transcoder::wtGLTFTranscodeOptions::wtGLTFTranscodeOptions WT::Transcoder::wtGLTFTranscodeOptions::ParaseOptions(const std::unordered_map<std::string, std::string>& options)
{
	return Transcoder::ParseOptions<GLTFExportOptions>(options);
}

USINGTRANSCODERNAMESPACE

void WT::Transcoder::wtGLTFTranscodeOptions::setFileExt(const std::string& value)
{
	std::unordered_map<std::string, GLTFFileExt> gltfFileExt = {
		{"gltf",GLTFFileExt::GLTF},
		{"glb",GLTFFileExt::GLB},
		{"gltf2",GLTFFileExt::GLTF2},
		{"glb2",GLTFFileExt::GLB2}
	};
	glTFExt = gltfFileExt.at(value);
	if (!glTFExt)
	{
		throw WTInvalidArgException("不支持的格式:" + value);
	}
}

void WT::Transcoder::wtGLTFTranscodeOptions::setNormalInfo(const std::string& value)
{
	std::unordered_map<std::string, NormalInfo> normalInfos = {
		{"origin",NormalInfo::ORIGIN},
		{"generate",NormalInfo::GENERATE},
		{"none",NormalInfo::NONE}
	};
	nornalInfo = normalInfos[value];
	if (!nornalInfo)
	{
		throw WTInvalidArgException("不法线设置形式: " + value);
	}
}

void WT::Transcoder::wtGLTFTranscodeOptions::setTextureCompressiong(const std::string& value)
{
	std::unordered_map<std::string, TextureCompression> textureCompressions = {
		{"origin",TextureCompression::ORIGIN},
		{"png",TextureCompression::PNG},
		{"jpg",TextureCompression::JPG},
		{"webp",TextureCompression::WEBP},
		{"basis",TextureCompression::BASIS}
	};
	textureCompression = textureCompressions[value];
	if (!textureCompression)
	{
		throw WTInvalidArgException("不支持的纹理压缩形式: " + value);
	}
}

void WT::Transcoder::wtGLTFTranscodeOptions::setDracoUsing(const std::string& value)
{
	std::unordered_map<std::string, DracoUsing> dracoUsings = {
		{"no",DracoUsing::NO},
		{"yes",DracoUsing::YES}
	};
	dracoUsing = dracoUsings[value];
	if (!dracoUsing)
	{
		throw WTInvalidArgException("不支持draco设置形式: " + value);
	}
}

void WT::Transcoder::wtGLTFTranscodeOptions::setDoubleSide(const std::string& value)
{
	std::unordered_map<std::string, DoubleSide> doubleSides = {
		{"no",DoubleSide::NO},
		{"yes",DoubleSide::YES}
	};
	doubleSide = doubleSides[value];
	if (!doubleSide)
	{
		throw WTInvalidArgException("不支持三角面双面渲染与否设置形式: " + value);
	}
}
