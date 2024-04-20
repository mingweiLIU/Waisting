#include "wtGLTFTranscodeOptions.h"
#include "WTUlits/wtSmartThrow.h"

USINGTRANSCODERNAMESPACE

WT::Transcoder::wtGLTFTranscodeOptions::wtGLTFTranscodeOptions()
{
	AddOption({ "fileExt","���������ʽ",[this](const std::string& value) {setFileExt(value); } });
	AddOption({ "normalInfo","�����Ƿ��������",[this](const std::string& value) {setNormalInfo(value); } });
	AddOption({ "textureCompression","��������ѹ����ʽ",[this](const std::string& value) {setTextureCompressiong(value); } });
	AddOption({ "dracoUsing","�����Ƿ�ʹ�÷���",[this](const std::string& value) {setNormalInfo(value); } });
	AddOption({ "doubleSide","�����Ƿ�˫����Ⱦ",[this](const std::string& value) {setDoubleSide(value); } });
	AddOption({ "outpath","���·��",[this](const std::string& value) {setOutputPath(value); } });
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
		throw WT::Ulits::WTInvalidArgException("��֧�ֵĸ�ʽ:" + value);
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
		throw WT::Ulits::WTInvalidArgException("������������ʽ: " + value);
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
		throw WT::Ulits::WTInvalidArgException("��֧�ֵ�����ѹ����ʽ: " + value);
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
		throw WT::Ulits::WTInvalidArgException("��֧��draco������ʽ:" + value);
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
		throw WT::Ulits::WTInvalidArgException("��֧��������˫����Ⱦ���������ʽ: " + value);
	}
}

void WT::Transcoder::wtGLTFTranscodeOptions::setOutputPath(const std::string& value)
{
	outputPath = value;
}
