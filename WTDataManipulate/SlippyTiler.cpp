#include "SlippyTiler.h"


namespace WT{
	SlippyMapTiler::SlippyMapTiler(std::shared_ptr<SlippyMapTilerOptions> options) {
		this->options = options;
		// 初始化GDAL
		GDALAllRegister();

		// 创建内存池
		memory_allocator = std::make_shared<JemallocAllocator>(1024 * 1024 * 10); // 10MB 块大小

		// 创建文件缓冲管理器
		file_buffer = std::make_shared<FileBufferManager>(5000); // 允许5000个文件缓存

		// 如果没有指定线程数，使用系统逻辑CPU数量的一半
		if (this->options->numThreads <= 0) {
			this->options->numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency() / 2));
		}
	}

	SlippyMapTiler::~SlippyMapTiler()
	{
		// 确保释放资源
		if (dataset) {
			GDALClose(dataset);
		}
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
		min_tile_x = static_cast<int>(floor((min_x + 180.0) / 360.0 * (1 << zoom)));
		min_tile_y = static_cast<int>(floor((1.0 - log(tan(min_y * M_PI / 180.0) + 1.0 / cos(min_y * M_PI / 180.0)) / M_PI) / 2.0 * (1 << zoom)));

		// 计算最大瓦片坐标
		max_tile_x = static_cast<int>(floor((max_x + 180.0) / 360.0 * (1 << zoom)));
		max_tile_y = static_cast<int>(floor((1.0 - log(tan(max_y * M_PI / 180.0) + 1.0 / cos(max_y * M_PI / 180.0)) / M_PI) / 2.0 * (1 << zoom)));

		// 确保y坐标的大小关系正确（在切片坐标系中，y值从上到下增加）
		if (min_tile_y > max_tile_y) {
			std::swap(min_tile_y, max_tile_y);
		}
	}

	void SlippyMapTiler::get_tile_geo_bounds(int zoom, int tile_x, int tile_y, double& tile_min_x, double& tile_min_y, double& tile_max_x, double& tile_max_y)
	{
		// 瓦片经纬度范围
		tile_min_x = tile_x * 360.0 / (1 << zoom) - 180.0;
		tile_max_x = (tile_x + 1) * 360.0 / (1 << zoom) - 180.0;

		tile_max_y = atan(sinh(M_PI * (1 - 2 * tile_y / (1 << zoom)))) * 180.0 / M_PI;
		tile_min_y = atan(sinh(M_PI * (1 - 2 * (tile_y + 1) / (1 << zoom)))) * 180.0 / M_PI;
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

			if (!OCTTransform(inv_transform, 1, &x, &y, nullptr)) {
				OCTDestroyCoordinateTransformation(inv_transform);
				return false;
			}

			OCTDestroyCoordinateTransformation(inv_transform);
		}

		// 使用地理变换参数将坐标转换为像素坐标
		double det = geo_transform[1] * geo_transform[5] - geo_transform[2] * geo_transform[4];
		if (fabs(det) < 1e-10) {
			return false; // 无法转换
		}

		double inv_det = 1.0 / det;

		double dx = x - geo_transform[0];
		double dy = y - geo_transform[3];

		pixel_x = static_cast<int>(std::round((dx * geo_transform[5] - dy * geo_transform[2]) * inv_det));
		pixel_y = static_cast<int>(std::round((dy * geo_transform[1] - dx * geo_transform[4]) * inv_det));

		return true;
	}

	bool SlippyMapTiler::generate_tile(int zoom, int tile_x, int tile_y)
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

			// 获取有效波段数量
			int bands = std::min(band_count, 4); // 最多支持4个波段(RGBA)

			// 分配内存为每个波段创建缓冲区
			size_t pixel_size = GDALGetDataTypeSize(data_type) / 8;
			size_t buffer_size = width * height * bands * pixel_size;

			// 使用jemalloc分配内存
			void* pData = memory_allocator->allocate(buffer_size);
			if (!pData) {
				std::cerr << "内存分配失败!" << std::endl;
				return false;
			}

			// 读取数据
			CPLErr err = GDALDatasetRasterIO(
				dataset, GF_Read,
				src_min_x, src_min_y, width, height,
				pData, width, height,
				data_type, bands, nullptr,
				0, 0, 0
			);

			if (err != CE_None) {
				memory_allocator->deallocate(pData);
				return false;
			}

			// 创建目标瓦片数据集
			GDALDriverH memDriver = GDALGetDriverByName("MEM");
			GDALDatasetH memDS = GDALCreate(memDriver, "", options->tileSize, options->tileSize, bands, data_type, nullptr);

			if (!memDS) {
				memory_allocator->deallocate(pData);
				return false;
			}

			// 写入数据到内存数据集，需要重采样
			err = GDALDatasetRasterIO(
				memDS, GF_Write,
				0, 0, options->tileSize, options->tileSize,
				pData, width, height,
				data_type, bands, nullptr,
				0, 0, 0
			);

			// 释放原始数据内存
			memory_allocator->deallocate(pData);

			if (err != CE_None) {
				GDALClose(memDS);
				return false;
			}

			// 创建输出驱动
			GDALDriverH outputDriver = GDALGetDriverByName(options->outputFormat.c_str());
			if (!outputDriver) {
				GDALClose(memDS);
				return false;
			}

			// 创建内存文件
			CPLStringList optionsList;
			if (options->outputFormat == "JPEG") {
				optionsList.AddString("QUALITY=90");
			}
			else if (options->outputFormat == "PNG") {
				optionsList.AddString("COMPRESS=DEFLATE");
			}
			else if (options->outputFormat == "WEBP") {
				optionsList.AddString("QUALITY=80");
			}

			// 使用内存文件系统创建临时文件
			std::string vsimem_filename = "/vsimem/temp_tile_" + std::to_string(zoom) + "_" +
				std::to_string(tile_x) + "_" + std::to_string(tile_y);

			GDALDatasetH outDS = GDALCreateCopy(
				outputDriver, vsimem_filename.c_str(),
				memDS, FALSE, optionsList.List(), nullptr, nullptr
			);

			// 关闭数据集
			GDALClose(memDS);

			if (!outDS) {
				VSIUnlink(vsimem_filename.c_str());
				return false;
			}

			GDALClose(outDS);

			// 从内存文件系统读取数据
			vsi_l_offset size = 0;
			unsigned char* data = VSIGetMemFileBuffer(vsimem_filename.c_str(), &size, TRUE);

			if (data && size > 0) {
				// 将内存数据移动到vector中
				std::vector<unsigned char> buffer_data(data, data + size);
				CPLFree(data); // 释放GDAL分配的内存

				// 将文件加入写入队列
				file_buffer->write_file(tile_path, std::move(buffer_data));
				return true;
			}

			return false;
		}
		catch (const std::exception& e) {
			std::cerr << "瓦片生成异常: " << e.what() << std::endl;
			return false;
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

		// 使用TBB并行处理
		tbb::parallel_for(
			tbb::blocked_range2d<int, int>(min_tile_y, max_tile_y + 1, min_tile_x, max_tile_x + 1),
			[this, zoom, total_tiles,&progressInfo](const tbb::blocked_range2d<int, int>& r) {
				for (int tile_y = r.rows().begin(); tile_y != r.rows().end(); ++tile_y) {
					for (int tile_x = r.cols().begin(); tile_x != r.cols().end(); ++tile_x) {
						// 生成单个瓦片
						if (generate_tile(zoom, tile_x, tile_y)) {
							// 更新进度
							int processed = ++total_tiles_processed;
							if (processed % 50 == 0 || processed == total_tiles) {
								// 显示进度
								progressInfo->showProgress(processed, std::to_string(zoom) + "/" + std::to_string(tile_x) + "/" + std::to_string(tile_y), "数据切片");
							}
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
			delete poOpenInfo;
			std::string virtual_file = "/vsimem/input_" + std::to_string(std::time(nullptr));
			std::cout << "使用内存映射方式加载文件: " << options->inputFile << std::endl;

			// 使用GDAL的VSI文件系统进行文件读取，而非系统调用
			// 这种方法跨平台，不依赖于特定操作系统的mmap系统调用

			// 打开文件以读取
			VSILFILE* fp = VSIFOpenL(options->inputFile.c_str(), "rb");
			if (fp == nullptr) {
				std::cerr << "无法打开输入文件: " << options->inputFile << std::endl;
				return false;
			}

			// 获取文件大小
			VSIFSeekL(fp, 0, SEEK_END);
			size_t file_size = static_cast<size_t>(VSIFTellL(fp));
			VSIFSeekL(fp, 0, SEEK_SET);

			// 分配内存
			GByte* buffer = static_cast<GByte*>(VSIMalloc(file_size));
			if (buffer == nullptr) {
				std::cerr << "内存分配失败: " << options->inputFile << std::endl;
				VSIFCloseL(fp);
				return false;
			}

			// 读取整个文件到内存
			if (VSIFReadL(buffer, 1, file_size, fp) != file_size) {
				std::cerr << "读取文件到内存失败: " << options->inputFile << std::endl;
				VSIFree(buffer);
				VSIFCloseL(fp);
				return false;
			}

			// 关闭原始文件
			VSIFCloseL(fp);

			// 创建GDAL虚拟文件
			VSILFILE* vsi_mem_fp = VSIFileFromMemBuffer(virtual_file.c_str(), buffer, file_size, TRUE);

			if (vsi_mem_fp == nullptr) {
				std::cerr << "创建内存文件失败: " << options->inputFile << std::endl;
				VSIFree(buffer);
				return false;
			}

			// 现在使用虚拟文件打开数据集
			dataset = GDALOpen(virtual_file.c_str(), GA_ReadOnly);

			if (dataset == nullptr) {
				std::cerr << "打开数据集失败: " << virtual_file << std::endl;
				// 注意：不要关闭vsi_mem_fp，因为VSIFileFromMemBuffer已经将其管理起来
				// 当我们不再需要buffer时，会被GDAL清理
				return false;
			}
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
		double corners[8] = {
			0, 0,                   // 左上角
			img_width, 0,           // 右上角
			img_width, img_height,  // 右下角
			0, img_height           // 左下角
		};

		// 应用地理变换
		for (int i = 0; i < 4; ++i) {
			double x = corners[i * 2];
			double y = corners[i * 2 + 1];

			// 像素坐标转为地理坐标
			double geo_x = geo_transform[0] + x * geo_transform[1] + y * geo_transform[2];
			double geo_y = geo_transform[3] + x * geo_transform[4] + y * geo_transform[5];

			corners[i * 2] = geo_x;
			corners[i * 2 + 1] = geo_y;
		}

		// 转换到WGS84坐标系
		coord_system->transform_points(corners, 4);

		// 找出地理范围
		min_x = std::numeric_limits<double>::max();
		min_y = std::numeric_limits<double>::max();
		max_x = std::numeric_limits<double>::lowest();
		max_y = std::numeric_limits<double>::lowest();

		for (int i = 0; i < 4; ++i) {
			min_x = std::min(min_x, corners[i * 2]);
			max_x = std::max(max_x, corners[i * 2]);
			min_y = std::min(min_y, corners[i * 2 + 1]);
			max_y = std::max(max_y, corners[i * 2 + 1]);
		}

		// 确保范围在有效的经纬度范围内
		min_x = std::max(min_x, MIN_LONGITUDE);
		max_x = std::min(max_x, MAX_LONGITUDE);
		min_y = std::max(min_y, MIN_LATITUDE);
		max_y = std::min(max_y, MAX_LATITUDE);

		std::cout << "地理范围: "
			<< "经度=" << min_x << "至" << max_x
			<< ", 纬度=" << min_y << "至" << max_y << std::endl;

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

		//计算所有的瓦片数 方便进度条的处理
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
			process_zoom_level(zoom,progressInfo);
		}

		// 等待所有文件写入完成
		std::cout << "等待文件写入完成..." << std::endl;
		file_buffer->wait_completion();

		// 计算总处理时间
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

		// 输出统计信息
		size_t total_bytes = file_buffer->get_bytes_written();
		size_t total_files = file_buffer->get_files_written();

		std::cout << "\n切片完成!" << std::endl;
		std::cout << "总瓦片数: " << total_files << std::endl;
		std::cout << "总数据量: " << (total_bytes / (1024.0 * 1024.0)) << " MB" << std::endl;
		std::cout << "总处理时间: " << duration << " 秒" << std::endl;
		if (duration > 0) {
			std::cout << "平均速度: " << (total_files / duration) << " 瓦片/秒, "
				<< (total_bytes / (1024.0 * 1024.0) / duration) << " MB/秒" << std::endl;
		}

		return true;



		if (!dataset) {
			std::cerr << "数据集未初始化" << std::endl;
			return false;
		}

		// 设置TBB线程数量
		tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, options->numThreads);

		

		// 处理所有缩放级别
		for (int zoom = options->minLevel; zoom <= options->maxLevel; zoom++) {
			std::cout << "处理缩放级别: " << zoom << std::endl;

			// 创建该缩放级别的目录
			create_directories(zoom);

			// 计算该级别的瓦片范围
			int min_tile_x, min_tile_y, max_tile_x, max_tile_y;
			get_tile_range(zoom, min_tile_x, min_tile_y, max_tile_x, max_tile_y);

			std::cout << "  瓦片范围: X(" << min_tile_x << "-" << max_tile_x
				<< "), Y(" << min_tile_y << "-" << max_tile_y << ")" << std::endl;
			std::atomic<int> processed_tiles(0);

			// 使用TBB并行处理
			tbb::parallel_for(
				tbb::blocked_range2d<int, int>(min_tile_y, max_tile_y + 1, min_tile_x, max_tile_x + 1),
				[&](const tbb::blocked_range2d<int, int>& r) {
					for (int y = r.rows().begin(); y != r.rows().end(); ++y) {
						for (int x = r.cols().begin(); x != r.cols().end(); ++x) {
							if (generate_tile(zoom, x, y)) {
								processed_tiles++;
							}

							
						}
					}
				}
			);

			progressInfo->finished();
		}

		return true;
	}
}