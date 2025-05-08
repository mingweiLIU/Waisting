#pragma once
// 切片生成类
#include <filesystem>
// GDAL库
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

// TBB库
#include "tbb/task_group.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"
#include "tbb/global_control.h"
#include "tbb/concurrent_queue.h"

#include "MemoryPool.h"
#include "FileBufferManager.h"
#include "IDataM.h"
#include "CoordinateSystemManager.h"

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
	std::string prjFilePath = "";//外部prj文件路径 可以为空
	std::string wktString = "";//外部wkt字符串 可以为空
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
	// 常量定义
	int DEFAULT_TILE_SIZE = 256;
	int DEFAULT_MAX_QUEUE_SIZE = 5000;
	int DEFAULT_JPEG_QUALITY = 90;
	double MAX_LATITUDE = 85.0511;
	double MIN_LATITUDE = -85.0511;
	double MAX_LONGITUDE = 180.0;
	double MIN_LONGITUDE = -180.0;
private:
	//处理所使用的Options
	std::shared_ptr<SlippyMapTilerOptions> options;
	GDALDatasetH dataset;
	double geo_transform[6];
	int img_width, img_height;
	double min_x, min_y, max_x, max_y;
	int band_count;
	GDALDataType data_type;

	// 坐标系统和转换
	std::unique_ptr<CoordinateSystem> coord_system;

	std::shared_ptr<JemallocAllocator> memory_allocator;
	std::shared_ptr<FileBufferManager> file_buffer;

	// 统计信息
	std::atomic<int> total_tiles_processed;
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

	//// 使用非锁定的缓存来存储常用的变量
	//struct {
	//	int tile_size = 256;
	//} config;

	// 创建输出目录
	void create_directories(int zoom);

	/**
	 * @brief 获取某一级别的瓦片范围
	 * @param zoom 缩放级别
	 * @param min_tile_x 最小X瓦片坐标
	 * @param min_tile_y 最小Y瓦片坐标
	 * @param max_tile_x 最大X瓦片坐标
	 * @param max_tile_y 最大Y瓦片坐标
	 */
	void get_tile_range(int zoom, int& min_tile_x, int& min_tile_y, int& max_tile_x, int& max_tile_y);

	/**
	 * @brief 计算指定瓦片在WGS84中的经纬度范围
	 * @param zoom 缩放级别
	 * @param tile_x 瓦片X坐标
	 * @param tile_y 瓦片Y坐标
	 * @param tile_min_x 瓦片最小经度
	 * @param tile_min_y 瓦片最小纬度
	 * @param tile_max_x 瓦片最大经度
	 * @param tile_max_y 瓦片最大纬度
	 */
	void get_tile_geo_bounds(int zoom, int tile_x, int tile_y, double& tile_min_x, double& tile_min_y, double& tile_max_x, double& tile_max_y);

	/**
	 * @brief 从瓦片地理坐标到原始影像像素坐标的转换
	 * @param geo_x 地理X坐标
	 * @param geo_y 地理Y坐标
	 * @param pixel_x 输出的像素X坐标
	 * @param pixel_y 输出的像素Y坐标
	 * @return 是否转换成功
	 */
	bool geo_to_pixel(const double& geo_x, const double& geo_y, int& pixel_x, int& pixel_y);

	/**
	* @brief 生成单个瓦片
	* @param zoom 缩放级别
	* @param tile_x 瓦片X坐标
	* @param tile_y 瓦片Y坐标
	* @return 是否成功生成瓦片
	*/
	bool generate_tile(int zoom, int tile_x, int tile_y);

	/**
	* @brief 处理指定缩放级别的所有瓦片
	* @param zoom 缩放级别
	*/
	void process_zoom_level(int zoom,std::shared_ptr<IProgressInfo> progressInfo);
};
}