#include "SlippyTiler.h"
#include <cmath>
// TBB库
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/global_control.h>
#include <tbb/enumerable_thread_specific.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include<fstream>

//#include "ImageFileIOAdapter.h"
#include "ImageFileParallelIOAdapter.h"
#include "MemoryPool.h"

namespace WT{
	SlippyMapTiler::SlippyMapTiler(std::shared_ptr<SlippyMapTilerOptions> options) {
		this->options = options;
		// 初始化地理变换参数
		for (int i = 0; i < 6; ++i) {
			geo_transform[i] = 0.0;
		}
				
		// 创建坐标系统对象
		coord_system = std::make_unique<CoordinateSystem>();
	}

	SlippyMapTiler::~SlippyMapTiler()
	{
		// 确保释放资源
		if (dataset) {
			GDALClose(dataset);
			dataset = nullptr;
		}
		GDALDestroyDriverManager();
		//这里要清除内存池的内容
		MemoryPool::releaseInstance(this->getName());
	}

	void SlippyMapTiler::setOptions(std::shared_ptr<IDataOptions> options) {
		this->options = std::dynamic_pointer_cast<SlippyMapTilerOptions>(options);
	}

	void SlippyMapTiler::create_directories(int zoom)
	{
		std::string outputDir = options->outputDir;
		fs::path zoom_dir = fs::path(outputDir) / std::to_string(zoom);
		if (!fs::exists(zoom_dir)) {
			fs::create_directories(zoom_dir);
		}
	}

	void SlippyMapTiler::get_tile_range(int zoom, int& min_tile_x, int& min_tile_y, int& max_tile_x, int& max_tile_y)
	{
		// 计算最小瓦片坐标
		min_tile_x = long2tilex(min_x, zoom);
		min_tile_y = lat2tiley(min_y,zoom);

		// 计算最大瓦片坐标
		max_tile_x = long2tilex(max_x, zoom);
		max_tile_y = lat2tiley(max_y, zoom);

		// 确保y坐标的大小关系正确（在切片坐标系中，y值从上到下增加）
		if (min_tile_y > max_tile_y) {
			std::swap(min_tile_y, max_tile_y);
		}
	}

	void SlippyMapTiler::get_tile_geo_bounds(int zoom, int tile_x, int tile_y, double& tile_min_x, double& tile_min_y, double& tile_max_x, double& tile_max_y)
	{
		// 瓦片经纬度范围
		tile_min_x = tilex2long(tile_x,zoom);
		tile_max_x = tilex2long(tile_x+1, zoom);

		tile_max_y = tiley2lat(tile_y,zoom);
		tile_min_y = tiley2lat(tile_y+1, zoom);
	}

	bool SlippyMapTiler::geo_to_pixel(const double& geo_x, const double& geo_y, int& pixel_x, int& pixel_y)
	{
		double x = geo_x;
		double y = geo_y;

		// 如果需要坐标转换，将WGS84坐标转换为影像原始坐标系
		if (coord_system->requires_transform()) {
			OGRCoordinateTransformationH inv_transform = coord_system->create_inverse_transform();

			if (!inv_transform) {
				return false;
			}

			if (!OCTTransform(inv_transform, 1,  &y, &x, nullptr)) {
				OCTDestroyCoordinateTransformation(inv_transform);
				return false;
			}

			OCTDestroyCoordinateTransformation(inv_transform);
		}

		// 使用地理变换参数将坐标转换为像素坐标
		pixel_x = int((y - geo_transform[0]) / geo_transform[1]);
		pixel_y = int((x - geo_transform[3]) / geo_transform[5]);

		return true;
	}

	int SlippyMapTiler::long2tilex(double lon, int z)
	{
		return (int)(floor((lon + 180.0) / 360.0 * (1 << z)));
	}

	double SlippyMapTiler::tilex2long(int x, int z)
	{
		return x / (double)(1 << z) * 360.0 - 180;
	}

