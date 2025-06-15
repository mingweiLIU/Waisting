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
	{}

	bool TerrainFileParalleIOAdapter::initialize() {
		if (mCreateDirs) {
			std::filesystem::create_directories(mBasePath);
		}
		return std::filesystem::exists(mBasePath);
	}



};