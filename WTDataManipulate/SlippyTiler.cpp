#include "SlippyTiler.h"
#include <cmath>
// TBB库
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/global_control.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
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

			// 计算瓦片在原始影像中的像素范围
			int src_min_x, src_min_y, src_max_x, src_max_y;
			if (!geo_to_pixel(tile_min_x, tile_max_y, src_min_x, src_min_y) ||
				!geo_to_pixel(tile_max_x, tile_min_y, src_max_x, src_max_y)) {
				return false; // 坐标转换失败
			}

			// 计算瓦片边界和映射关系
			TileBounds bounds;
			if (!calculate_tile_bounds(src_min_x, src_min_y, src_max_x, src_max_y, bounds)) {
				return false; // 瓦片完全在影像范围外或无效
			}

			// 确保输出目录存在
			if (!ensure_output_directory(x_dir)) {
				return false;
			}

			// 分配完整瓦片大小的缓冲区（基于输出波段数）
			//采用这样的策略:如果是png 那么一开始就配套一个独立的透明波段 如果有透明值则为0 对应像素值不为非数据区或者nodata区 则为255 
			//这样最后统计值是否和数据相等就能判断是否需要它 不需要则去掉
			size_t buffer_size=options->tileSize * options->tileSize * image_info.output_band_count * image_info.pixel_size;			
			unsigned char* pData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(buffer_size));
			memset(pData , 0, buffer_size);

			//透明波段直接设置为8位的
			unsigned char* pAlphaData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(options->tileSize * options->tileSize));
			memset(pAlphaData, 255, options->tileSize * options->tileSize);//非透明区数据为主 所以选择初始为255

			// 根据影像类型处理数据
			bool success;
			if (image_info.has_palette) {
				success = process_palette_image(local_dataset, bounds, image_info, buffer_size,pData,pAlphaData);
			}
			else {
				success = process_regular_image(local_dataset, bounds, image_info, buffer_size,pData, pAlphaData);
			}

			if (!success) {
				MemoryPool::GetInstance(this->getName())->deallocate(pData, buffer_size);
				return false;
			}

			//下面来缩放数据
			unsigned char* scaledData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(image_info.output_band_count * options->tileSize * options->tileSize));
			if (data_type == GDT_Byte) {
				memcpy(scaledData, pData, image_info.output_band_count * options->tileSize * options->tileSize); 
			}
			else
			{
				unsigned char* scaledData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(image_info.output_band_count * options->tileSize * options->tileSize));
				scaleDataRange(pData, scaledData, maxs, mins);
				MemoryPool::GetInstance(this->getName())->deallocate(pData, buffer_size);
			}

			unsigned char* pMergedData = nullptr;//在合并函数中来分配内存
			bool merged=mergeDataAndAlpha(scaledData, pAlphaData, pMergedData, image_info.output_band_count, options->tileSize * options->tileSize);
			
			//将内存数据提交给输出器
			fs::path file = fs::path(std::to_string(zoom)) / std::to_string(tile_x) / std::to_string(tile_y);
			IOFileInfo* oneFileInfo = nullptr;//这个释放交给fileIO来
			if (!merged)
			{
				oneFileInfo = new IOFileInfo{ file.string(), scaledData,(size_t)image_info.output_band_count * options->tileSize * options->tileSize, this->getName() };
			}
			else {
				oneFileInfo = new IOFileInfo{ file.string(), pMergedData,(size_t)(image_info.output_band_count+1) * options->tileSize * options->tileSize, this->getName() };
			}
			fileBatchOutputer->addFile(oneFileInfo);

			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "瓦片生成异常: " << e.what() << std::endl;
			return false;
		}
	}

	bool SlippyMapTiler::mergeDataAndAlpha(unsigned char* pData, unsigned char* alphaData, unsigned char*& finalData, int bandCount, int pixelCount) {
		//先判断是alpha图层是否需要保留
		bool needAlpha = false;
		::tbb::task_group_context context;
		::tbb::parallel_for(0, pixelCount, 1, [&](int i) {
			if (*(alphaData+i)==0)
			{
				context.cancel_group_execution();
				needAlpha = true;
				return;
			}
		},context);
		//如果需要透明通道 那么就合并
		if (needAlpha)
		{
			finalData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate((bandCount+1)*pixelCount));
			//memset(finalData, 255, (bandCount + 1) * pixelCount);
			if (!finalData) {
				return false;
			}

			::tbb::parallel_for(::tbb::blocked_range<int>(0,pixelCount),
				[&](const ::tbb::blocked_range<int> range){
					for (int i = range.begin(); i !=range.end(); ++i)
					{
						int disIndex = (bandCount + 1) * i;
						int srcIndex = bandCount * i;
						memcpy(finalData + disIndex, pData + srcIndex, bandCount);
						memcpy(finalData + disIndex + bandCount, alphaData + i, 1);
					}
				}
			);
			return true;
		}

		return false;
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

	bool SlippyMapTiler::calculate_tile_bounds(int src_min_x, int src_min_y, int src_max_x, int src_max_y, TileBounds& bounds)
	{
		// 检查瓦片是否完全在影像范围外
		if (src_max_x < 0 || src_min_x >= img_width ||
			src_max_y < 0 || src_min_y >= img_height) {
			return false;
		}

		// 初始化目标瓦片边界
		bounds.dst_min_x = 0;
		bounds.dst_min_y = 0;
		bounds.dst_max_x = options->tileSize - 1;
		bounds.dst_max_y = options->tileSize - 1;

		// 计算目标瓦片中需要填充数据的区域
		if (src_min_x < 0) {
			// 左边超出，计算目标区域的左边界
			double ratio = (double)(-src_min_x) / (src_max_x - src_min_x);
			bounds.dst_min_x = (int)(ratio * options->tileSize);
		}
		if (src_min_y < 0) {
			// 上边超出，计算目标区域的上边界
			double ratio = (double)(-src_min_y) / (src_max_y - src_min_y);
			bounds.dst_min_y = (int)(ratio * options->tileSize);
		}
		if (src_max_x >= img_width) {
			// 右边超出，计算目标区域的右边界
			double ratio = (double)(img_width - 1 - src_min_x) / (src_max_x - src_min_x);
			bounds.dst_max_x = (int)(ratio * options->tileSize);
		}
		if (src_max_y >= img_height) {
			// 下边超出，计算目标区域的下边界
			double ratio = (double)(img_height - 1 - src_min_y) / (src_max_y - src_min_y);
			bounds.dst_max_y = (int)(ratio * options->tileSize);
		}

		// 裁剪源数据范围到影像边界内
		bounds.clipped_src_min_x = std::max(0, src_min_x);
		bounds.clipped_src_min_y = std::max(0, src_min_y);
		bounds.clipped_src_max_x = std::min(img_width - 1, src_max_x);
		bounds.clipped_src_max_y = std::min(img_height - 1, src_max_y);

		// 计算实际需要读取和目标的数据大小
		bounds.read_width = bounds.clipped_src_max_x - bounds.clipped_src_min_x + 1;
		bounds.read_height = bounds.clipped_src_max_y - bounds.clipped_src_min_y + 1;
		bounds.dst_width = bounds.dst_max_x - bounds.dst_min_x + 1;
		bounds.dst_height = bounds.dst_max_y - bounds.dst_min_y + 1;

		return bounds.read_width > 0 && bounds.read_height > 0;
	}

	ImageInfo SlippyMapTiler::get_image_info(GDALDatasetH dataset)
	{
		ImageInfo info;

		// 检查是否为带调色板的单波段影像
		GDALRasterBandH first_band = GDALGetRasterBand(dataset, 1);
		info.color_table = GDALGetRasterColorTable(first_band);
		info.has_palette = (band_count == 1 && info.color_table != nullptr);

		//计算实际的波段数
		if (info.has_palette) {
			GDALPaletteInterp paleteInterp = GDALGetPaletteInterpretation(dataset);

			switch (paleteInterp)
			{
				/*! Grayscale (in GDALColorEntry.c1) */ 
				case GPI_Gray: {
					info.output_band_count = 1;
					break;
				}
				/*! Red, Green, Blue and Alpha in (in c1, c2, c3 and c4) */ 
				case GPI_RGB: {
					info.output_band_count = 3;
					break;
				}
				/*! Cyan, Magenta, Yellow and Black (in c1, c2, c3 and c4)*/ 
				case GPI_CMYK: {
					info.output_band_count = 4;
					break;
				}
				/*! Hue, Lightness and Saturation (in c1, c2, and c3) */ 
				case GPI_HLS: {
					info.output_band_count = 3;
					break;

				}
				default: {
					// 对于未知格式，可以通过检查调色板条目的alpha值来判断
					// 如果所有条目的alpha都是255，则为RGB，否则为RGBA
					int entry_count = GDALGetColorEntryCount(info.color_table);
					bool has_alpha = false;
					for (int i = 0; i < entry_count; i++) {
						const GDALColorEntry* entry = GDALGetColorEntry(info.color_table, i);
						if (entry && entry->c4 != 255) {  // c4是alpha通道
							has_alpha = true;
							break;
						}
					}
					info.output_band_count = has_alpha ? 4 : 3;  // RGBA或RGB
					break;
				}
			}
		}
		else {
			info.output_band_count = band_count;
		}

		// 计算像素大小
		info.pixel_size = GDALGetDataTypeSizeBytes(data_type);// / 8;

		//没办法 需要自己的手动缩放
		//if (options->outputFormat == "jpg") {
		//	info.pixel_size = 1;
		//}
		//else if (options->outputFormat == "png") {
		//	info.pixel_size = info.pixel_size > 2 ? 1 : info.pixel_size;
		//}

		return info;
	}

	bool SlippyMapTiler::process_palette_image(GDALDatasetH dataset, const TileBounds& bounds, const ImageInfo& info
		, size_t dataBufferSize, unsigned char* output_buffer, unsigned char* alphaBuffer)
	{
		// 分配临时缓冲区用于读取原始索引数据
		size_t temp_buffer_size = bounds.dst_width * bounds.dst_height * info.pixel_size;
		unsigned char* temp_buffer = (unsigned char*)malloc(temp_buffer_size);
		if (!temp_buffer) {
			return false;
		}

		// 读取索引数据
		CPLErr err;
		{
			std::lock_guard lock(gdal_mutex);
			err = GDALDatasetRasterIO(
				dataset, GF_Read,
				bounds.clipped_src_min_x, bounds.clipped_src_min_y, bounds.read_width, bounds.read_height,
				temp_buffer, bounds.dst_width, bounds.dst_height,
				data_type, 1, nullptr,  // 只读取一个波段
				info.pixel_size, bounds.dst_width * info.pixel_size, 1
			);
		}

		if (err != CE_None) {
			free(temp_buffer);
			return false;
		}

		// 获取调色板条目数
		int palette_count = GDALGetColorEntryCount(info.color_table);

		// 使用TBB并行处理像素转换
		::tbb::parallel_for(
			::tbb::blocked_range2d<int>(0, bounds.dst_height, 0, bounds.dst_width),
			[&](const ::tbb::blocked_range2d<int>& range) {
				for (int y = range.rows().begin(); y != range.rows().end(); ++y) {
					int dst_y = bounds.dst_min_y + y;
					for (int x = range.cols().begin(); x != range.cols().end(); ++x) {
						int dst_x = bounds.dst_min_x + x;

						// 获取索引值
						int index;
						if (info.pixel_size == 1) {
							index = temp_buffer[y * bounds.dst_width + x];
						}
						else if (info.pixel_size == 2) {
							index = ((unsigned short*)temp_buffer)[y * bounds.dst_width + x];
						}
						else {
							index = ((unsigned int*)temp_buffer)[y * bounds.dst_width + x];
						}

						// 获取对应的RGB值
						GDALColorEntry color_entry;
						if (index >= 0 && index < palette_count) {
							GDALGetColorEntryAsRGB(info.color_table, index, &color_entry);
						}
						else {
							// 索引超出范围 那么我直接设置为nodata的值
							if (options->outputFormat == "png") {
								color_entry.c1 = options->nodata[0];
								if (image_info.output_band_count == 3) {
									color_entry.c2 = options->nodata[1];
									color_entry.c3 = options->nodata[2];
								}
							}
							else if (options->outputFormat == "jpg") {
								color_entry.c1 = 0; // Red
								color_entry.c2 = 0; // Green
								color_entry.c3 = 0; // Blue
								color_entry.c4 = 0; // Alpha
							}
						}

						// 计算目标位置的索引
						size_t dst_idx = (dst_y * options->tileSize + dst_x) * 3 * info.pixel_size;

						// 写入RGB值
						if (info.pixel_size == 1) {
							output_buffer[dst_idx] = (unsigned char)color_entry.c1;     // R
							output_buffer[dst_idx + 1] = (unsigned char)color_entry.c2; // G
							output_buffer[dst_idx + 2] = (unsigned char)color_entry.c3; // B
							//std::cout <<index<<"===="<< color_entry.c1 << "----" << color_entry.c2 << "----" << color_entry.c3 << std::endl;
						}
						else if (info.pixel_size == 2) {
							((unsigned short*)output_buffer)[dst_idx / 2] = (unsigned short)color_entry.c1;     // R
							((unsigned short*)output_buffer)[dst_idx / 2 + 1] = (unsigned short)color_entry.c2; // G
							((unsigned short*)output_buffer)[dst_idx / 2 + 2] = (unsigned short)color_entry.c3; // B
						}
					}
				}
			}
		);

		// 释放临时缓冲区
		free(temp_buffer);
		return true;
	}

	bool SlippyMapTiler::process_regular_image(GDALDatasetH dataset, const TileBounds& bounds, const ImageInfo& info
		,size_t dataBufferSize, unsigned char* output_buffer, unsigned char* alphaBuffer)
	{
		//这里分两种情况 一种是瓦片完全在影像范围内 直接读取就好了 一种不在范围内 需要读取一部分再放到原有的数据范围内 这里为了避免多余的拷贝 写到不同的选项分支中
		if (bounds.dst_height==options->tileSize&&bounds.dst_width==options->tileSize)
		{
			//直接读取
			CPLErr err;
			{
				std::lock_guard lock(gdal_mutex);
				err = GDALDatasetRasterIO(
					dataset, GF_Read,
					bounds.clipped_src_min_x, bounds.clipped_src_min_y, bounds.read_width, bounds.read_height,
					output_buffer, bounds.dst_width, bounds.dst_height,
					data_type, band_count, nullptr,
					band_count * info.pixel_size, bounds.dst_width * band_count * info.pixel_size, info.pixel_size
				);
			}

			if (err != CE_None) {
				MemoryPool::GetInstance(this->getName())->deallocate(output_buffer, dataBufferSize);
				return false;
			}

			//检查是否为透明
			//读取后需要判断和nodata的值的关系
			::tbb::parallel_for(
				::tbb::blocked_range2d<int>(0, bounds.dst_height, 0, bounds.dst_width),
				[&](const ::tbb::blocked_range2d<int>& range) {
					for (int y = range.rows().begin(); y != range.rows().end(); ++y) {
						int dst_y = bounds.dst_min_y + y;
						for (int x = range.cols().begin(); x != range.cols().end(); ++x) {
							int dst_x = bounds.dst_min_x + x;
							size_t alpha_idx = (dst_y * options->tileSize + dst_x) ;//1个波段
							size_t dst_idx = alpha_idx * band_count * info.pixel_size;
							//并设置值多个波段和nodata一致性的位置为0
							if (options->outputFormat == "png" && checkNodata(output_buffer, dst_idx, info.pixel_size, band_count))
							{
								*(alphaBuffer + alpha_idx) = 0;
							}
						}
					}
				}
			);

		}
		else {
			//存在非数据范围区的 那么非数据范围区的透明度就为0 为了方便 直接将这个全部设置为0 再来各个转为255
			memset(alphaBuffer, 0, dataBufferSize/(image_info.output_band_count*image_info.pixel_size));
			::tbb::parallel_for(
				::tbb::blocked_range2d<int>(bounds.dst_min_y, bounds.dst_max_y+1, bounds.dst_min_x, bounds.dst_max_x+1),
				[&](const ::tbb::blocked_range2d<int>& range) {
					for (int y=range.rows().begin();y!=range.rows().end();++y)
					{
						for (int x=range.cols().begin();x!=range.cols().end();++x)
						{
							*(alphaBuffer + y * options->tileSize + x) = 255;
						}
					}
				}
			);

			// 分配临时缓冲区用于读取实际数据
			size_t temp_buffer_size = bounds.dst_width * bounds.dst_height * band_count * info.pixel_size;
			unsigned char* temp_buffer = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(temp_buffer_size));
			if (!temp_buffer) {
				return false;
			}
			// 初始化临时缓冲区
			memset(temp_buffer, 0, temp_buffer_size);
			
			CPLErr err;
			{
				std::lock_guard lock(gdal_mutex);
				err = GDALDatasetRasterIO(
					dataset, GF_Read,
					bounds.clipped_src_min_x, bounds.clipped_src_min_y, bounds.read_width, bounds.read_height,
					temp_buffer, bounds.dst_width, bounds.dst_height,
					data_type, band_count, nullptr,
					band_count * info.pixel_size, bounds.dst_width * band_count * info.pixel_size, info.pixel_size
				);
			}

			if (err != CE_None) {
				MemoryPool::GetInstance(this->getName())->deallocate(temp_buffer,temp_buffer_size);
				return false;
			}

			//读取后需要判断和nodata的值的关系
			::tbb::parallel_for(
				::tbb::blocked_range2d<int>(0, bounds.dst_height, 0, bounds.dst_width),
				[&](const ::tbb::blocked_range2d<int>& range) {
					for (int y = range.rows().begin(); y != range.rows().end(); ++y) {
						int dst_y = bounds.dst_min_y + y;
						for (int x = range.cols().begin(); x != range.cols().end(); ++x) {
							int dst_x = bounds.dst_min_x + x;
							// 计算源和目标位置的索引
							size_t src_idx = (y * bounds.dst_width + x) * band_count * info.pixel_size;
							size_t alpha_idx= (dst_y * options->tileSize + dst_x) ;//1个8位波段
							size_t dst_idx = alpha_idx * band_count * info.pixel_size;
							// 复制像素数据
							memcpy(output_buffer + dst_idx, temp_buffer + src_idx, band_count * info.pixel_size);
							//并设置值多个波段和nodata一致性的位置为0
							if (options->outputFormat == "png"&& checkNodata(output_buffer, dst_idx, info.pixel_size,band_count))
							{
								*(alphaBuffer + alpha_idx) = 0;
							}
						}
					}
				}
			);
			// 释放临时缓冲区
			MemoryPool::GetInstance(this->getName())->deallocate(temp_buffer, temp_buffer_size);
		}
		return true;
	}

	bool SlippyMapTiler::ensure_output_directory(const fs::path& x_dir)
	{
		std::lock_guard lock(fs_mutex);
		if (!fs::exists(x_dir)) {
			try {
				fs::create_directories(x_dir);
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "创建目录失败: " << e.what() << std::endl;
				return false;
			}
		}
		return true;
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

		// 获取影像信息
		image_info = get_image_info(dataset);

		//下面要处理NoDataValue 
		if (options->outputFormat == "png") {
			//先检查nodata的合法性 nodata一定要在datatype范围内
			if (options->nodata.size() > 0)
			{
				std::map<GDALDataType, std::function<bool(double)>> nodataRange = {
					{GDT_Byte,[](double bandNoData) {return (bandNoData >= std::numeric_limits<BYTE>::min()) && (bandNoData <= std::numeric_limits<BYTE>::max()); }},
					{GDT_Int8,[](double bandNoData) {return (bandNoData >= std::numeric_limits<int8_t>::min()) && (bandNoData <= std::numeric_limits<int8_t>::max()); }},
					{GDT_UInt16,[](double bandNoData) {return (bandNoData >= std::numeric_limits<uint16_t>::min()) && (bandNoData <= std::numeric_limits<uint16_t>::max()); }},
					{GDT_Int16,[](double bandNoData) {return (bandNoData >= std::numeric_limits<int16_t>::min()) && (bandNoData <= std::numeric_limits<int16_t>::max()); }},
					{GDT_UInt32,[](double bandNoData) {return (bandNoData >= std::numeric_limits<uint32_t>::min()) && (bandNoData <= std::numeric_limits<uint32_t>::max()); }},
					{GDT_Int32,[](double bandNoData) {return (bandNoData >= std::numeric_limits<int32_t>::min()) && (bandNoData <= std::numeric_limits<int32_t>::max()); }},
					{GDT_UInt64,[](double bandNoData) {return (bandNoData >= std::numeric_limits<uint64_t>::min()) && (bandNoData <= std::numeric_limits<uint64_t>::max()); }},
					{GDT_Int64,[](double bandNoData) {return (bandNoData >= std::numeric_limits<int64_t>::min()) && (bandNoData <= std::numeric_limits<int64_t>::max()); }},
					{GDT_Float32,[](double bandNoData) {return (bandNoData >= std::numeric_limits<float>::min()) && (bandNoData <= std::numeric_limits<float>::max()); }},
					{GDT_Float64,[](double bandNoData) {return (bandNoData >= std::numeric_limits<double>::min()) && (bandNoData <= std::numeric_limits<double>::max()); }}
				};
				for (size_t i = 0; i < image_info.output_band_count; i++)
				{					
					bool isNodataValide = nodataRange[data_type](options->nodata[i]);
					if (!isNodataValide) {
						std::cerr << "Nodata值设置应在影像位数的最小最大范围内！" << std::endl;
						return false;
					}
				}
			}

			//就算没有nodata值 如果是png 也应该给它设置值 因为要处理非影像范围内的数据
			std::map<GDALDataType, double> nodataMax = {
				{GDT_Byte,std::numeric_limits<BYTE>::max()},
				{GDT_Int8,std::numeric_limits<int8_t>::max()},
				{GDT_UInt16,std::numeric_limits<uint16_t>::max()},
				{GDT_Int16,std::numeric_limits<int16_t>::max()},
				{GDT_UInt32,std::numeric_limits<uint32_t>::max()},
				{GDT_Int32,std::numeric_limits<int32_t>::max()},
				{GDT_UInt64,std::numeric_limits<uint64_t>::max()},
				{GDT_Int64,std::numeric_limits<int64_t>::max()},
				{GDT_Float32,std::numeric_limits<float>::max()},
				{GDT_Float64,std::numeric_limits<double>::max()}
			};

			for (size_t i = 0; i < image_info.output_band_count; i++)
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
						options->nodata.resize(image_info.output_band_count);
					}
					options->nodata[i] = noDataValue;
				}
			}

			//现在思考这样的问题 既然我知道哪些地方是非数据区 那么我是不是直接就可以把该地方的值赋予任意一个值 同时开启半透明波段
			//然后对比nodata区域 也开启半透明波段 这样我就把png一开始就分类了 分为了有透明波段和待核实透明波段的（和之前的方法一样全局核实）
		}

		//获取图层像素值统计信息 以方便对像素进行缩放
		mins.swap(std::vector<double>()); maxs.swap(std::vector<double>());
		if (data_type != GDT_Byte) {
			for (int i = 1; i <= band_count; i++) {
				GDALRasterBandH pBand = GDALGetRasterBand(dataset,i);

				double minVal, maxVal;
				CPLErr err = GDALComputeRasterStatistics(
					pBand,FALSE,&minVal,&maxVal,NULL,NULL,NULL,NULL
				);

				if (err == CE_None) {
					mins.push_back(minVal);
					maxs.push_back(maxVal);
				}
			}
		}

		// 创建内存管理器和文件缓冲管理器
		fileBatchOutputer = std::make_shared<FileBatchOutput>();
		std::unique_ptr<ImageFileParallelIOAdapter> imageIOAdatper = std::make_unique<ImageFileParallelIOAdapter>(options->outputDir, true
			, options->tileSize, options->tileSize, options->outputFormat
			, image_info.output_band_count,options->nodata);
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


	//outData大小由外部分配
	bool SlippyMapTiler::scaleDataRange(unsigned char* pData, unsigned char* outData, std::vector<double>& statisticMax, std::vector<double>& statisticMin) {
		// 确保参数有效性
		if (!pData || !outData) {
			return false;
		}

		// 获取波段数，确保统计值和nodata向量长度匹配
		int bands = image_info.output_band_count;
		if (statisticMax.size() < bands || statisticMin.size() < bands ) {
			return false;
		}

		// 计算像素总数
		int pixelCount = options->tileSize * options->tileSize;

		// 根据不同的数据类型进行处理 缩放只针对非8位数据
		switch (data_type) {
		//case GDT_Byte: {
		//	// 对于Byte类型，直接拷贝数据，不需要缩放
		//	memcpy(outData, pData, pixelCount * bands);
		//	break;
		//}
		case GDT_UInt16: {
			scaleValue<uint16_t>(pData, outData, pixelCount, bands, statisticMin, statisticMax);
			//uint16_t* srcData = reinterpret_cast<uint16_t*>(pData);

			//// 对每个波段分别处理
			//for (int b = 0; b < bands; ++b) {
			//	double range = statisticMax[b] - statisticMin[b];
			//	double scale = (range > 0) ? 255.0 / range : 0.0;

			//	for (int i = 0; i < pixelCount; ++i) {
			//		uint16_t value = srcData[i + b * pixelCount];
			//		// 线性缩放到0-255范围
			//		double scaled = (value - statisticMin[b]) * scale;
			//		// 限制在0-255范围内
			//		outData[i + b * pixelCount] = static_cast<unsigned char>(
			//			std::max(0.0, std::min(255.0, scaled)));				
			//	}
			//}
			break;
		}
		case GDT_Int16: {
			scaleValue<int16_t>(pData, outData, pixelCount, bands, statisticMin, statisticMax);
			break;
		}
		case GDT_UInt32: {
			scaleValue<uint32_t>(pData, outData, pixelCount, bands, statisticMin, statisticMax);
			break;
		}
		case GDT_Int32: {
			scaleValue<int32_t>(pData, outData, pixelCount, bands, statisticMin, statisticMax);
			break;
		}
		case GDT_Float32: {
			scaleValue<float>(pData, outData, pixelCount, bands, statisticMin, statisticMax);
			break;
		}
		case GDT_Float64: {
			scaleValue<double>(pData, outData, pixelCount, bands, statisticMin, statisticMax);
			break;
		}
		default:
			// 对于未支持的数据类型，返回失败
			return false;
		}

		return true;
	}

	bool SlippyMapTiler::checkNodata(unsigned char* pData, size_t pos,int pixelSize,  int bandNum)
	{
		if (options->nodata.empty()) {
			return false;
		}
		// 计算当前像素的起始位置
		unsigned char* pixelStart = pData + pos ;

		// 遍历每个波段
		for (int band = 0; band < bandNum; ++band) {
			if (band >= options->nodata.size()) {
				break; // 如果nodata值数量少于波段数，则只检查已有的
			}

			bool isNoData = false;
			double noDataValue = options->nodata[band];
			unsigned char* bandData = pixelStart + band * GDALGetDataTypeSizeBytes(data_type);

			switch (data_type) {
			case GDT_Byte: {
				unsigned char value = *reinterpret_cast<const unsigned char*>(bandData);
				isNoData = isEqual<unsigned char>(value, static_cast<unsigned char>(noDataValue));
				break;
			}

			case GDT_UInt16: {
				unsigned short value = *reinterpret_cast<const unsigned short*>(bandData);
				isNoData = isEqual<unsigned short>(value, static_cast<unsigned short>(noDataValue));
				break;
			}

			case GDT_Int16: {
				short value = *reinterpret_cast<const short*>(bandData);
				isNoData = isEqual< short>(value, static_cast< short>(noDataValue));
				break;
			}

			case GDT_UInt32: {
				unsigned int value = *reinterpret_cast<const unsigned int*>(bandData);
				isNoData = isEqual< unsigned int>(value, static_cast<unsigned int>(noDataValue));
				break;
			}

			case GDT_Int32: {
				int value = *reinterpret_cast<const int*>(bandData);
				isNoData = isEqual< int>(value, static_cast<int>(noDataValue));
				break;
			}

			case GDT_Float32: {
				float value = *reinterpret_cast<const float*>(bandData);
				// 对于浮点数，需要特别处理NaN的情况
				if (std::isnan(value) && std::isnan(noDataValue)) {
					isNoData = true;
				}
				else {
					isNoData = isEqual< float>(value, static_cast<float>(noDataValue), 1e-6);
				}
				break;
			}

			case GDT_Float64: {
				double value = *reinterpret_cast<const double*>(bandData);
				// 对于浮点数，需要特别处理NaN的情况
				if (std::isnan(value) && std::isnan(noDataValue)) {
					isNoData = true;
				}
				else {
					isNoData = isEqual< double>(value, static_cast<double>(noDataValue), 1e-6);
				}
				break;
			}

			case GDT_CInt16: {
				// 复数类型：实部和虚部都需要检查
				short* complexValue = (short*)(bandData);
				double realPart = static_cast<double>(complexValue[0]);
				double imagPart = static_cast<double>(complexValue[1]);
				// 对于复数，通常只检查实部，或者可以检查模
				isNoData = isEqual<double>(realPart, noDataValue);
				break;
			}

			case GDT_CInt32: {
				int* complexValue = (int*)(bandData);
				double realPart = static_cast<double>(complexValue[0]);
				isNoData = isEqual<double>(realPart, noDataValue);
				break;
			}

			case GDT_CFloat32: {
				float* complexValue = (float*)(bandData);
				double realPart = static_cast<double>(complexValue[0]);
				if (std::isnan(complexValue[0]) && std::isnan(noDataValue)) {
					isNoData = true;
				}
				else {
					isNoData = isEqual<double>(realPart, noDataValue, 1e-6);
				}
				break;
			}

			case GDT_CFloat64: {
				double* complexValue = (double*)(bandData);
				double realPart = complexValue[0];
				if (std::isnan(realPart) && std::isnan(noDataValue)) {
					isNoData = true;
				}
				else {
					isNoData = isEqual<double>(realPart, noDataValue, 1e-6);
				}
				break;
			}

			default:
				// 未知数据类型，返回false
				return false;
			}

			// 如果任何一个波段不是NoData，则整个像素不是NoData
			if (!isNoData) {
				return false;
			}
		}

		// 所有波段都是NoData值
		return true;
	}

}