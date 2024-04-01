#include "WTExporterGLTF.h"

#include <GLTFSDK/ExtensionsKHR.h>
#include <GLTFSDK/GLTFResourceWriter.h>
#include <GLTFSDK/Serialize.h>
#include "wtGLTFExportOptions.h"
#include "WTFrame/wtLog.h"
#include "Asset3DToGLTF.h"
#include "IWTOutputStreamFactory.h"
USINGTRANSCODERNAMESPACE

Microsoft::glTF::BufferBuilder IntializeBufferBuilder(const std::string& assetName, std::unique_ptr<const Microsoft::glTF::IStreamWriter>&& streatWriter) {
	auto resourceWriter = std::make_unique<Microsoft::glTF::GLTFResourceWriter>(std::move(streatWriter));
	resourceWriter->SetUriPrefix(assetName + "_");
	return Microsoft::glTF::BufferBuilder(std::move(resourceWriter));
}

wtGLTFWriter::wtGLTFWriter(const std::string& assetName, std::unique_ptr<const Microsoft::glTF::IStreamWriter>&& streamWriter)
	:mAssetName(assetName),mBufferBuilder(IntializeBufferBuilder(assetName,std::move(streamWriter)))
{
	mBufferBuilder.AddBuffer();
}

void WT::Transcoder::wtGLTFWriter::WriteImage(Microsoft::glTF::Document& document, const std::vector<uint8_t>& data, const std::string& id, const std::string& mimeType, const std::string& extension)
{
	Microsoft::glTF::Image image;
	image.id = id;
	image.uri = id + "." + extension;
	image.mimeType = mimeType;

	mBufferBuilder.GetResourceWriter().WriteExternal(image.uri, data);
	document.images.Append(std::move(image));
}

void WT::Transcoder::wtGLTFWriter::Finalize(Microsoft::glTF::Document& document, const Microsoft::glTF::ExtensionSerializer& extensionSerializer)
{
	mBufferBuilder.Output(document);
	if (document.meshes.Size() == 0)
	{
		wtLOG_WARN("Warning: No meshes found.  This file will only be an empty scene.");
		document.buffers.Clear();
	}

	const auto gltfManifest = Microsoft::glTF::Serialize(document, extensionSerializer, Microsoft::glTF::SerializeFlags::Pretty);
	mBufferBuilder.GetResourceWriter().WriteExternal(mAssetName + "." + Microsoft::glTF::GLTF_EXTENSION, gltfManifest.c_str(), gltfManifest.length());
}

void WT::Transcoder::ExporterGLTF::ExportStatic(std::shared_ptr<aiScene>& scene, std::string const& assetName, IOutputStreamFactory* steamFactory, FractionalProgressCallback progress /*= nullptr*/, WT::Frame::IWTCancellationTokenPtr cancellationToken /*= nullptr*/, const std::unordered_map<std::string, std::string>& options /*= {}*/)
{
	auto writer = std::make_unique<IStreamWriterOSFAdapter>(*steamFactory);
	Export(*scene.get(), std::move(writer), assetName, wtGLTFExportOptions::ParseOptions(options), cancellationToken);
}

void WT::Transcoder::ExporterGLTF::Export(const aiScene& scene, std::unique_ptr<const Microsoft::glTF::IStreamWriter>&& streamWriter, const std::string& assetName, const wtGLTFExportOptions& options, WT::Frame::IWTCancellationTokenPtr cancellationToken /*= nullptr*/)
{
	Asset3DToGLTF converter(scene, options, cancellationToken);
	wtGLTFWriter gltfWriter(assetName, std::move(streamWriter));
	converter.Write(gltfWriter);
}