	int SlippyMapTiler::lat2tiley(double lat, int z)
	{
		double latrad = lat * M_PI / 180.0;  // 转换为弧度
		return (int)(floor((1.0 - asinh(tan(latrad)) / M_PI) / 2.0 * (1 << z)));
	}

	double SlippyMapTiler::tiley2lat(int y, int z)
	{
		double n = M_PI - 2.0 * M_PI * y / (double)(1 << z);
		return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));  // 反双曲正切计算
	}

	double SlippyMapTiler::mapSize(int level, int tileSize)
	{
		return  std::ceil(tileSize * (double)(1<<level));
	}

	double SlippyMapTiler::groundResolution(double latitude, double level, int tileSize)
	{
		return cos(latitude * M_PI / 180) * 2 * M_PI * EARTH_RADIUS / mapSize(level, tileSize);
	}

	int SlippyMapTiler::getProperLevel(double groundResolution,int tileSize)
	{
		const double EARTH_CIRCUMFERENCE = 40075016.686;  // 地球赤道周长（米）

		// OSM 在 zoom=0 时的分辨率（米/像素）
		const double RESOLUTION_0 = EARTH_CIRCUMFERENCE / tileSize;

		// 计算最佳 zoom（理论值可能是小数）
		double zoom = std::log2(RESOLUTION_0 / groundResolution);

		// 取整（向下取整，避免分辨率过高）
		int bestZoom = static_cast<int>(std::floor(zoom));

		return bestZoom;
	}

	// 创建一个线程本地的GDAL数据集
	GDALDatasetH SlippyMapTiler::create_local_dataset() {
		std::lock_guard<std::mutex> lock(gdal_mutex);
		return GDALOpen(options->inputFile.c_str(), GA_ReadOnly);
	}

	bool SlippyMapTiler::generate_tile(int zoom, int tile_x, int tile_y, GDALDatasetH local_dataset)
	{
		try {
			// 计算瓦片的地理范围（在WGS84坐标系下）
			double tile_min_x, tile_min_y, tile_max_x, tile_max_y;
			get_tile_geo_bounds(zoom, tile_x, tile_y, tile_min_x, tile_min_y, tile_max_x, tile_max_y);

			// 创建瓦片路径
			fs::path x_dir = fs::path(options->outputDir) / std::to_string(zoom) / std::to_string(tile_x);
			std::string tile_path = (x_dir / (std::to_string(tile_y) + "." + options->outputFormat)).string();

			// 计算瓦片在原始影像中的像素范围
			int src_min_x, src_min_y, src_max_x, src_max_y;
			if (!geo_to_pixel(tile_min_x, tile_max_y, src_min_x, src_min_y) ||
				!geo_to_pixel(tile_max_x, tile_min_y, src_max_x, src_max_y)) {
				return false; // 坐标转换失败
			}

			// 检查瓦片是否完全在影像范围外
			if (src_max_x < 0 || src_min_x >= img_width ||
				src_max_y < 0 || src_min_y >= img_height) {
				return false; // 瓦片完全在影像范围外
			}

			// 计算目标瓦片中需要填充数据的区域
			int dst_min_x = 0, dst_min_y = 0;
			int dst_max_x = options->tileSize - 1, dst_max_y = options->tileSize - 1;

			// 如果源数据范围超出影像边界，计算目标区域的偏移
			if (src_min_x < 0) {
				// 左边超出，计算目标区域的左边界
				double ratio = (double)(-src_min_x) / (src_max_x - src_min_x);
				dst_min_x = (int)(ratio * options->tileSize);
			}
			if (src_min_y < 0) {
				// 上边超出，计算目标区域的上边界
				double ratio = (double)(-src_min_y) / (src_max_y - src_min_y);
				dst_min_y = (int)(ratio * options->tileSize);
			}
			if (src_max_x >= img_width) {
				// 右边超出，计算目标区域的右边界
				double ratio = (double)(img_width - 1 - src_min_x) / (src_max_x - src_min_x);
				dst_max_x = (int)(ratio * options->tileSize);
			}
			if (src_max_y >= img_height) {
				// 下边超出，计算目标区域的下边界
				double ratio = (double)(img_height - 1 - src_min_y) / (src_max_y - src_min_y);
				dst_max_y = (int)(ratio * options->tileSize);
			}

			// 裁剪源数据范围到影像边界内
			int clipped_src_min_x = std::max(0, src_min_x);
			int clipped_src_min_y = std::max(0, src_min_y);
			int clipped_src_max_x = std::min(img_width - 1, src_max_x);
			int clipped_src_max_y = std::min(img_height - 1, src_max_y);

			// 计算实际需要读取的数据大小
			int read_width = clipped_src_max_x - clipped_src_min_x + 1;
			int read_height = clipped_src_max_y - clipped_src_min_y + 1;

			if (read_width <= 0 || read_height <= 0) {
				return false;
			}

			// 确保x目录存在
			{
				std::lock_guard lock(fs_mutex);
				if (!fs::exists(x_dir)) {
					fs::create_directories(x_dir);
				}
			}

			// 计算像素大小
			size_t pixel_size = GDALGetDataTypeSize(data_type) / 8;
			if (options->outputFormat == "jpg") {
				pixel_size = 1;
			}
			else if (options->outputFormat == "png") {
				pixel_size = pixel_size > 2 ? 1 : pixel_size;
			}

			// 分配完整瓦片大小的缓冲区
			size_t buffer_size = options->tileSize * options->tileSize * band_count * pixel_size;
			unsigned char* pData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(buffer_size));

			// 初始化整个缓冲区为0（或其他背景值）
			memset(pData, 0, buffer_size);

			// 计算目标缓冲区中的有效数据区域大小
			int dst_width = dst_max_x - dst_min_x + 1;
			int dst_height = dst_max_y - dst_min_y + 1;

			// 分配临时缓冲区用于读取实际数据
			size_t temp_buffer_size = dst_width * dst_height * band_count * pixel_size;
			unsigned char* temp_buffer = (unsigned char*)malloc(temp_buffer_size);
			if (!temp_buffer) {
				MemoryPool::GetInstance(this->getName())->deallocate(pData, buffer_size);
				return false;
			}

			// 读取数据到临时缓冲区
			CPLErr err;
			{
				std::lock_guard lock(gdal_mutex);
				err = GDALDatasetRasterIO(
					local_dataset, GF_Read,
					clipped_src_min_x, clipped_src_min_y, read_width, read_height,
					temp_buffer, dst_width, dst_height,
					data_type, band_count, nullptr,
					band_count, dst_width * band_count, 1
				);
			}

			if (err != CE_None) {
				free(temp_buffer);
				MemoryPool::GetInstance(this->getName())->deallocate(pData, buffer_size);
				return false;
			}

			// 将临时缓冲区的数据复制到目标缓冲区的正确位置
			for (int y = 0; y < dst_height; y++) {
				int dst_y = dst_min_y + y;
				for (int x = 0; x < dst_width; x++) {
					int dst_x = dst_min_x + x;

					// 计算源和目标位置的索引
					size_t src_idx = (y * dst_width + x) * band_count * pixel_size;
					size_t dst_idx = (dst_y * options->tileSize + dst_x) * band_count * pixel_size;

					// 复制像素数据
					memcpy(pData + dst_idx, temp_buffer + src_idx, band_count * pixel_size);
				}
			}

			// 释放临时缓冲区
			free(temp_buffer);

			// 将内存数据移动到vector中
			fs::path file = fs::path(std::to_string(zoom)) / std::to_string(tile_x) / std::to_string(tile_y);
			IOFileInfo* oneFileInfo = new IOFileInfo{ file.string(), pData, buffer_size, this->getName() };
			fileBatchOutputer->addFile(oneFileInfo);

			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "瓦片生成异常: " << e.what() << std::endl;
			return false;
		}
	}

	// 增加一个线程安全的进度更新函数
	void SlippyMapTiler::update_progress(int zoom, int tile_x, int tile_y, int total_tiles, std::shared_ptr<IProgressInfo> progressInfo) {
		int current = ++total_tiles_processed;
		int temp = std::max(1,total_tiles / 100);
		if (current % temp == 0 || current == total_tiles) {
			std::lock_guard<std::mutex> lock(progress_mutex);
			progressInfo->showProgress(current, "", "");
		}
	}

	void SlippyMapTiler::process_zoom_level(int zoom, std::shared_ptr<IProgressInfo> progressInfo)
	{
		std::cout << "\n处理缩放级别: " << zoom << std::endl;

		// 计算该级别的瓦片范围
		int min_tile_x, min_tile_y, max_tile_x, max_tile_y;
		get_tile_range(zoom, min_tile_x, min_tile_y, max_tile_x, max_tile_y);

		// 确保输出目录存在
		create_directories(zoom);

		// 计算总共需要处理的瓦片数量
		int tiles_wide = max_tile_x - min_tile_x + 1;
		int tiles_high = max_tile_y - min_tile_y + 1;
		int total_tiles = tiles_wide * tiles_high;

		std::cout << "瓦片范围: X=" << min_tile_x << "-" << max_tile_x
			<< ", Y=" << min_tile_y << "-" << max_tile_y
			<< " (总计 " << total_tiles << " 个瓦片)" << std::endl;

		// 重置计数器
		//total_tiles_processed = 0;

		// 使用线程本地存储处理Dataset
		struct ThreadLocalData {
			GDALDatasetH local_dataset;
			ThreadLocalData() : local_dataset(nullptr) {}
			~ThreadLocalData() {
				if (local_dataset) {
					GDALClose(local_dataset);
				}
			}
		};

		::tbb::enumerable_thread_specific<ThreadLocalData> tls_data;

		// 使用TBB并行处理
		::tbb::parallel_for(
			::tbb::blocked_range2d<int, int>(min_tile_y, max_tile_y + 1, min_tile_x, max_tile_x + 1),
			[this, zoom, total_tiles, &progressInfo, &tls_data](const ::tbb::blocked_range2d<int, int>& r) {
				// 获取线程本地存储
				ThreadLocalData& local_data = tls_data.local();

				// 懒初始化本地数据集
				if (!local_data.local_dataset) {
					local_data.local_dataset = this->create_local_dataset();
					if (!local_data.local_dataset) {
						std::cerr << "无法创建线程本地数据集" << std::endl;
						return;
					}
				}

				for (int tile_y = r.rows().begin(); tile_y != r.rows().end(); ++tile_y) {
					for (int tile_x = r.cols().begin(); tile_x != r.cols().end(); ++tile_x) {
						// 使用线程本地数据集生成瓦片
						if (generate_tile(zoom, tile_x, tile_y, local_data.local_dataset)) {
							// 更新进度
							update_progress(zoom, tile_x, tile_y, total_tiles, progressInfo);
						}
					}
				}
			}
		);
		std::cout << std::endl;
	}

	bool SlippyMapTiler::initialize()
	{
		// 注册GDAL驱动
		GDALAllRegister();

		// 设置线程数量
		if (options->numThreads > 0) {
			std::cout << "设置线程数量: " << options->numThreads << std::endl;
			::tbb::global_control global_limit(::tbb::global_control::max_allowed_parallelism, options->numThreads);
		}

		// 打开数据集
		GDALOpenInfo* poOpenInfo = new GDALOpenInfo(options->inputFile.c_str(), GA_ReadOnly);

		// 检查是否是GDAL支持的格式
		if (!GDALIdentifyDriver(poOpenInfo->pszFilename, nullptr)) {
			std::cerr << "无法识别输入文件格式: " << options->inputFile << std::endl;
			delete poOpenInfo;
			return false;
		}

		dataset = GDALOpen(options->inputFile.c_str(), GA_ReadOnly);		

		if (!dataset) {
			std::cerr << "无法打开输入文件: " << options->inputFile << std::endl;
			return false;
		}


		// 获取地理变换参数
		if (GDALGetGeoTransform(dataset, geo_transform) != CE_None) {
			std::cerr << "无法获取地理变换参数" << std::endl;
			return false;
		}

		// 获取影像尺寸
		img_width = GDALGetRasterXSize(dataset);
		img_height = GDALGetRasterYSize(dataset);

		if (img_width <= 0 || img_height <= 0) {
			std::cerr << "无效的影像尺寸: " << img_width << "x" << img_height << std::endl;
			return false;
		}

		// 获取波段数量和数据类型
		band_count = GDALGetRasterCount(dataset);
		if (band_count <= 0) {
			std::cerr << "无效的波段数量: " << band_count << std::endl;
			return false;
		}

		// 获取数据类型（使用第一个波段的类型）
		GDALRasterBandH band = GDALGetRasterBand(dataset, 1);
		if (!band) {
			std::cerr << "无法获取波段信息" << std::endl;
			return false;
		}
		data_type = GDALGetRasterDataType(band);

		// 输出影像信息
		std::cout << "影像尺寸: " << img_width << "x" << img_height
			<< ", 波段数: " << band_count
			<< ", 数据类型: " << GDALGetDataTypeName(data_type) << std::endl;

		// 初始化坐标系统
		if (!coord_system->initialize(dataset, options->prjFilePath, options->wktString)) {
			std::cerr << "初始化坐标系统失败" << std::endl;
			return false;
		}

		// 计算影像的地理范围
		double xBounds[4] = { 0,img_width,img_width,0 };
		double yBounds[4] = { 0,0,img_height,img_height };

		// 应用地理变换
		for (int i = 0; i < 4; ++i) {
			double x = xBounds[i];
			double y = yBounds[i];

			// 像素坐标转为地理坐标
			double geo_x = geo_transform[0] + x * geo_transform[1] + y * geo_transform[2];
			double geo_y = geo_transform[3] + x * geo_transform[4] + y * geo_transform[5];

			xBounds[i] = geo_x;
			yBounds[i] = geo_y;
		}

		// 转换到WGS84坐标系
		coord_system->transform_points(xBounds, yBounds, 4);

		// 找出地理范围
		min_x = std::numeric_limits<double>::max();
		min_y = std::numeric_limits<double>::max();
		max_x = std::numeric_limits<double>::lowest();
		max_y = std::numeric_limits<double>::lowest();

		for (int i = 0; i < 4; ++i) {
			min_x = std::min(min_x, yBounds[i]);
			max_x = std::max(max_x, yBounds[i]);
			min_y = std::min(min_y, xBounds[i]);
			max_y = std::max(max_y, xBounds[i]);
		}

		// 确保范围在有效的经纬度范围内
		min_x = std::max(min_x, MIN_LONGITUDE);
		max_x = std::min(max_x, MAX_LONGITUDE);
		min_y = std::max(min_y, MIN_LATITUDE);
		max_y = std::min(max_y, MAX_LATITUDE);

		std::cout << "地理范围: "
			<< "经度=" << min_x << "至" << max_x
			<< ", 纬度=" << min_y << "至" << max_y << std::endl;

		//处理要切片的层级问题
		if (options->minLevel < 0) options->minLevel = 0;
		//获取影像分辨率最佳层级
		double xResolutionM = 0, yResolutionM = 0; 
		double xOriginResolution = geo_transform[1];
		double yOriginResolution=std::abs(geo_transform[5]);

		const char* proj_wkt = GDALGetProjectionRef(dataset);
		bool has_projection = (proj_wkt && strlen(proj_wkt) > 0);
		if (has_projection)
		{
			options->maxLevel = this->getProperLevel(std::min(xOriginResolution, yOriginResolution), options->tileSize);
		}
		else {
			//就用转换为wgs84后的处理
			coord_system->calculateGeographicResolution((min_y + max_y) / 2.0, xOriginResolution, yOriginResolution, xResolutionM = 0, yResolutionM);
			options->maxLevel = this->getProperLevel(std::min(xResolutionM, yResolutionM), options->tileSize);
		}

		//下面要处理NoDataValue
		if (options->outputFormat == "png") {
			for (size_t i = 0; i < band_count; i++)
			{
				GDALRasterBandH  hBand = GDALGetRasterBand(dataset, 1+i);
				if (hBand == nullptr) {
					std::cerr << "Error: Failed to access band " << i << std::endl;
					continue;
				}
				int hasNoData = 0;
				double noDataValue = GDALGetRasterNoDataValue(hBand, &hasNoData);
				//如果有就使用原文件的 没有就使用设置的
				if (hasNoData) {
					if (options->nodata.size() == 0)
					{
						options->nodata.resize(band_count);
					}
					options->nodata[i] = noDataValue;
				}
			}
		}

		// 创建内存管理器和文件缓冲管理器
		fileBatchOutputer = std::make_shared<FileBatchOutput>();
		std::unique_ptr<ImageFileParallelIOAdapter> imageIOAdatper = std::make_unique<ImageFileParallelIOAdapter>(options->outputDir, true
			, options->tileSize, options->tileSize, options->outputFormat
			, band_count,options->nodata);
		fileBatchOutputer->setAdapter(std::move(imageIOAdatper));

		return true;
	}

	bool SlippyMapTiler::process(std::shared_ptr<IProgressInfo> progressInfo)
	{
		if (!dataset) {
			std::cerr << "数据集未初始化，请先调用initialize()" << std::endl;
			return false;
		}

		// 创建根输出目录
		if (!fs::exists(options->outputDir)) {
			if (!fs::create_directories(options->outputDir)) {
				std::cerr << "无法创建输出目录: " << options->outputDir << std::endl;
				return false;
			}
		}

		// 计算所有的瓦片数 方便进度条的处理
		int total_tiles = 0;
		for (int zoom = options->minLevel; zoom <= options->maxLevel; zoom++) {
			// 计算该级别的瓦片范围
			int min_tile_x, min_tile_y, max_tile_x, max_tile_y;
			get_tile_range(zoom, min_tile_x, min_tile_y, max_tile_x, max_tile_y);

			// 计算总瓦片数量
			total_tiles += (max_tile_x - min_tile_x + 1) * (max_tile_y - min_tile_y + 1);
		}
		progressInfo->setTotalNum(total_tiles);

		// 记录开始时间
		start_time = std::chrono::high_resolution_clock::now();

		// 处理每个缩放级别
		for (int zoom = options->minLevel; zoom <= options->maxLevel; ++zoom) {
			process_zoom_level(zoom, progressInfo);
		}

		//所有处理完毕后 需要清空
		fileBatchOutputer->output();

		//输出元数据
		nlohmann::json metaInfo;
		metaInfo["extent"] = { min_x, min_y, max_x, max_y };
		metaInfo["center"] = { (min_x + max_x) / 2,(min_y + max_y) / 2 };
		metaInfo["levels"] = { options->minLevel,options->maxLevel };
		std::ofstream fStream((fs::path(options->outputDir) / "meta.json").string().c_str());
		fStream << std::setw(4) << metaInfo << std::endl;
		fStream.close();

		// 等待所有文件写入完成
		std::cout << "等待文件写入完成..." << std::endl;

		// 计算总处理时间
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

		// 输出统计信息
		std::cout << "\n切片完成!" << std::endl;
		std::cout << "总处理时间: " << duration << " 秒" << std::endl;

		return true;
	}
}