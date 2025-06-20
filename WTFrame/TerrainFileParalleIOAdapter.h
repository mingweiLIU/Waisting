#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <future>
#include <memory>
#include "IOAdapter.h"
#include <filesystem>
#include <glm/glm.hpp>
namespace WT {

	//专门定义为Quanified Mesh地形瓦片输出器
	// TBB前向声明
	namespace tbb {
		class global_control;
	}
	constexpr int QUANTIZED_COORDINATE_SIZE = 32767;

	class QuantifiedMeshData {
	public:
		struct QuantizedMeshHeader
		{
		public:
			// The center of the tile in Earth-centered Fixed coordinates.
			//double CenterX;
			//double CenterY;
			//double CenterZ;
			glm::dvec3 center;

			// The minimum and maximum heights in the area covered by this tile.
			// The minimum may be lower and the maximum may be higher than
			// the height of any vertex in this tile in the case that the min/max vertex
			// was removed during mesh simplification, but these are the appropriate
			// values to use for analysis or visualization.
			//float MinimumHeight;
			//float MaximumHeight;
			glm::vec2 minMaxHeight;

			// The tile’s bounding sphere.  The X,Y,Z coordinates are again expressed
			// in Earth-centered Fixed coordinates, and the radius is in meters.
			//double BoundingSphereCenterX;
			//double BoundingSphereCenterY;
			//double BoundingSphereCenterZ;
			//double BoundingSphereRadius;
			glm::dvec4 boundingSphere;

			// The horizon occlusion point, expressed in the ellipsoid-scaled Earth-centered Fixed frame.
			// If this point is below the horizon, the entire tile is below the horizon.
			// See http://cesiumjs.org/2013/04/25/Horizon-culling/ for more information.
			//double HorizonOcclusionPointX;
			//double HorizonOcclusionPointY;
			//double HorizonOcclusionPointZ;
			glm::dvec3 horizonOcclusionPoint;

