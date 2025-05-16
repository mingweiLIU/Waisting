#include "SlippyTiler.h"

#include "ImageFileIOAdapter.h"
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
		min_tile_x = long2tilex(min_x, zoom);// static_cast<int>(floor((min_x + 180.0) / 360.0 * (1 << zoom)));
		min_tile_y = lat2tiley(min_y,zoom);// static_cast<int>(floor((1.0 - log(tan(min_y * M_PI / 180.0) + 1.0 / cos(min_y * M_PI / 180.0)) / M_PI) / 2.0 * (1 << zoom)));

		// 计算最大瓦片坐标
		max_tile_x = long2tilex(max_x, zoom);//static_cast<int>(floor((max_x + 180.0) / 360.0 * (1 << zoom)));
		max_tile_y = lat2tiley(max_y, zoom);//static_cast<int>(floor((1.0 - log(tan(max_y * M_PI / 180.0) + 1.0 / cos(max_y * M_PI / 180.0)) / M_PI) / 2.0 * (1 << zoom)));

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

			// 确保x目录存在
			{
				std::lock_guard<std::mutex> lock(fs_mutex);
				if (!fs::exists(x_dir)) {
					fs::create_directories(x_dir);
				}
			}
			std::string tile_path = (x_dir / (std::to_string(tile_y) + "." + options->outputFormat)).string();

			// 计算瓦片在原始影像中的像素范围
			int src_min_x, src_min_y, src_max_x, src_max_y;
			if (!geo_to_pixel(tile_min_x, tile_max_y, src_min_x, src_min_y) ||
				!geo_to_pixel(tile_max_x, tile_min_y, src_max_x, src_max_y)) {
				return false; // 坐标转换失败
			}

			// 边界检查
			if (src_max_x < 0 || src_min_x >= img_width || src_max_y < 0 || src_min_y >= img_height) {
				return false; // 瓦片在影像范围外
			}

			// 裁剪范围修正
			src_min_x = std::max(0, src_min_x);
			src_min_y = std::max(0, src_min_y);
			src_max_x = std::min(img_width - 1, src_max_x);
			src_max_y = std::min(img_height - 1, src_max_y);

			// 计算读取的数据大小
			int width = src_max_x - src_min_x + 1;
			int height = src_max_y - src_min_y + 1;

			if (width <= 0 || height <= 0) {
				return false;
			}

			// 分配内存为每个波段创建缓冲区
			size_t pixel_size = GDALGetDataTypeSize(data_type) / 8;
			//这里需要定义下 如果输出jpg 其只支持8位，如果是png 其只支持8或者16位，大于16位的我们用8位代替 那么像素位不管是多少 都应为1 然后gdal会自动将数值范围缩放
			if (options->outputFormat=="jpg")
			{
				pixel_size = 1;
			}
			else if (options->outputFormat == "png") {
				pixel_size = pixel_size > 2 ? 1 : pixel_size;
			}
			size_t buffer_size = options->tileSize * options->tileSize * band_count * pixel_size;

			//这里得到的pData是rgb rgb这样的形式
			//unsigned char * pData = (unsigned char*) malloc(buffer_size);
			
			unsigned char* pData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(buffer_size));
			//std::unique_ptr<unsigned char[]> pData(new unsigned char[buffer_size]);
			//if (!pData) {
			//	std::cerr << "内存分配失败!" << std::endl;
			//	return false;
			//}

			// 读取数据 - 使用线程本地的dataset
			CPLErr err;
			{
				// 设置内存布局参数
				std::lock_guard<std::mutex> lock(gdal_mutex);
				err = GDALDatasetRasterIO(
					local_dataset, GF_Read,
					src_min_x, src_min_y, width, height,
					pData, options->tileSize, options->tileSize,
					data_type, band_count, nullptr,
					band_count, options->tileSize* band_count, 1
				);
			}

			if (err != CE_None) {
				free(pData);
				return false;
			}
		
			// 将内存数据移动到vector中
			fs::path file = fs::path(std::to_string(zoom)) / std::to_string(tile_x) / std::to_string(tile_y);
			IOFileInfo* oneFileInfo = new IOFileInfo{ file.string(),pData,buffer_size,this->getName()};
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
		if (current % 50 == 0 || current == total_tiles) {
			std::lock_guard<std::mutex> lock(progress_mutex);
			progressInfo->showProgress(current, std::to_string(zoom) + "/" + std::to_string(tile_x) + "/" + std::to_string(tile_y), "数据切片");
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
		total_tiles_processed = 0;

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

		tbb::enumerable_thread_specific<ThreadLocalData> tls_data;

		// 使用TBB并行处理
		tbb::parallel_for(
			tbb::blocked_range2d<int, int>(min_tile_y, max_tile_y + 1, min_tile_x, max_tile_x + 1),
			[this, zoom, total_tiles, &progressInfo, &tls_data](const tbb::blocked_range2d<int, int>& r) {
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
			tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, options->numThreads);
		}

		// 打开数据集
		GDALOpenInfo* poOpenInfo = new GDALOpenInfo(options->inputFile.c_str(), GA_ReadOnly);

		// 检查是否是GDAL支持的格式
		if (!GDALIdentifyDriver(poOpenInfo->pszFilename, nullptr)) {
			std::cerr << "无法识别输入文件格式: " << options->inputFile << std::endl;
			delete poOpenInfo;
			return false;
		}

		// 使用VSI文件系统
		if (options->useMemoryMapping) {
			
		}
		else {
			// 直接打开文件
			delete poOpenInfo;
			dataset = GDALOpen(options->inputFile.c_str(), GA_ReadOnly);
		}

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
		coord_system->transform_points(xBounds,yBounds, 4);

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
		std::unique_ptr<ImageFileIOAdapter> imageIOAdatper = std::make_unique<ImageFileIOAdapter>(options->outputDir, true
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