#ifndef ASSET3DTOGLTF_H
#define ASSET3DTOGLTF_H
#include "WTFrame/WTDefines.h"
#include <GLTFSDK/BufferBuilder.h>
#include <GLTFSDK/Constants.h>
#include <GLTFSDK/Document.h>
#include <GLTFSDK/Exceptions.h>
#include <GLTFSDK/ExtensionHandlers.h>
#include <GLTFSDK/ResourceWriter.h>

#include <assimp/scene.h>

#include "wtGLTFExportOptions.h"
#include "WTFrame/wtCancelation.h"

namespace WT {
    namespace Transcoder {

		class IGLTFWriter
		{
		public:
			virtual ~IGLTFWriter(){}
			virtual void WriteImage(Microsoft::glTF::Document& document, const std::vector<uint8_t>& data,
				const std::string& id, const std::string& mimeType, const std::string& extension) = 0;
			virtual void Finalize(Microsoft::glTF::Document& document, const Microsoft::glTF::ExtensionSerializer& extensionSerializer) = 0;
			virtual Microsoft::glTF::BufferBuilder& GetBufferBuilder() = 0;
		};

		class Asset3DToGLTF
		{
		public:
			Asset3DToGLTF(const aiScene& scene,const wtGLTFExportOptions& options={},WT::Frame::IWTCancellationTokenPtr cancellationToken=nullptr);
			std::shared_ptr<Microsoft::glTF::Document> Write(IGLTFWriter& writer);

		private:
			void PopulateDocument(IGLTFWriter& bufferBuilder);
			void AddPrimitiveToMesh(Microsoft::glTF::BufferBuilder& bufferBuilder, Microsoft::glTF::Mesh& gltfMesh, const aiMesh& oneAiMesh, std::unordered_map<uint32_t, aiMaterial*>& materials);
			Microsoft::glTF::MeshPrimitive CreateMeshPrimitive(Microsoft::glTF::BufferBuilder& bufferBuilder, const aiMesh& oneAiMesh) const;

			void AddMaterials(IGLTFWriter& writer, const std::unordered_map<uint32_t, aiMaterial*> materials);
			Microsoft::glTF::Material AddMaterials(IGLTFWriter& writer, aiMaterial* oneAiMaterial, const uint8_t scale);

		private:
			const aiScene& mAsset3d;
			const wtGLTFExportOptions mOptions;
			std::shared_ptr<Microsoft::glTF::Document> mGLTFDocument;

			std::unordered_set<std::string> mTextures;
			std::unordered_map<std::string, std::string>mGeneratedTextures;
			std::vector<Microsoft::glTF::Sampler> mSampler;
			WT::Frame::IWTCancellationTokenPtr mCancellationToken; 
		};

    }
}
#endif // ASSET3DTOGLTF_H
