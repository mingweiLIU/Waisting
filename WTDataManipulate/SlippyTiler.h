#pragma once
// 切片生成类
#include <filesystem>
#include <atomic>
#include <mutex>
// GDAL库
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

// TBB库
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/global_control.h>
#include <tbb/enumerable_thread_specific.h>

#include "FileBatchOutput.h"
#include "IDataM.h"
#include "CoordinateSystemManager.h"

namespace fs = std::filesystem;

namespace WT{
class SlippyMapTilerOptions :public IDataOptions {
public:
	std::string inputFile="";//输入的文件
	std::string outputDir="";//输出路径
	int minLevel = 15;//最小切片级数
	int maxLevel = 15;//最大切片级数
	int tileSize = 255;//瓦片大小
	bool useVRT = false;//是否使用VRT
	bool useMemoryMapping = false;//是否启用文件映射
	std::string outputFormat = "GTiff";//输出瓦片后缀
	int numThreads = 1;//使用的线程数
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
	GDALDatasetH dataset=nullptr;
	double geo_transform[6];
	int img_width, img_height;
	double min_x, min_y, max_x, max_y;
	int band_count;
	GDALDataType data_type;

	// 坐标系统和转换
	std::unique_ptr<CoordinateSystem> coord_system;

	/*std::shared_ptr<JemallocAllocator> memory_allocator;
	std::shared_ptr<FileBufferManager> file_buffer;*/
	std::shared_ptr<FileBatchOutput> fileBatchOutputer;

	// 统计信息
	std::atomic<int> total_tiles_processed;
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

	// 多线程同步用的互斥锁
	std::mutex gdal_mutex;
	std::mutex progress_mutex;
	std::mutex fs_mutex;

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
	bool generate_tile(int zoom, int tile_x, int tile_y, GDALDatasetH local_dataset);

	/**
	* @brief 处理指定缩放级别的所有瓦片
	* @param zoom 缩放级别
	*/
	void process_zoom_level(int zoom,std::shared_ptr<IProgressInfo> progressInfo);


	/**
	 * 将经度转换为瓦片坐标 X 值（基于 Web Mercator 投影）
	 *
	 * @param lon 经度（单位：度，范围 -180 到 180）
	 * @param z 缩放级别（通常 0-20，取决于地图服务）
	 * @return 瓦片 X 坐标（从 0 开始）
	 */
	int long2tilex(double lon, int z);

	/**
	 * 将纬度转换为瓦片坐标 Y 值（基于 Web Mercator 投影）
	 *
	 * @param lat 纬度（单位：度，范围 -85.0511 到 85.0511，超出会被投影截断）
	 * @param z 缩放级别（通常 0-20，取决于地图服务）
	 * @return 瓦片 Y 坐标（从 0 开始）
	 */
	int lat2tiley(double lat, int z);

	/**
	 * 将瓦片 X 坐标转换回经度（基于 Web Mercator 投影）
	 *
	 * @param x 瓦片 X 坐标（从 0 开始）
	 * @param z 缩放级别（必须与瓦片坐标生成时一致）
	 * @return 经度（单位：度，范围 -180 到 180）
	 */
	double tilex2long(int x, int z);

	/**
	 * 将瓦片 Y 坐标转换回纬度（基于 Web Mercator 投影）
	 *
	 * @param y 瓦片 Y 坐标（从 0 开始）
	 * @param z 缩放级别（必须与瓦片坐标生成时一致）
	 * @return 纬度（单位：度，范围 -85.0511 到 85.0511）
	 */
	double tiley2lat(int y, int z);
	
	// 创建一个线程本地的GDAL数据集
	GDALDatasetH create_local_dataset();

	// 增加一个线程安全的进度更新函数
	void SlippyMapTiler::update_progress(int zoom, int tile_x, int tile_y, int total_tiles, std::shared_ptr<IProgressInfo> progressInfo);
};
}