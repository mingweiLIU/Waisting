#ifndef EXPORTERGLTF_H
#define EXPORTERGLTF_H
#include "WTTranscoderDefines.h"
#include "IWTOutputStreamFactory.h"
#include "WTFrame/wtCancelation.h"
#include "WTAsset3DToGLTF.h"
#include "GLTFSDK/IStreamWriter.h"
#include<assimp/scene.h>
#include <unordered_map>
#include <string>

WTNAMESPACESTART
TRANSCODERNAMESPACESTART

class GLTFExportOptions;

class WTAPI ExporterGLTF
{
public:
	static void ExportStatic(std::shared_ptr<aiScene>& scene, std::string const& assetName, IOutputStreamFactory* steamFactory, 
		FractionalProgressCallback progress = nullptr, WT::Frame::IWTCancellationTokenPtr cancellationToken = nullptr,
		const std::unordered_map<std::string, std::string>& options = {});

	static void Export(const aiScene& scene, std::unique_ptr<const Microsoft::glTF::IStreamWriter>&& streamWriter, 
		const std::string& assetName, const wtGLTFExportOptions& options, WT::Frame::IWTCancellationTokenPtr cancellationToken = nullptr);
};


class wtGLTFWriter:public IGLTFWriter
{
public:
	wtGLTFWriter(const std::string& assetName, std::unique_ptr<const Microsoft::glTF::IStreamWriter>&& streamWriter);
	void WriteImage(Microsoft::glTF::Document& document, const std::vector<uint8_t>& data, const std::string& id, const std::string& mimeType, const std::string& extension) override;
	void Finalize(Microsoft::glTF::Document& document, const Microsoft::glTF::ExtensionSerializer& extensionSerializer) override;
	Microsoft::glTF::BufferBuilder& GetBufferBuilder()override { return mBufferBuilder; }

private:
	const std::string mAssetName;
	Microsoft::glTF::BufferBuilder mBufferBuilder;
};

TRANSCODERNAMESPACEEND
WTNAMESPACEEND
#endif // EXPORTERGLTF_H
