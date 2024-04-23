#ifndef WTGLTFTRANSCODEOPTIONS_H
#define WTGLTFTRANSCODEOPTIONS_H
#include "WTTranscoderDefines.h"

#include <assimp/scene.h>

#include "WTTranscodeOptions.h"

WTNAMESPACESTART
TRANSCODERNAMESPACESTART

enum class GLTFFileExt
{
    GLTF,
    GLB,
    GLTF2,
    GLB2,
};

enum class NormalInfo
{
    ORIGIN,         //ʹ��ԭ���ģ������������û�������
    GENERATE,       //����,�����
    NONE,           //��д��
};

enum class TextureCompression
{
    ORIGIN,
    WEBP,
    PNG,
    JPG,
    BASIS,
};

enum class DracoUsing
{
	YES,
	NO,
};

enum class DoubleSide {
    YES,
    NO,
};

//gltf�����������ð���
//fileExt==>��glb��/gltf
//normalInfo==>��y��/n
//textureCompression==>��ori��/webp/png/jpg/basis
//dracoUsing==>��y��/n
//doubleSide==>y/��n��
//outpath==>""

class WTTRANSCODERAPI wtGLTFTranscodeOptions : public TranscodeOptions
{
public:
    wtGLTFTranscodeOptions();
    static wtGLTFTranscodeOptions ParaseOptions(const std::unordered_map<std::string, std::string>& options);

    void setFileExt(const std::string& value);
    void setNormalInfo(const std::string& value);
	void setTextureCompressiong(const std::string& value);
	void setDracoUsing(const std::string& value);
	void setDoubleSide(const std::string& value);
    void setOutputPath(const std::string& value);

    GLTFFileExt glTFExt = GLTFFileExt::GLB2;
    NormalInfo nornalInfo = NormalInfo::ORIGIN;
    TextureCompression textureCompression = TextureCompression::ORIGIN;
    DracoUsing dracoUsing = DracoUsing::YES;
    DoubleSide doubleSide = DoubleSide::NO;


    std::string outputPath="";
    std::string foramtID = "glb2";
};

TRANSCODERNAMESPACEEND
WTNAMESPACEEND
#endif // WTGLTFTRANSCODEOPTIONS_H
