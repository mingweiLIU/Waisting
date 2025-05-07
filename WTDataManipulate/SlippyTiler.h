#pragma once
// 切片生成类
#include <filesystem>

// GDAL库
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <cpl_vsi.h>

// TBB库
#include <tbb/task_group.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/global_control.h>
#include <tbb/concurrent_queue.h>

#include "MemoryPool.h"
#include "FileBufferManager.h"
#include "IDataM.h"

namespace fs = std::filesystem;

namespace WT{
class SlippyMapTilerOptions :public IDataOptions {
public:
	std::string inputFile="";//输入的文件
	std::string outputDir="";//输出路径
	int minLevel = 0;//最小切片级数
	int maxLevel = 5;//最大切片级数
	int tileSize = 255;//瓦片大小
	bool useVRT = true;//是否使用VRT
	bool useMemoryMapping = true;//是否启用文件映射
	std::string outputFormat = "png";//输出瓦片后缀
	int numThreads = 5;//使用的线程数
};


class SlippyMapTiler :public IDataProcessor {
public:
	SlippyMapTiler(std::shared_ptr<SlippyMapTilerOptions> options);
	~SlippyMapTiler();
	bool process(std::shared_ptr<IProgressInfo> progressInfo) override;
	//获取处理器名称
	virtual std::string getName()const { return "影像切片器"; };
	bool initialize();
	void setOptions(std::shared_ptr<IDataOptions> options);

private:
	//处理所使用的Options
	std::shared_ptr<SlippyMapTilerOptions> options;
	GDALDatasetH dataset;
	double geo_transform[6];
	int img_width, img_height;
	double min_x, min_y, max_x, max_y;

	std::shared_ptr<MemoryPool> memory_pool;
	std::shared_ptr<FileBufferManager> file_buffer;

	// 使用非锁定的缓存来存储常用的变量
	struct {
		int tile_size = 256;
	} config;

	// 创建输出目录
	void create_directories(int zoom);

	// 获取某一级别的瓦片范围
	void get_tile_range(int zoom, int& min_tile_x, int& min_tile_y, int& max_tile_x, int& max_tile_y);

	// 计算指定瓦片在原始影像中的范围
	void get_tile_geo_bounds(int zoom, int tile_x, int tile_y, double& tile_min_x, double& tile_min_y, double& tile_max_x, double& tile_max_y);

	// 将地理坐标转换为像素坐标
	void geo_to_pixel(const double& geo_x, const double& geo_y, int& pixel_x, int& pixel_y);

	// 生成单个瓦片
	bool generate_tile(int zoom, int tile_x, int tile_y);
};
}