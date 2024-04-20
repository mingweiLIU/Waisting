#include "wtGLTFTranscodeOptions.h"
#include "WTUlits/wtSmartThrow.h"

USINGTRANSCODERNAMESPACE

WT::Transcoder::wtGLTFTranscodeOptions::wtGLTFTranscodeOptions()
{
	AddOption({ "fileExt","设置输出格式",[this](const std::string& value) {setFileExt(value); } });
	AddOption({ "normalInfo","设置是否输出法线",[this](const std::string& value) {setNormalInfo(value); } });
	AddOption({ "textureCompression","设置纹理压缩格式",[this](const std::string& value) {setTextureCompressiong(value); } });
	AddOption({ "dracoUsing","设置是否使用法线",[this](const std::string& value) {setNormalInfo(value); } });
	AddOption({ "doubleSide","设置是否双面渲染",[this](const std::string& value) {setDoubleSide(value); } });
	AddOption({ "outpath","输出路径",[this](const std::string& value) {setOutputPath(value); } });
}

wtGLTFTranscodeOptions wtGLTFTranscodeOptions::ParaseOptions(const std::unordered_map<std::string, std::string>& options)
{
	return Transcoder::ParseOptions<wtGLTFTranscodeOptions>(options);
}


void WT::Transcoder::wtGLTFTranscodeOptions::setFileExt(const std::string& value)
{
	std::unordered_map<std::string, GLTFFileExt> gltfFileExt = {
		{"gltf",GLTFFileExt::GLTF},
		{"glb",GLTFFileExt::GLB},
		{"gltf2",GLTFFileExt::GLTF2},
		{"glb2",GLTFFileExt::GLB2}
	};

	auto it = gltfFileExt.find(value);
	if (it != gltfFileExt.end())
	{
		glTFExt = it->second;
	}
	else {
		throw WT::Ulits::WTInvalidArgException("不支持的格式:" + value);
	}

	foramtID = value;
}

void WT::Transcoder::wtGLTFTranscodeOptions::setNormalInfo(const std::string& value)
{
	std::unordered_map<std::string, NormalInfo> normalInfos = {
		{"origin",NormalInfo::ORIGIN},
		{"generate",NormalInfo::GENERATE},
		{"none",NormalInfo::NONE}
	};

	auto it = normalInfos.find(value);
	if (it != normalInfos.end())
	{
		nornalInfo = it->second;
	}
	else {
		throw WT::Ulits::WTInvalidArgException("不法线设置形式: " + value);
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
	auto it = textureCompressions.find(value);
	if (it != textureCompressions.end())
	{
		textureCompression = it->second;
	}
	else {
		throw WT::Ulits::WTInvalidArgException("不支持的纹理压缩形式: " + value);
	}
}

void WT::Transcoder::wtGLTFTranscodeOptions::setDracoUsing(const std::string& value)
{
	std::unordered_map<std::string, DracoUsing> dracoUsings = {
		{"no",DracoUsing::NO},
		{"yes",DracoUsing::YES}
	};

	auto it = dracoUsings.find(value);
	if (it != dracoUsings.end())
	{
		dracoUsing = it->second;
	}
	else {
		throw WT::Ulits::WTInvalidArgException("不支持draco设置形式:" + value);
	}
}

void WT::Transcoder::wtGLTFTranscodeOptions::setDoubleSide(const std::string& value)
{
	std::unordered_map<std::string, DoubleSide> doubleSides = {
		{"no",DoubleSide::NO},
		{"yes",DoubleSide::YES}
	};

	auto it = doubleSides.find(value);
	if (it != doubleSides.end())
	{
		doubleSide = it->second;
	}
	else {
		throw WT::Ulits::WTInvalidArgException("不支持三角面双面渲染与否设置形式: " + value);
	}
}

void WT::Transcoder::wtGLTFTranscodeOptions::setOutputPath(const std::string& value)
{
	outputPath = value;
}
