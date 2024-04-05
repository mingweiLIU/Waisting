#include "WTAsset3DToGLTF.h"

#include <string>
#include <queue>
#include <algorithm>
#include <sstream>

#include "wtExtensionsInWT.h"


USINGTRANSCODERNAMESPACE

std::string GetGenerator() {
	std::stringstream ss;
	ss << GLTFVENDER << " ";
	return ss.str();
}

Asset3DToGLTF::Asset3DToGLTF(const aiScene& scene, const wtGLTFExportOptions& options/*={}*/, WT::Frame::IWTCancellationTokenPtr cancellationToken/*=nullptr*/)
	:mAsset3d(scene)
	,mOptions(options)
	,mCancellationToken(cancellationToken)
{
	if (nullptr==mCancellationToken)
	{
		mCancellationToken = WT::Frame::MakeNullCancellationToken();
	}
}

std::shared_ptr<Microsoft::glTF::Document> WT::Transcoder::Asset3DToGLTF::Write(IGLTFWriter& writer)
{
	Microsoft::glTF::Asset asset;
	asset.version = Microsoft::glTF::GLTF_VERSION_2_0;
	asset.generator = GetGenerator();

	mGLTFDocument = std::make_shared<Microsoft::glTF::Document>(std::move(asset));
	PopulateDocument(writer);

	auto extensionSerializer = GetMSFTKHRExtensionSerializer();
	writer.Finalize(*mGLTFDocument, extensionSerializer);
	return mGLTFDocument;
}

Microsoft::glTF::MeshPrimitive WT::Transcoder::Asset3DToGLTF::CreateMeshPrimitive(Microsoft::glTF::BufferBuilder& bufferBuilder, const aiMesh& oneAiMesh) const
{

}
