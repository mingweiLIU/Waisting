#include "wtGLTFTranscodeOptions.h"
#include "WTUlits/wtSmartThrow.h"


WT::Transcoder::wtGLTFTranscodeOptions::wtGLTFTranscodeOptions()
{
	AddOption({ "fileExt","���������ʽ",[this](const std::string& value) {setFileExt(value); } });
	AddOption({ "normalInfo","�����Ƿ��������",[this](const std::string& value) {setNormalInfo(value); } });
	AddOption({ "textureCompression","��������ѹ����ʽ",[this](const std::string& value) {setTextureCompressiong(value); } });
	AddOption({ "dracoUsing","�����Ƿ�ʹ�÷���",[this](const std::string& value) {setNormalInfo(value); } });
	AddOption({ "doubleSide","�����Ƿ�˫����Ⱦ",[this](const std::string& value) {setDoubleSide(value); } });
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
		throw WTInvalidArgException("��֧�ֵĸ�ʽ:" + value);
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
		throw WTInvalidArgException("������������ʽ: " + value);
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
		throw WTInvalidArgException("��֧�ֵ�����ѹ����ʽ: " + value);
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
		throw WTInvalidArgException("��֧��draco������ʽ: " + value);
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
		throw WTInvalidArgException("��֧��������˫����Ⱦ���������ʽ: " + value);
	}
}
