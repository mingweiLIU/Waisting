#include "wtAssectConvertor.h"

#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
//#include <taskflow.hpp>

#include "WTUlits/wtFileIO.h"
#include "WTUlits/wtStreamUtils.h"

USINGTRANSCODERNAMESPACE

WT::Transcoder::wtAssectConvertor::wtAssectConvertor(aiScene* assets,const wtGLTFTranscodeOptions& options/*={}*/, WT::Frame::IWTCancellationTokenPtr cancellationToken/*=nullptr*/)
	: mAssets(assets)
	, mOptions(options)
	, mCancellationToken(cancellationToken)
{
}

bool WT::Transcoder::wtAssectConvertor::ConvertFilesInFolder(std::string folderPath, std::string formatID, std::string outputFolder)
{
	std::vector<std::string> objsInFolder= WT::Ulits::wtFileIO::GetFilesInDirectory(folderPath, "obj");
	WT::Ulits::wtFileIO::MakeDirectory(outputFolder);
	for (auto& oneFile : objsInFolder)
	{
		std::string filePath = folderPath + "//" + oneFile;
		Assimp::Importer importer;
		const aiScene* assetScene=importer.ReadFile(filePath, aiProcess_ValidateDataStructure);
		wtGLTFTranscodeOptions exportOption;
		exportOption.ParaseOptions(std::unordered_map<std::string, std::string>{
			{"outpath",outputFolder+"//"+ WT::Ulits::wtStringUtils::GetPathFileName(oneFile)+".glb"}
		});
		wtAssectConvertor assetConvertor(const_cast<aiScene*>(assetScene),exportOption);
		bool oneResult=assetConvertor.Write();
	}
	return true;
}

bool WT::Transcoder::wtAssectConvertor::Write()
{
	mCancellationToken->CheckCancelledAndThrow();
	//现在来处理各个参数信息
	uint32_t exportPostProcess = aiProcess_OptimizeGraph | aiPostProcessSteps::aiProcess_OptimizeMeshes;
	Assimp::ExportProperties exportProperty;

	switch (mOptions.nornalInfo)
	{
		case NormalInfo::GENERATE: {
			exportPostProcess |= aiProcess_GenNormals;
			exportPostProcess |= aiProcess_FixInfacingNormals;
			exportProperty.SetPropertyBool("GLTF2_TARGET_NORMAL_EXP", true);
			break;
		}
		case NormalInfo::NONE:
		{
			//删除
			exportProperty.SetPropertyBool("GLTF2_TARGET_NORMAL_EXP", false);
			break;
		}
		default: {
			exportProperty.SetPropertyBool("GLTF2_TARGET_NORMAL_EXP", true);
			break;
		}
	}

	if (DoubleSide::YES==mOptions.doubleSide)
	{
		//遍历所有的材质 设置为双面
		aiMaterial** mats=mAssets->mMaterials;
		for (int i=0,i_up=mAssets->mNumMaterials;i<i_up;++i)
		{
			int temp = 1;
			mAssets->mMaterials[i]->AddProperty<int>(&temp, 1, AI_MATKEY_TWOSIDED);
		}
	}

	Assimp::Exporter exporter;
	return exporter.Export(mAssets, mOptions.foramtID, mOptions.outputPath, exportPostProcess, &exportProperty);
}
