#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <future>
#include <memory>
#include "IOAdapter.h"
#include <filesystem>
namespace WT {

	//专门定义为Quanified Mesh地形瓦片输出器
	// TBB前向声明
	namespace tbb {
		class global_control;
	}
	
	class QuantifiedMeshData {
	public:
		struct QuantizedMeshHeader
		{
			// The center of the tile in Earth-centered Fixed coordinates.
			double CenterX;
			double CenterY;
			double CenterZ;

			// The minimum and maximum heights in the area covered by this tile.
			// The minimum may be lower and the maximum may be higher than
			// the height of any vertex in this tile in the case that the min/max vertex
			// was removed during mesh simplification, but these are the appropriate
			// values to use for analysis or visualization.
			float MinimumHeight;
			float MaximumHeight;

			// The tile’s bounding sphere.  The X,Y,Z coordinates are again expressed
			// in Earth-centered Fixed coordinates, and the radius is in meters.
			double BoundingSphereCenterX;
			double BoundingSphereCenterY;
			double BoundingSphereCenterZ;
			double BoundingSphereRadius;

			// The horizon occlusion point, expressed in the ellipsoid-scaled Earth-centered Fixed frame.
			// If this point is below the horizon, the entire tile is below the horizon.
			// See http://cesiumjs.org/2013/04/25/Horizon-culling/ for more information.
			double HorizonOcclusionPointX;
			double HorizonOcclusionPointY;
			double HorizonOcclusionPointZ;
		};

		struct VertexData
		{
			unsigned int vertexCount;
			unsigned short u[vertexCount];
			unsigned short v[vertexCount];
			unsigned short height[vertexCount];
		};
		struct IndexData16
		{
			unsigned int triangleCount;
			unsigned short indices[triangleCount * 3];
		}

		struct IndexData32
		{
			unsigned int triangleCount;
			unsigned int indices[triangleCount * 3];
		}

		struct EdgeIndices16
		{
			unsigned int westVertexCount;
			unsigned short westIndices[westVertexCount];

			unsigned int southVertexCount;
			unsigned short southIndices[southVertexCount];

			unsigned int eastVertexCount;
			unsigned short eastIndices[eastVertexCount];

			unsigned int northVertexCount;
			unsigned short northIndices[northVertexCount];
		}

		struct EdgeIndices32
		{
			unsigned int westVertexCount;
			unsigned int westIndices[westVertexCount];

			unsigned int southVertexCount;
			unsigned int southIndices[southVertexCount];

			unsigned int eastVertexCount;
			unsigned int eastIndices[eastVertexCount];

			unsigned int northVertexCount;
			unsigned int northIndices[northVertexCount];
		}
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