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

//��������ʽת�� TODO ����û�п���triangle_fans/triagnle_strip�� ����ڹ�����ģ�ͳ��а�ģ��ģ��ȴ�Ǳ���� ��ʱ�򿴿���δ���
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
	throw WT::Ulits::WTException("��֧�ֵļ�������");
}


std::vector<uint32_t> GetIndexOfAiMesh(const aiMesh& oneMesh) {
	std::vector<uint32_t> indices;
	indices.reserve(oneMesh.mNumFaces * 3);//Ԥ�Ȱ���������������
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

	//��������������
	Microsoft::glTF::MeshPrimitive meshPrimitive;
	meshPrimitive.mode = GetGltfMeshPrimitiveMode(oneAiMesh);

	//��������
	if (oneAiMesh.HasFaces())
	{
		bufferBuilder.AddBufferView(Microsoft::glTF::ELEMENT_ARRAY_BUFFER);
		meshPrimitive.indicesAccessorId = bufferBuilder.AddAccessor(GetIndexOfAiMesh(oneAiMesh), { Microsoft::glTF::TYPE_SCALAR,Microsoft::glTF::COMPONENT_UNSIGNED_INT }).id;
	}

	//������
	{
		aiAABB oneExtent = oneAiMesh.mAABB;
		std::vector<float> minExt{oneExtent.mMin.x, oneExtent.mMin.y, oneExtent.mMin.z};
		std::vector<float> maxExt{oneExtent.mMax.x, oneExtent.mMax.y, oneExtent.mMax.z};
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_POSITION] = bufferBuilder.AddAccessor(oneAiMesh.mVertices, oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC3,Microsoft::glTF::COMPONENT_FLOAT,false,std::move(minExt),std::move(maxExt) }).id;
	}

	//������
	if (oneAiMesh.HasNormals())
	{
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_NORMAL] = bufferBuilder.AddAccessor(oneAiMesh.mNormals, oneAiMesh.mNumVertices, 
			{ Microsoft::glTF::TYPE_VEC3,Microsoft::glTF::COMPONENT_FLOAT }).id;
	}

	//��������
	if (oneAiMesh.HasTangentsAndBitangents())
	{
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_TANGENT]=bufferBuilder.AddAccessor(oneAiMesh.mTangents,oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC4, Microsoft::glTF::COMPONENT_FLOAT }).id;
	}

	//��������ͨ��0
	if (oneAiMesh.HasTextureCoords(0))
	{
		aiVector3D* texCoord0= oneAiMesh.mTextureCoords[0];
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_TEXCOORD_0] = bufferBuilder.AddAccessor(texCoord0, oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC2,Microsoft::glTF::COMPONENT_FLOAT }).id;
	}

	//��������ͨ��1
	if (oneAiMesh.HasTextureCoords(1))
	{
		aiVector3D* texCoord1 = oneAiMesh.mTextureCoords[1];
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_TEXCOORD_1] = bufferBuilder.AddAccessor(texCoord1, oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC2,Microsoft::glTF::COMPONENT_FLOAT }).id;
	}

	//����ԭ������ɫ
	if (oneAiMesh.HasVertexColors(0))
	{
		bufferBuilder.AddBufferView(Microsoft::glTF::ARRAY_BUFFER);
		meshPrimitive.attributes[Microsoft::glTF::ACCESSOR_COLOR_0] = bufferBuilder.AddAccessor(oneAiMesh.mColors[0],oneAiMesh.mNumVertices,
			{ Microsoft::glTF::TYPE_VEC4,Microsoft::glTF::COMPONENT_UNSIGNED_BYTE,true }).id;
	}

	//TODO ������Ƥ����
	//if (oneAiMesh.HasBones())
	//{
	//}

	return meshPrimitive;
}

Microsoft::glTF::Material WT::Transcoder::Asset3DToGLTF::AddMaterials(IGLTFWriter& writer, aiMaterial* oneAiMaterial, const uint8_t scale)
{
	
}