			bool writeToFile(FILE** fp) {
				size_t doubleSize = sizeof(double);
				size_t floatSize = sizeof(float);

				//写入中心点
				fwrite(&center, sizeof(double), 3, *fp);
				//写入minmax
				fwrite(&minMaxHeight, sizeof(float), 2, *fp);
				//写入boundingbox
				fwrite(&boundingSphere, sizeof(double), 4, *fp);
				//写入水平线裁切点
				fwrite(&horizonOcclusionPoint, sizeof(double), 3, *fp);				
			}
		};
		struct VertexData
		{
		public:
			VertexData(std::vector<glm::dvec3> positions, glm::dvec3 minPos, glm::dvec3 maxPos) {
				double ratioX = QUANTIZED_COORDINATE_SIZE / (maxPos.x - minPos.x);
				double ratioY = QUANTIZED_COORDINATE_SIZE / (maxPos.y - minPos.y);
				double ratioZ = QUANTIZED_COORDINATE_SIZE / (maxPos.z - minPos.z);

				int preU = 0, preV = 0, preH = 0;
				for (glm::dvec3& onePos:positions)
				{
					int scaledX = static_cast<int> ((onePos.x - minPos.x) * ratioX);
					int scaledY = static_cast<int> ((onePos.y - minPos.y) * ratioY);
					int scaledZ = static_cast<int> ((onePos.z - minPos.z) * ratioZ);
					
					u.push_back(zigZagEncode(scaledX - preU)); 
					v.push_back(zigZagEncode(scaledY - preV)); 
					height.push_back(zigZagEncode(scaledZ - preH));

					preU = scaledX; preV = scaledY; preH = scaledZ;
				}
			}
			bool writeToFile(FILE** fp) {
				size_t doubleSize = sizeof(double);
				size_t floatSize = sizeof(float);

				//写入顶点数
				fwrite(&vertexCount, sizeof(unsigned int), 1, *fp);
				//写入u
				fwrite(u.data(), sizeof(unsigned short), vertexCount, *fp);
				//写入v
				fwrite(v.data(), sizeof(unsigned short), vertexCount, *fp);
				//写入height
				fwrite(height.data(), sizeof(unsigned short), vertexCount, *fp);
			}
		private:
			unsigned int vertexCount;
			std::vector<unsigned short> u;
			std::vector<unsigned short>v;// v[vertexCount];
			std::vector<unsigned short> height;// [vertexCount] ;
			//这里的value为代编码的值 minValue为最低值 delta为max-min值
			inline uint16_t zigZagEncode(int16_t i)
			{
				return ((i >> 15) ^ (i << 1));
			}
		};
		struct IndexData
		{
		public:
			IndexData(std::vector<unsigned int> indices, FILE** fp) {
				unsigned int triangleCount = indices.size()/3;
				fwrite(&triangleCount, sizeof(unsigned int), 1, *fp);
				if (indices.size() < 65536)
				{
					std::vector<unsigned short> temp(indices.begin(), indices.end());

					unsigned short waterMark = 0;
					for (unsigned short i=0,iUP= indices.size();i<iUP;++i)
					{
						unsigned short index = indices[i];
						unsigned short delta = waterMark - index;
						temp[i] = delta;
						if (index==waterMark)
						{
							waterMark++;
						}
					}

					fwrite(temp.data(), sizeof(unsigned short), temp.size(), *fp);
				}
				else
				{
					unsigned int waterMark = 0;
					for (unsigned int i = 0, iUP = indices.size(); i < iUP; ++i)
					{
						unsigned int index = indices[i];
						unsigned int delta = waterMark - index;
						indices[i] = delta;
						if (index == waterMark)
						{
							waterMark++;
						}
					}
					fwrite(indices.data(), sizeof(unsigned short), indices.size(), *fp);
				}
			}
		};
		struct EdgeIndices
		{
		public:
			EdgeIndices(std::vector<unsigned int> westIndices
				, std::vector<unsigned int> southIndices
				, std::vector<unsigned int> eastIndices
				, std::vector<unsigned int> northIndices
				, FILE** fp) {
				writeEdgeIndex(westIndices,fp);
				writeEdgeIndex(southIndices,fp);
				writeEdgeIndex(eastIndices,fp);
				writeEdgeIndex(northIndices,fp);
			}
		private:
			void writeEdgeIndex(std::vector<unsigned int> indices, FILE** fp) {
				unsigned int triangleCount = indices.size();
				fwrite(&triangleCount, sizeof(unsigned int), 1, *fp);
				if (triangleCount < 65536)
				{
					std::vector<unsigned short> temp(indices.begin(), indices.end());
					fwrite(temp.data(), sizeof(unsigned short), temp.size(), *fp);
				}
				else
				{
					fwrite(indices.data(), sizeof(unsigned short), indices.size(), *fp);
				}
			}
		};
		
		QuantizedMeshHeader header;
		VertexData vertexData;
		IndexData indexData;
		EdgeIndices edgeIndices;
	};
	// 前向声明
	struct TerrainTask;

	class TerrainFileParalleIOAdapter :public IOAdapter {
	public:
		// 构造函数
		TerrainFileParalleIOAdapter(const std::string& basePath, bool createDirs = true,
			int width = 256, int height = 256);

		// 析构函数
		~TerrainFileParalleIOAdapter() = default;

		// 初始化
		bool initialize() override;

		// 输出单个文件（异步）
		bool output(const IOFileInfo* fileInfo) override;

		// 批量输出（并行同步）
		bool outputBatch(const std::vector<IOFileInfo*> files) override;

		// 批量输出（完全异步）
		bool outputBatchAsync(const std::vector<IOFileInfo*> files) override;


		std::string type() const override { return "TerrainFileParalleIOAdapter"; }

		// 完成输出
		bool finalize();

	private:
		// 成员变量
		std::string mBasePath;
		bool mCreateDirs;
		int mWidth;
		int mHeight;
		int ticProgressNum = 0;//每处理多少个文件就执行一次progressCallback

		// 私有方法

		// 处理单个地形任务
		bool processTerrainTask(const TerrainTask& task);

		// 将数据转换为地形
		bool encodeTerrain(IOFileInfo* ioFileInfo);
	};
};