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
			void AddPrimitiveToMesh(Microsoft::glTF::BufferBuilder& bufferBuilder, Microsoft::glTF::Mesh& gltfMesh, const Geometry& geometry, std::unordered_map<uint32_t, MaterialDescriptor*>& materials);
		};

    }
}
#endif // ASSET3DTOGLTF_H
