#include "SlippyTiler.h"
namespace WT{
	SlippyMapTiler::SlippyMapTiler(std::shared_ptr<SlippyMapTilerOptions> options) {
		this->options = options;
		// 初始化GDAL
		GDALAllRegister();

		// 创建内存池
		memory_pool = std::make_shared<MemoryPool>(1024 * 1024 * 10); // 10MB 块大小

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
			fs::path x_dir = fs::path(output_dir) / std::to_string(zoom) / std::to_string(tile_x);
			std::string tile_path = (x_dir / (std::to_string(tile_y) + "." + format)).string();

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
			GDALDatasetH memDS = GDALCreate(memDriver, "", tile_size, tile_size, bands, data_type, nullptr);

			if (!memDS) {
				memory_allocator->deallocate(pData);
				return false;
			}

			// 写入数据到内存数据集，需要重采样
			err = GDALDatasetRasterIO(
				memDS, GF_Write,
				0, 0, tile_size, tile_size,
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
			GDALDriverH outputDriver = GDALGetDriverByName(format.c_str());
			if (!outputDriver) {
				GDALClose(memDS);
				return false;
			}

			// 创建内存文件
			CPLStringList options;
			if (format == "JPEG") {
				options.AddString("QUALITY=90");
			}
			else if (format == "PNG") {
				options.AddString("COMPRESS=DEFLATE");
			}
			else if (format == "WEBP") {
				options.AddString("QUALITY=80");
			}

			// 使用内存文件系统创建临时文件
			std::string vsimem_filename = "/vsimem/temp_tile_" + std::to_string(zoom) + "_" +
				std::to_string(tile_x) + "_" + std::to_string(tile_y);

			GDALDatasetH outDS = GDALCreateCopy(
				outputDriver, vsimem_filename.c_str(),
				memDS, FALSE, options.List(), nullptr, nullptr
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

	bool SlippyMapTiler::initialize()
	{
		// 打开数据集
		dataset = GDALOpen(options->inputFile.c_str(), GA_ReadOnly);
		if (!dataset) {
			std::cerr << "无法打开输入文件: " << options->inputFile << std::endl;
			return false;
		}

		// 获取地理变换参数
		if (GDALGetGeoTransform(dataset, geo_transform) != CE_None) {
			std::cerr << "无法获取地理变换参数" << std::endl;
			GDALClose(dataset);
			dataset = nullptr;
			return false;
		}

		// 获取影像尺寸
		img_width = GDALGetRasterXSize(dataset);
		img_height = GDALGetRasterYSize(dataset);

		// 计算影像的边界范围 (WGS84经纬度)
		min_x = geo_transform[0];
		max_y = geo_transform[3];
		max_x = geo_transform[0] + img_width * geo_transform[1] + img_height * geo_transform[2];
		min_y = geo_transform[3] + img_width * geo_transform[4] + img_height * geo_transform[5];

		// 创建输出根目录
		fs::create_directories(options->outputDir);

		return true;
	}

	bool SlippyMapTiler::process(std::shared_ptr<IProgressInfo> progressInfo)
	{
		if (!dataset) {
			std::cerr << "数据集未初始化" << std::endl;
			return false;
		}

		// 设置TBB线程数量
		tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, options->numThreads);

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

							// 显示进度
							progressInfo->showProgress(processed_tiles, std::to_string(zoom)+"/"+std::to_string(x)+"/"+std::to_string(y), "数据切片");
						}
					}
				}
			);

			progressInfo->finished();
		}

		return true;
	}
}