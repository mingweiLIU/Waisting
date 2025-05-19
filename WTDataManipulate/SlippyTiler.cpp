#include "SlippyTiler.h"
#include <cmath>
// TBB��
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
			std::string tile_path = (x_dir / (std::to_string(tile_y) + "." + options->outputFormat)).string();

			// ������Ƭ��ԭʼӰ���е����ط�Χ
			int src_min_x, src_min_y, src_max_x, src_max_y;
			if (!geo_to_pixel(tile_min_x, tile_max_y, src_min_x, src_min_y) ||
				!geo_to_pixel(tile_max_x, tile_min_y, src_max_x, src_max_y)) {
				return false; // ����ת��ʧ��
			}

			// �����Ƭ�Ƿ���ȫ��Ӱ��Χ��
			if (src_max_x < 0 || src_min_x >= img_width ||
				src_max_y < 0 || src_min_y >= img_height) {
				return false; // ��Ƭ��ȫ��Ӱ��Χ��
			}

			// ����Ŀ����Ƭ����Ҫ������ݵ�����
			int dst_min_x = 0, dst_min_y = 0;
			int dst_max_x = options->tileSize - 1, dst_max_y = options->tileSize - 1;

			// ���Դ���ݷ�Χ����Ӱ��߽磬����Ŀ�������ƫ��
			if (src_min_x < 0) {
				// ��߳���������Ŀ���������߽�
				double ratio = (double)(-src_min_x) / (src_max_x - src_min_x);
				dst_min_x = (int)(ratio * options->tileSize);
			}
			if (src_min_y < 0) {
				// �ϱ߳���������Ŀ��������ϱ߽�
				double ratio = (double)(-src_min_y) / (src_max_y - src_min_y);
				dst_min_y = (int)(ratio * options->tileSize);
			}
			if (src_max_x >= img_width) {
				// �ұ߳���������Ŀ��������ұ߽�
				double ratio = (double)(img_width - 1 - src_min_x) / (src_max_x - src_min_x);
				dst_max_x = (int)(ratio * options->tileSize);
			}
			if (src_max_y >= img_height) {
				// �±߳���������Ŀ��������±߽�
				double ratio = (double)(img_height - 1 - src_min_y) / (src_max_y - src_min_y);
				dst_max_y = (int)(ratio * options->tileSize);
			}

			// �ü�Դ���ݷ�Χ��Ӱ��߽���
			int clipped_src_min_x = std::max(0, src_min_x);
			int clipped_src_min_y = std::max(0, src_min_y);
			int clipped_src_max_x = std::min(img_width - 1, src_max_x);
			int clipped_src_max_y = std::min(img_height - 1, src_max_y);

			// ����ʵ����Ҫ��ȡ�����ݴ�С
			int read_width = clipped_src_max_x - clipped_src_min_x + 1;
			int read_height = clipped_src_max_y - clipped_src_min_y + 1;

			if (read_width <= 0 || read_height <= 0) {
				return false;
			}

			// ȷ��xĿ¼����
			{
				std::lock_guard lock(fs_mutex);
				if (!fs::exists(x_dir)) {
					fs::create_directories(x_dir);
				}
			}

			// �������ش�С
			size_t pixel_size = GDALGetDataTypeSize(data_type) / 8;
			if (options->outputFormat == "jpg") {
				pixel_size = 1;
			}
			else if (options->outputFormat == "png") {
				pixel_size = pixel_size > 2 ? 1 : pixel_size;
			}

			// ����������Ƭ��С�Ļ�����
			size_t buffer_size = options->tileSize * options->tileSize * band_count * pixel_size;
			unsigned char* pData = (unsigned char*)(MemoryPool::GetInstance(this->getName())->allocate(buffer_size));

			// ��ʼ������������Ϊ0������������ֵ��
			memset(pData, 0, buffer_size);

			// ����Ŀ�껺�����е���Ч���������С
			int dst_width = dst_max_x - dst_min_x + 1;
			int dst_height = dst_max_y - dst_min_y + 1;

			// ������ʱ���������ڶ�ȡʵ������
			size_t temp_buffer_size = dst_width * dst_height * band_count * pixel_size;
			unsigned char* temp_buffer = (unsigned char*)malloc(temp_buffer_size);
			if (!temp_buffer) {
				MemoryPool::GetInstance(this->getName())->deallocate(pData, buffer_size);
				return false;
			}

			// ��ȡ���ݵ���ʱ������
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

			// ����ʱ�����������ݸ��Ƶ�Ŀ�껺��������ȷλ��
			for (int y = 0; y < dst_height; y++) {
				int dst_y = dst_min_y + y;
				for (int x = 0; x < dst_width; x++) {
					int dst_x = dst_min_x + x;

					// ����Դ��Ŀ��λ�õ�����
					size_t src_idx = (y * dst_width + x) * band_count * pixel_size;
					size_t dst_idx = (dst_y * options->tileSize + dst_x) * band_count * pixel_size;

					// ������������
					memcpy(pData + dst_idx, temp_buffer + src_idx, band_count * pixel_size);
				}
			}

			// �ͷ���ʱ������
			free(temp_buffer);

			// ���ڴ������ƶ���vector��
			fs::path file = fs::path(std::to_string(zoom)) / std::to_string(tile_x) / std::to_string(tile_y);
			IOFileInfo* oneFileInfo = new IOFileInfo{ file.string(), pData, buffer_size, this->getName() };
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

		//����Ҫ����NoDataValue
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
				//����о�ʹ��ԭ�ļ��� û�о�ʹ�����õ�
				if (hasNoData) {
					if (options->nodata.size() == 0)
					{
						options->nodata.resize(band_count);
					}
					options->nodata[i] = noDataValue;
				}
			}
		}

		// �����ڴ���������ļ����������
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
}