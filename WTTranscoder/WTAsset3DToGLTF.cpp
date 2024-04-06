#include "WTAsset3DToGLTF.h"

#include <string>
#include <queue>
#include <algorithm>
#include <sstream>

#include <assimp/GltfMaterial.h>

#include "wtExtensionsInWT.h"
#include "WTUlits/wtSmartThrow.h"


USINGTRANSCODERNAMESPACE

std::string GetGenerator() {
	std::stringstream ss;
	ss << GLTFVENDER << " ";
	return ss.str();
}

//三角网形式转换 TODO 这里没有考虑triangle_fans/triagnle_strip等 这个在管网建模和城市白模建模上却是必须的 到时候看看如何处理
Microsoft::glTF::MeshMode GetGltfMeshPrimitiveMode(const aiMesh& oneAiMesh) {
	switch (oneAiMesh.mPrimitiveTypes)
	{
	case aiPrimitiveType_POINT:
		return Microsoft::glTF::MeshMode::MESH_POINTS;
	case aiPrimitiveType_LINE:
		return Microsoft::glTF::MeshMode::MESH_LINES;
	case aiPrimitiveType_TRIANGLE:
		return Microsoft::glTF::MeshMode::MESH_TRIANGLES;
	}
	throw WT::Ulits::WTException("不支持的几何类型");
}


std::vector<uint32_t> GetIndexOfAiMesh(const aiMesh& oneMesh) {
	std::vector<uint32_t> indices;
	indices.reserve(oneMesh.mNumFaces * 3);//预先按照三角形来申请
	for (size_t i = 0; i < oneMesh.mNumFaces; i++)
	{
		for (size_t j = 0,j_up= oneMesh.mFaces[i].mNumIndices; j < j_up; j++)
		{
			indices.emplace_back(oneMesh.mFaces[i].mIndices[j]);
		}
	}
	return std::move(indices);
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

void WT::Transcoder::Asset3DToGLTF::AddPrimitiveToMesh(Microsoft::glTF::BufferBuilder& bufferBuilder, Microsoft::glTF::Mesh& gltfMesh, const aiMesh& oneAiMesh, std::unordered_map<uint32_t, aiMaterial*>& materials)
{
	Microsoft::glTF::MeshPrimitive primitive = CreateMeshPrimitive(bufferBuilder, oneAiMesh);

}

Microsoft::glTF::MeshPrimitive WT::Transcoder::Asset3DToGLTF::CreateMeshPrimitive(Microsoft::glTF::BufferBuilder& bufferBuilder, const aiMesh& oneAiMesh) const
{
	mCancellationToken->CheckCancelledAndThrow();

	//处理三角网类型
	Microsoft::glTF::MeshPrimitive meshPrimitive;
	meshPrimitive.mode = GetGltfMeshPrimitiveMode(oneAiMesh);

	//处理索引
	if (oneAiMesh.HasFaces())
	{
		bufferBuilder.AddBufferView(Microsoft::glTF::ELEMENT_ARRAY_BUFFER);
		meshPrimitive.indicesAccessorId = bufferBuilder.AddAccessor(GetIndexOfAiMesh(oneAiMesh), { Microsoft::glTF::TYPE_SCALAR,Microsoft::glTF::COMPONENT_UNSIGNED_INT }).id;
	}

	//处理顶点
	{
		aiAABB oneExtent = oneAiMesh.mAABB;
		std::vector<float> minExt{oneExtent.mMin.x, oneExtent.mMin.y, oneExtent.mMin.z};
		std::vector<float> maxExt{oneExtent.mMax.x, oneExtent.mMax.y, oneExtent.mMax.z};
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_POSITION] = bufferBuilder.AddAccessor(oneAiMesh.mVertices, oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC3,Microsoft::glTF::COMPONENT_FLOAT,false,std::move(minExt),std::move(maxExt) }).id;
	}

	//处理法线
	if (oneAiMesh.HasNormals())
	{
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_NORMAL] = bufferBuilder.AddAccessor(oneAiMesh.mNormals, oneAiMesh.mNumVertices, 
			{ Microsoft::glTF::TYPE_VEC3,Microsoft::glTF::COMPONENT_FLOAT }).id;
	}

	//处理切线
	if (oneAiMesh.HasTangentsAndBitangents())
	{
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_TANGENT]=bufferBuilder.AddAccessor(oneAiMesh.mTangents,oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC4, Microsoft::glTF::COMPONENT_FLOAT }).id;
	}

	//处理纹理通道0
	if (oneAiMesh.HasTextureCoords(0))
	{
		aiVector3D* texCoord0= oneAiMesh.mTextureCoords[0];
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_TEXCOORD_0] = bufferBuilder.AddAccessor(texCoord0, oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC2,Microsoft::glTF::COMPONENT_FLOAT }).id;
	}

	//处理纹理通道1
	if (oneAiMesh.HasTextureCoords(1))
	{
		aiVector3D* texCoord1 = oneAiMesh.mTextureCoords[1];
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_TEXCOORD_1] = bufferBuilder.AddAccessor(texCoord1, oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC2,Microsoft::glTF::COMPONENT_FLOAT }).id;
	}

	//处理原本的颜色
	if (oneAiMesh.HasVertexColors(0))
	{
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_COLOR_0] = bufferBuilder.AddAccessor(oneAiMesh.mColors[0],oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC4,Microsoft::glTF::COMPONENT_UNSIGNED_BYTE,true }).id;
	}

	//TODO 处理蒙皮动画
	//if (oneAiMesh.HasBones())
	//{
	//}

	return meshPrimitive;
}

Microsoft::glTF::Material WT::Transcoder::Asset3DToGLTF::AddMaterials(IGLTFWriter& writer, aiMaterial* oneAiMaterial, const uint8_t scale)
{
	
}
