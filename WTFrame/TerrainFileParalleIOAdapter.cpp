#include "TerrainFileParalleIOAdapter.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <limits>
#include <memory>
#include <atomic>

// GDAL库
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

// TBB库
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_for_each.h>
#include <oneapi/tbb/task_group.h>
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/concurrent_queue.h>
#include <oneapi/tbb/global_control.h>

#include "DelaunayTriangle.h"
#include "Rectangle.h"

namespace WT {

	// 任务结构体，用于异步处理
	struct TerrainTask {
		IOFileInfo* fileInfo;
		std::string fullPath;
		std::promise<bool> promise;

		TerrainTask(IOFileInfo* info, std::string path)
			: fileInfo(info), fullPath(std::move(path)) {}
	};


	TerrainFileParalleIOAdapter::TerrainFileParalleIOAdapter(const std::string& basePath, bool createDirs /*= true*/, int width /*= 256*/, int height /*= 256*/)
	:mBasePath(basePath), mCreateDirs(createDirs),mWidth(width),mHeight(height)
	{
	}

	bool TerrainFileParalleIOAdapter::initialize() {
		if (mCreateDirs) {
			std::filesystem::create_directories(mBasePath);
		}
		return std::filesystem::exists(mBasePath);
	}



	bool TerrainFileParalleIOAdapter::output(const IOFileInfo* fileInfo)
	{
		const auto fullpath = std::filesystem::path(mBasePath) / fileInfo->filePath;
		auto task = std::make_shared<TerrainTask>(fileInfo,fullpath.string());

		::oneapi::tbb::task_group tg;
		tg.run([this, task]() {
			try
			{
				bool success = processTerrainTask(*task);
				task->promise.set_value(true);
			}
			catch (const std::exception& e)
			{
				std::cerr << "文件输出失败:" << e.what() << std::endl;
				task->promise.set_value(false);
			}
		});

		auto future = task->promise.get_future();
		tg.wait();
		return future.get();
	}

	bool TerrainFileParalleIOAdapter::processTerrainTask(const TerrainTask& task)
	{

	}

	bool TerrainFileParalleIOAdapter::encodeTerrain(IOFileInfo* ioFileInfo)
	{
		//将IOFileInfo转为Terrain的数据结构 注意这里只转换 不输出
		int tileSize = sqrt(ioFileInfo->dataSize);
		TerraMesh terraMesh(tileSize, tileSize, ioFileInfo);
		terraMesh.greedyInsert(1.9);
		std::array<std::vector<glm::dvec3>, 4> boundaryPs = terraMesh.getBoundaryPoints();
		std::vector<int> indices; std::vector<glm::dvec3> pos;
		terraMesh.getMeshData(indices, pos);

		Rectangle* tileRect = static_cast<Rectangle*>(ioFileInfo->userData);
		//根据矩形来重采样平面经纬度位置 然后再转换为地心坐标
		//


		terraMesh.convertToOBJ();
	}

};