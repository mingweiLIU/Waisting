#include "SlippyTiler.h"
#include <cmath>
// TBB��
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
		// ��ʼ������任����
		for (int i = 0; i < 6; ++i) {
			geo_transform[i] = 0.0;
		}
				
		// ��������ϵͳ����
		coord_system = std::make_unique<CoordinateSystem>();
	}

	SlippyMapTiler::~SlippyMapTiler()
	{
		// ȷ���ͷ���Դ
		if (dataset) {
			GDALClose(dataset);
			dataset = nullptr;
		}
		GDALDestroyDriverManager();
		//����Ҫ����ڴ�ص�����
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
		// ������С��Ƭ����
		min_tile_x = long2tilex(min_x, zoom);
		min_tile_y = lat2tiley(min_y,zoom);

		// ���������Ƭ����
		max_tile_x = long2tilex(max_x, zoom);
		max_tile_y = lat2tiley(max_y, zoom);

		// ȷ��y����Ĵ�С��ϵ��ȷ������Ƭ����ϵ�У�yֵ���ϵ������ӣ�
		if (min_tile_y > max_tile_y) {
			std::swap(min_tile_y, max_tile_y);
		}
	}

	void SlippyMapTiler::get_tile_geo_bounds(int zoom, int tile_x, int tile_y, double& tile_min_x, double& tile_min_y, double& tile_max_x, double& tile_max_y)
	{
		// ��Ƭ��γ�ȷ�Χ
		tile_min_x = tilex2long(tile_x,zoom);
		tile_max_x = tilex2long(tile_x+1, zoom);

		tile_max_y = tiley2lat(tile_y,zoom);
		tile_min_y = tiley2lat(tile_y+1, zoom);
	}

	bool SlippyMapTiler::geo_to_pixel(const double& geo_x, const double& geo_y, int& pixel_x, int& pixel_y)
	{
		double x = geo_x;
		double y = geo_y;

		// �����Ҫ����ת������WGS84����ת��ΪӰ��ԭʼ����ϵ
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

		// ʹ�õ���任����������ת��Ϊ��������
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
		double latrad = lat * M_PI / 180.0;  // ת��Ϊ����
		return (int)(floor((1.0 - asinh(tan(latrad)) / M_PI) / 2.0 * (1 << z)));
	}

	double SlippyMapTiler::tiley2lat(int y, int z)
	{
		double n = M_PI - 2.0 * M_PI * y / (double)(1 << z);
		return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));  // ��˫�����м���
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
		const double EARTH_CIRCUMFERENCE = 40075016.686;  // �������ܳ����ף�

		// OSM �� zoom=0 ʱ�ķֱ��ʣ���/���أ�
		const double RESOLUTION_0 = EARTH_CIRCUMFERENCE / tileSize;

		// ������� zoom������ֵ������С����
		double zoom = std::log2(RESOLUTION_0 / groundResolution);

		// ȡ��������ȡ��������ֱ��ʹ��ߣ�
		int bestZoom = static_cast<int>(std::floor(zoom));

		return bestZoom;
	}

	// ����һ���̱߳��ص�GDAL���ݼ�
	GDALDatasetH SlippyMapTiler::create_local_dataset() {
		std::lock_guard<std::mutex> lock(gdal_mutex);
		return GDALOpen(options->inputFile.c_str(), GA_ReadOnly);
	}

	bool SlippyMapTiler::generate_tile(int zoom, int tile_x, int tile_y, GDALDatasetH local_dataset)
	{
		try {
			// ������Ƭ�ĵ���Χ����WGS84����ϵ�£�
			double tile_min_x, tile_min_y, tile_max_x, tile_max_y;
			get_tile_geo_bounds(zoom, tile_x, tile_y, tile_min_x, tile_min_y, tile_max_x, tile_max_y);

			// ������Ƭ·��
			fs::path x_dir = fs::path(options->outputDir) / std::to_string(zoom) / std::to_string(tile_x);

			// ������Ƭ��ԭʼӰ���е����ط�Χ
			int src_min_x, src_min_y, src_max_x, src_max_y;
			if (!geo_to_pixel(tile_min_x, tile_max_y, src_min_x, src_min_y) ||
				!geo_to_pixel(tile_max_x, tile_min_y, src_max_x, src_max_y)) {
				return false; // ����ת��ʧ��
			}

			// ������Ƭ�߽��ӳ���ϵ
			TileBounds bounds;
			if (!calculate_tile_bounds(src_min_x, src_min_y, src_max_x, src_max_y, bounds)) {
				return false; // ��Ƭ��ȫ��Ӱ��Χ�����Ч
			}

			// ȷ�����Ŀ¼����
			if (!ensure_output_directory(x_dir)) {
				return false;
			}

			// ����������Ƭ��С�Ļ����������������������
			size_t buffer_size = options->tileSize * options->tileSize * image_info.output_band_count * image_info.pixel_size;
			unsigned char* pData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(buffer_size));

			// ʹ��TBB���г�ʼ������������ ����Ҫע�� ��Ӧ��ʹ��ĳ����ɫ ��Ӧ��ʹ��nodata��ɫ �Է���û��ֵ�ĵط�͸��
			::tbb::parallel_for(
				::tbb::blocked_range<size_t>(0, buffer_size),
				[&](const ::tbb::blocked_range<size_t>& range) {
					if (options->outputFormat == "png") {
						for (size_t i = range.begin(); i < range.end(); ++i)
						{
							int nodataIndex = i % image_info.output_band_count;
							*(pData + i) = options->nodata[nodataIndex];
						}
					}
					else if (options->outputFormat == "jpg") {
						memset(pData + range.begin(), 0, range.size());
					}
				}
			);

			// ����Ӱ�����ʹ�������
			bool success;
			if (image_info.has_palette) {
				success = process_palette_image(local_dataset, bounds, image_info, pData);
			}
			else {
				success = process_regular_image(local_dataset, bounds, image_info, pData);
			}

			if (!success) {
				MemoryPool::GetInstance(this->getName())->deallocate(pData, buffer_size);
				return false;
			}

			//��������������
			unsigned char* scaledData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(image_info.output_band_count * options->tileSize * options->tileSize));
			scaleDataRange(pData, scaledData, options->nodata, maxs, mins);
			MemoryPool::GetInstance(this->getName())->deallocate(pData, buffer_size);

			// ���ڴ������ύ�������
			fs::path file = fs::path(std::to_string(zoom)) / std::to_string(tile_x) / std::to_string(tile_y);
			IOFileInfo* oneFileInfo = new IOFileInfo{ file.string(), scaledData,(size_t) image_info.output_band_count * options->tileSize * options->tileSize, this->getName() };
			fileBatchOutputer->addFile(oneFileInfo);

			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "��Ƭ�����쳣: " << e.what() << std::endl;
			return false;
		}
	}

	// ����һ���̰߳�ȫ�Ľ��ȸ��º���
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
		// �����Ƭ�Ƿ���ȫ��Ӱ��Χ��
		if (src_max_x < 0 || src_min_x >= img_width ||
			src_max_y < 0 || src_min_y >= img_height) {
			return false;
		}

		// ��ʼ��Ŀ����Ƭ�߽�
		bounds.dst_min_x = 0;
		bounds.dst_min_y = 0;
		bounds.dst_max_x = options->tileSize - 1;
		bounds.dst_max_y = options->tileSize - 1;

		// ����Ŀ����Ƭ����Ҫ������ݵ�����
		if (src_min_x < 0) {
			// ��߳���������Ŀ���������߽�
			double ratio = (double)(-src_min_x) / (src_max_x - src_min_x);
			bounds.dst_min_x = (int)(ratio * options->tileSize);
		}
		if (src_min_y < 0) {
			// �ϱ߳���������Ŀ��������ϱ߽�
			double ratio = (double)(-src_min_y) / (src_max_y - src_min_y);
			bounds.dst_min_y = (int)(ratio * options->tileSize);
		}
		if (src_max_x >= img_width) {
			// �ұ߳���������Ŀ��������ұ߽�
			double ratio = (double)(img_width - 1 - src_min_x) / (src_max_x - src_min_x);
			bounds.dst_max_x = (int)(ratio * options->tileSize);
		}
		if (src_max_y >= img_height) {
			// �±߳���������Ŀ��������±߽�
			double ratio = (double)(img_height - 1 - src_min_y) / (src_max_y - src_min_y);
			bounds.dst_max_y = (int)(ratio * options->tileSize);
		}

		// �ü�Դ���ݷ�Χ��Ӱ��߽���
		bounds.clipped_src_min_x = std::max(0, src_min_x);
		bounds.clipped_src_min_y = std::max(0, src_min_y);
		bounds.clipped_src_max_x = std::min(img_width - 1, src_max_x);
		bounds.clipped_src_max_y = std::min(img_height - 1, src_max_y);

		// ����ʵ����Ҫ��ȡ��Ŀ������ݴ�С
		bounds.read_width = bounds.clipped_src_max_x - bounds.clipped_src_min_x + 1;
		bounds.read_height = bounds.clipped_src_max_y - bounds.clipped_src_min_y + 1;
		bounds.dst_width = bounds.dst_max_x - bounds.dst_min_x + 1;
		bounds.dst_height = bounds.dst_max_y - bounds.dst_min_y + 1;

		return bounds.read_width > 0 && bounds.read_height > 0;
	}

	ImageInfo SlippyMapTiler::get_image_info(GDALDatasetH dataset)
	{
		ImageInfo info;

		// ����Ƿ�Ϊ����ɫ��ĵ�����Ӱ��
		GDALRasterBandH first_band = GDALGetRasterBand(dataset, 1);
		info.color_table = GDALGetRasterColorTable(first_band);
		info.has_palette = (band_count == 1 && info.color_table != nullptr);

		//����ʵ�ʵĲ�����
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
					// ����δ֪��ʽ������ͨ������ɫ����Ŀ��alphaֵ���ж�
					// ���������Ŀ��alpha����255����ΪRGB������ΪRGBA
					int entry_count = GDALGetColorEntryCount(info.color_table);
					bool has_alpha = false;
					for (int i = 0; i < entry_count; i++) {
						const GDALColorEntry* entry = GDALGetColorEntry(info.color_table, i);
						if (entry && entry->c4 != 255) {  // c4��alphaͨ��
							has_alpha = true;
							break;
						}
					}
					info.output_band_count = has_alpha ? 4 : 3;  // RGBA��RGB
					break;
				}
			}
		}
		else {
			info.output_band_count = band_count;
		}

		// �������ش�С
		info.pixel_size = GDALGetDataTypeSizeBytes(data_type);// / 8;

		//û�취 ��Ҫ�Լ����ֶ�����
		//if (options->outputFormat == "jpg") {
		//	info.pixel_size = 1;
		//}
		//else if (options->outputFormat == "png") {
		//	info.pixel_size = info.pixel_size > 2 ? 1 : info.pixel_size;
		//}

		return info;
	}

	bool SlippyMapTiler::process_palette_image(GDALDatasetH dataset, const TileBounds& bounds, const ImageInfo& info, unsigned char* output_buffer)
	{
		// ������ʱ���������ڶ�ȡԭʼ��������
		size_t temp_buffer_size = bounds.dst_width * bounds.dst_height * info.pixel_size;
		unsigned char* temp_buffer = (unsigned char*)malloc(temp_buffer_size);
		if (!temp_buffer) {
			return false;
		}

		// ��ȡ��������
		CPLErr err;
		{
			std::lock_guard lock(gdal_mutex);
			err = GDALDatasetRasterIO(
				dataset, GF_Read,
				bounds.clipped_src_min_x, bounds.clipped_src_min_y, bounds.read_width, bounds.read_height,
				temp_buffer, bounds.dst_width, bounds.dst_height,
				data_type, 1, nullptr,  // ֻ��ȡһ������
				info.pixel_size, bounds.dst_width * info.pixel_size, 1
			);
		}

		if (err != CE_None) {
			free(temp_buffer);
			return false;
		}

		// ��ȡ��ɫ����Ŀ��
		int palette_count = GDALGetColorEntryCount(info.color_table);

		// ʹ��TBB���д�������ת��
		::tbb::parallel_for(
			::tbb::blocked_range2d<int>(0, bounds.dst_height, 0, bounds.dst_width),
			[&](const ::tbb::blocked_range2d<int>& range) {
				for (int y = range.rows().begin(); y != range.rows().end(); ++y) {
					int dst_y = bounds.dst_min_y + y;
					for (int x = range.cols().begin(); x != range.cols().end(); ++x) {
						int dst_x = bounds.dst_min_x + x;

						// ��ȡ����ֵ
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

						// ��ȡ��Ӧ��RGBֵ
						GDALColorEntry color_entry;
						if (index >= 0 && index < palette_count) {
							GDALGetColorEntryAsRGB(info.color_table, index, &color_entry);
						}
						else {
							// ����������Χ ��ô��ֱ������Ϊnodata��ֵ
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

						// ����Ŀ��λ�õ�����
						size_t dst_idx = (dst_y * options->tileSize + dst_x) * 3 * info.pixel_size;

						// д��RGBֵ
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

		// �ͷ���ʱ������
		free(temp_buffer);
		return true;
	}

	bool SlippyMapTiler::process_regular_image(GDALDatasetH dataset, const TileBounds& bounds, const ImageInfo& info, unsigned char* output_buffer)
	{
		// ������ʱ���������ڶ�ȡʵ������
		size_t temp_buffer_size = bounds.dst_width * bounds.dst_height * band_count * info.pixel_size;
		unsigned char* temp_buffer = (unsigned char*)malloc(temp_buffer_size);
		if (!temp_buffer) {
			return false;
		} 

		// ��ʼ����ʱ������Ϊnodataֵ
		if (!options->nodata.empty()) {
			::tbb::parallel_for(
				::tbb::blocked_range<size_t>(0, bounds.dst_width * bounds.dst_height),
				[&](const ::tbb::blocked_range<size_t>& range) {
					for (size_t pixel_idx = range.begin(); pixel_idx != range.end(); ++pixel_idx) {
						for (int band = 0; band < band_count; ++band) {
							size_t byte_idx = pixel_idx * band_count * info.pixel_size + band * info.pixel_size;

							// ����������������nodataֵ
							double nodata_val = (band < options->nodata.size()) ? options->nodata[band] : options->nodata[0];

							switch (data_type) {
							case GDT_Byte:
								temp_buffer[byte_idx] = static_cast<unsigned char>(nodata_val);
								break;
							case GDT_UInt16:
								*reinterpret_cast<uint16_t*>(temp_buffer + byte_idx) = static_cast<uint16_t>(nodata_val);
								break;
							case GDT_Int16:
								*reinterpret_cast<int16_t*>(temp_buffer + byte_idx) = static_cast<int16_t>(nodata_val);
								break;
							case GDT_UInt32:
								*reinterpret_cast<uint32_t*>(temp_buffer + byte_idx) = static_cast<uint32_t>(nodata_val);
								break;
							case GDT_Int32:
								*reinterpret_cast<int32_t*>(temp_buffer + byte_idx) = static_cast<int32_t>(nodata_val);
								break;
							case GDT_Float32:
								*reinterpret_cast<float*>(temp_buffer + byte_idx) = static_cast<float>(nodata_val);
								break;
							case GDT_Float64:
								*reinterpret_cast<double*>(temp_buffer + byte_idx) = nodata_val;
								break;
							default:
								// ����δ֪���ͣ���0���
								memset(temp_buffer + byte_idx, 0, info.pixel_size);
								break;
							}
						}
					}
				}
			);
		}
		else {
			// ���û��nodataֵ����0��ʼ��
			memset(temp_buffer, 0, temp_buffer_size);
		}

		// ֻ�е���ȡ������Ӱ�������н���ʱ�Ž��ж�ȡ
		if (bounds.read_width > 0 && bounds.read_height > 0) {
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
				free(temp_buffer);
				return false;
			}
		}

		// ʹ��TBB���и������ݵ����������
		::tbb::parallel_for(
			::tbb::blocked_range2d<int>(0, bounds.dst_height, 0, bounds.dst_width),
			[&](const ::tbb::blocked_range2d<int>& range) {
				for (int y = range.rows().begin(); y != range.rows().end(); ++y) {
					int dst_y = bounds.dst_min_y + y;
					for (int x = range.cols().begin(); x != range.cols().end(); ++x) {
						int dst_x = bounds.dst_min_x + x;

						// ����Դ��Ŀ��λ�õ�����
						size_t src_idx = (y * bounds.dst_width + x) * band_count * info.pixel_size;
						size_t dst_idx = (dst_y * options->tileSize + dst_x) * band_count * info.pixel_size;

						// ������������
						memcpy(output_buffer + dst_idx, temp_buffer + src_idx, band_count * info.pixel_size);
					}
				}
			}
		);

		// �ͷ���ʱ������
		free(temp_buffer);
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
				std::cerr << "����Ŀ¼ʧ��: " << e.what() << std::endl;
				return false;
			}
		}
		return true;
	}

	void SlippyMapTiler::process_zoom_level(int zoom, std::shared_ptr<IProgressInfo> progressInfo)
	{
		std::cout << "\n�������ż���: " << zoom << std::endl;

		// ����ü������Ƭ��Χ
		int min_tile_x, min_tile_y, max_tile_x, max_tile_y;
		get_tile_range(zoom, min_tile_x, min_tile_y, max_tile_x, max_tile_y);

		// ȷ�����Ŀ¼����
		create_directories(zoom);

		// �����ܹ���Ҫ�������Ƭ����
		int tiles_wide = max_tile_x - min_tile_x + 1;
		int tiles_high = max_tile_y - min_tile_y + 1;
		int total_tiles = tiles_wide * tiles_high;

		std::cout << "��Ƭ��Χ: X=" << min_tile_x << "-" << max_tile_x
			<< ", Y=" << min_tile_y << "-" << max_tile_y
			<< " (�ܼ� " << total_tiles << " ����Ƭ)" << std::endl;

		// ���ü�����
		//total_tiles_processed = 0;

		// ʹ���̱߳��ش洢����Dataset
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

		// ʹ��TBB���д���
		::tbb::parallel_for(
			::tbb::blocked_range2d<int, int>(min_tile_y, max_tile_y + 1, min_tile_x, max_tile_x + 1),
			[this, zoom, total_tiles, &progressInfo, &tls_data](const ::tbb::blocked_range2d<int, int>& r) {
				// ��ȡ�̱߳��ش洢
				ThreadLocalData& local_data = tls_data.local();

				// ����ʼ���������ݼ�
				if (!local_data.local_dataset) {
					local_data.local_dataset = this->create_local_dataset();
					if (!local_data.local_dataset) {
						std::cerr << "�޷������̱߳������ݼ�" << std::endl;
						return;
					}
				}

				for (int tile_y = r.rows().begin(); tile_y != r.rows().end(); ++tile_y) {
					for (int tile_x = r.cols().begin(); tile_x != r.cols().end(); ++tile_x) {
						// ʹ���̱߳������ݼ�������Ƭ
						if (generate_tile(zoom, tile_x, tile_y, local_data.local_dataset)) {
							// ���½���
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
		// ע��GDAL����
		GDALAllRegister();

		// �����߳�����
		if (options->numThreads > 0) {
			std::cout << "�����߳�����: " << options->numThreads << std::endl;
			::tbb::global_control global_limit(::tbb::global_control::max_allowed_parallelism, options->numThreads);
		}

		// �����ݼ�
		GDALOpenInfo* poOpenInfo = new GDALOpenInfo(options->inputFile.c_str(), GA_ReadOnly);

		// ����Ƿ���GDAL֧�ֵĸ�ʽ
		if (!GDALIdentifyDriver(poOpenInfo->pszFilename, nullptr)) {
			std::cerr << "�޷�ʶ�������ļ���ʽ: " << options->inputFile << std::endl;
			delete poOpenInfo;
			return false;
		}

		dataset = GDALOpen(options->inputFile.c_str(), GA_ReadOnly);		

		if (!dataset) {
			std::cerr << "�޷��������ļ�: " << options->inputFile << std::endl;
			return false;
		}


		// ��ȡ����任����
		if (GDALGetGeoTransform(dataset, geo_transform) != CE_None) {
			std::cerr << "�޷���ȡ����任����" << std::endl;
			return false;
		}

		// ��ȡӰ��ߴ�
		img_width = GDALGetRasterXSize(dataset);
		img_height = GDALGetRasterYSize(dataset);

		if (img_width <= 0 || img_height <= 0) {
			std::cerr << "��Ч��Ӱ��ߴ�: " << img_width << "x" << img_height << std::endl;
			return false;
		}

		// ��ȡ������������������
		band_count = GDALGetRasterCount(dataset);
		if (band_count <= 0) {
			std::cerr << "��Ч�Ĳ�������: " << band_count << std::endl;
			return false;
		}

		// ��ȡ�������ͣ�ʹ�õ�һ�����ε����ͣ�
		GDALRasterBandH band = GDALGetRasterBand(dataset, 1);
		if (!band) {
			std::cerr << "�޷���ȡ������Ϣ" << std::endl;
			return false;
		}
		data_type = GDALGetRasterDataType(band);

		// ���Ӱ����Ϣ
		std::cout << "Ӱ��ߴ�: " << img_width << "x" << img_height
			<< ", ������: " << band_count
			<< ", ��������: " << GDALGetDataTypeName(data_type) << std::endl;

		// ��ʼ������ϵͳ
		if (!coord_system->initialize(dataset, options->prjFilePath, options->wktString)) {
			std::cerr << "��ʼ������ϵͳʧ��" << std::endl;
			return false;
		}

		// ����Ӱ��ĵ���Χ
		double xBounds[4] = { 0,img_width,img_width,0 };
		double yBounds[4] = { 0,0,img_height,img_height };

		// Ӧ�õ���任
		for (int i = 0; i < 4; ++i) {
			double x = xBounds[i];
			double y = yBounds[i];

			// ��������תΪ��������
			double geo_x = geo_transform[0] + x * geo_transform[1] + y * geo_transform[2];
			double geo_y = geo_transform[3] + x * geo_transform[4] + y * geo_transform[5];

			xBounds[i] = geo_x;
			yBounds[i] = geo_y;
		}

		// ת����WGS84����ϵ
		coord_system->transform_points(xBounds, yBounds, 4);

		// �ҳ�����Χ
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

		// ȷ����Χ����Ч�ľ�γ�ȷ�Χ��
		min_x = std::max(min_x, MIN_LONGITUDE);
		max_x = std::min(max_x, MAX_LONGITUDE);
		min_y = std::max(min_y, MIN_LATITUDE);
		max_y = std::min(max_y, MAX_LATITUDE);

		std::cout << "����Χ: "
			<< "����=" << min_x << "��" << max_x
			<< ", γ��=" << min_y << "��" << max_y << std::endl;

		//����Ҫ��Ƭ�Ĳ㼶����
		if (options->minLevel < 0) options->minLevel = 0;
		//��ȡӰ��ֱ�����Ѳ㼶
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
			//����ת��Ϊwgs84��Ĵ���
			coord_system->calculateGeographicResolution((min_y + max_y) / 2.0, xOriginResolution, yOriginResolution, xResolutionM = 0, yResolutionM);
			options->maxLevel = this->getProperLevel(std::min(xResolutionM, yResolutionM), options->tileSize);
		}

		// ��ȡӰ����Ϣ
		image_info = get_image_info(dataset);

		//����Ҫ����NoDataValue ����û��nodataֵ �����png ����ҲӦ�ø�������ֵ ��ΪҪ�����Ӱ��Χ�ڵ�����
		//�ȼ��nodata�ĺϷ��� nodataһ��Ҫ��datatype��Χ��
		for (size_t i = 0; i < image_info.output_band_count; i++)
		{
			std::map<GDALDataType, std::function<bool(double)>> nodataRange = {
					{GDT_Byte,std::numeric_limits<BYTE>::min()nodata std::numeric_limits<BYTE>::max()},
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
		}


		if (options->outputFormat == "png") {
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
				//����о�ʹ��ԭ�ļ��� û�о�ʹ�����õ�
				if (hasNoData) {
					if (options->nodata.size() == 0)
					{
						options->nodata.resize(image_info.output_band_count);
					}
					options->nodata[i] = noDataValue;
				}
			}

			//�����Ȼû��nodata ����Ҫ����һ��
			if (options->nodata.size() == 0)
			{
				for (size_t i = 0; i < image_info.output_band_count; i++)
				{
					options->nodata.push_back(nodataMax[data_type]);
				}
			}
		}

		//��ȡͼ������ֵͳ����Ϣ �Է�������ؽ�������
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

		// �����ڴ���������ļ����������
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
			std::cerr << "���ݼ�δ��ʼ�������ȵ���initialize()" << std::endl;
			return false;
		}

		// ���������Ŀ¼
		if (!fs::exists(options->outputDir)) {
			if (!fs::create_directories(options->outputDir)) {
				std::cerr << "�޷��������Ŀ¼: " << options->outputDir << std::endl;
				return false;
			}
		}

		// �������е���Ƭ�� ����������Ĵ���
		int total_tiles = 0;
		for (int zoom = options->minLevel; zoom <= options->maxLevel; zoom++) {
			// ����ü������Ƭ��Χ
			int min_tile_x, min_tile_y, max_tile_x, max_tile_y;
			get_tile_range(zoom, min_tile_x, min_tile_y, max_tile_x, max_tile_y);

			// ��������Ƭ����
			total_tiles += (max_tile_x - min_tile_x + 1) * (max_tile_y - min_tile_y + 1);
		}
		progressInfo->setTotalNum(total_tiles);

		// ��¼��ʼʱ��
		start_time = std::chrono::high_resolution_clock::now();

		// ����ÿ�����ż���
		for (int zoom = options->minLevel; zoom <= options->maxLevel; ++zoom) {
			process_zoom_level(zoom, progressInfo);
		}

		//���д�����Ϻ� ��Ҫ���
		fileBatchOutputer->output();

		//���Ԫ����
		nlohmann::json metaInfo;
		metaInfo["extent"] = { min_x, min_y, max_x, max_y };
		metaInfo["center"] = { (min_x + max_x) / 2,(min_y + max_y) / 2 };
		metaInfo["levels"] = { options->minLevel,options->maxLevel };
		std::ofstream fStream((fs::path(options->outputDir) / "meta.json").string().c_str());
		fStream << std::setw(4) << metaInfo << std::endl;
		fStream.close();

		// �ȴ������ļ�д�����
		std::cout << "�ȴ��ļ�д�����..." << std::endl;

		// �����ܴ���ʱ��
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

		// ���ͳ����Ϣ
		std::cout << "\n��Ƭ���!" << std::endl;
		std::cout << "�ܴ���ʱ��: " << duration << " ��" << std::endl;

		return true;
	}


	//outData��С���ⲿ����
	bool SlippyMapTiler::scaleDataRange(unsigned char* pData, unsigned char* outData, std::vector<double>& nodata, std::vector<double>& statisticMax, std::vector<double>& statisticMin) {
		// ȷ��������Ч��
		if (!pData || !outData) {
			return false;
		}

		// ��ȡ��������ȷ��ͳ��ֵ��nodata��������ƥ��
		int bands = band_count;
		if (statisticMax.size() < bands || statisticMin.size() < bands || nodata.size() < bands) {
			return false;
		}

		// ������������
		int pixelCount = options->tileSize * options->tileSize;

		// ���ݲ�ͬ���������ͽ��д���
		switch (data_type) {
		case GDT_Byte: {
			// ����Byte���ͣ�ֱ�ӿ������ݣ�����Ҫ����
			memcpy(outData, pData, pixelCount * bands);
			break;
		}
		case GDT_UInt16: {
			uint16_t* srcData = reinterpret_cast<uint16_t*>(pData);

			// ��ÿ�����ηֱ���
			for (int b = 0; b < bands; ++b) {
				double range = statisticMax[b] - statisticMin[b];
				double scale = (range > 0) ? 255.0 / range : 0.0;

				for (int i = 0; i < pixelCount; ++i) {
					uint16_t value = srcData[i + b * pixelCount];

					// ����nodataֵ
					if (std::abs(value - nodata[b]) < 1e-6) {
						outData[i + b * pixelCount] = 0; // ������Ϊ����͸��ֵ
					}
					else {
						// �������ŵ�0-255��Χ
						double scaled = (value - statisticMin[b]) * scale;
						// ������0-255��Χ��
						outData[i + b * pixelCount] = static_cast<unsigned char>(
							std::max(0.0, std::min(255.0, scaled)));
					}
				}
			}
			break;
		}
		case GDT_Int16: {
			int16_t* srcData = reinterpret_cast<int16_t*>(pData);

			for (int b = 0; b < bands; ++b) {
				double range = statisticMax[b] - statisticMin[b];
				double scale = (range > 0) ? 255.0 / range : 0.0;

				for (int i = 0; i < pixelCount; ++i) {
					int16_t value = srcData[i + b * pixelCount];

					// ����nodataֵ
					if (std::abs(value - nodata[b]) < 1e-6) {
						outData[i + b * pixelCount] = 0;
					}
					else {
						double scaled = (value - statisticMin[b]) * scale;
						outData[i + b * pixelCount] = static_cast<unsigned char>(
							std::max(0.0, std::min(255.0, scaled)));
					}
				}
			}
			break;
		}
		case GDT_UInt32: {
			uint32_t* srcData = reinterpret_cast<uint32_t*>(pData);

			for (int b = 0; b < bands; ++b) {
				double range = statisticMax[b] - statisticMin[b];
				double scale = (range > 0) ? 255.0 / range : 0.0;

				for (int i = 0; i < pixelCount; ++i) {
					uint32_t value = srcData[i + b * pixelCount];

					// ����nodataֵ
					if (std::abs(value - nodata[b]) < 1e-6) {
						outData[i + b * pixelCount] = 0;
					}
					else {
						double scaled = (value - statisticMin[b]) * scale;
						outData[i + b * pixelCount] = static_cast<unsigned char>(
							std::max(0.0, std::min(255.0, scaled)));
					}
				}
			}
			break;
		}
		case GDT_Int32: {
			int32_t* srcData = reinterpret_cast<int32_t*>(pData);

			for (int b = 0; b < bands; ++b) {
				double range = statisticMax[b] - statisticMin[b];
				double scale = (range > 0) ? 255.0 / range : 0.0;

				for (int i = 0; i < pixelCount; ++i) {
					int32_t value = srcData[i + b * pixelCount];

					// ����nodataֵ
					if (std::abs(value - nodata[b]) < 1e-6) {
						outData[i + b * pixelCount] = 0;
					}
					else {
						double scaled = (value - statisticMin[b]) * scale;
						outData[i + b * pixelCount] = static_cast<unsigned char>(
							std::max(0.0, std::min(255.0, scaled)));
					}
				}
			}
			break;
		}
		case GDT_Float32: {
			float* srcData = reinterpret_cast<float*>(pData);

			for (int b = 0; b < bands; ++b) {
				double range = statisticMax[b] - statisticMin[b];
				double scale = (range > 0) ? 255.0 / range : 0.0;

				for (int i = 0; i < pixelCount; ++i) {
					float value = srcData[i + b * pixelCount];

					// ����NaN��nodataֵ
					if (std::isnan(value) || std::abs(value - nodata[b]) < 1e-6) {
						outData[i + b * pixelCount] = 0;
					}
					else {
						double scaled = (value - statisticMin[b]) * scale;
						outData[i + b * pixelCount] = static_cast<unsigned char>(
							std::max(0.0, std::min(255.0, scaled)));
					}
				}
			}
			break;
		}
		case GDT_Float64: {
			double* srcData = reinterpret_cast<double*>(pData);

			for (int b = 0; b < bands; ++b) {
				double range = statisticMax[b] - statisticMin[b];
				double scale = (range > 0) ? 255.0 / range : 0.0;

				for (int i = 0; i < pixelCount; ++i) {
					double value = srcData[i + b * pixelCount];

					// ����NaN��nodataֵ
					if (std::isnan(value) || std::abs(value - nodata[b]) < 1e-6) {
						outData[i + b * pixelCount] = 0;
					}
					else {
						double scaled = (value - statisticMin[b]) * scale;
						outData[i + b * pixelCount] = static_cast<unsigned char>(
							std::max(0.0, std::min(255.0, scaled)));
					}
				}
			}
			break;
		}
		default:
			// ����δ֧�ֵ��������ͣ�����ʧ��
			return false;
		}

		return true;
	}
}