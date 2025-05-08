#include "SlippyTiler.h"


namespace WT{
	SlippyMapTiler::SlippyMapTiler(std::shared_ptr<SlippyMapTilerOptions> options) {
		this->options = options;
		// ��ʼ��GDAL
		GDALAllRegister();

		// �����ڴ��
		memory_allocator = std::make_shared<JemallocAllocator>(1024 * 1024 * 10); // 10MB ���С

		// �����ļ����������
		file_buffer = std::make_shared<FileBufferManager>(5000); // ����5000���ļ�����

		// ���û��ָ���߳�����ʹ��ϵͳ�߼�CPU������һ��
		if (this->options->numThreads <= 0) {
			this->options->numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency() / 2));
		}
	}

	SlippyMapTiler::~SlippyMapTiler()
	{
		// ȷ���ͷ���Դ
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
		// ������С��Ƭ����
		min_tile_x = static_cast<int>(floor((min_x + 180.0) / 360.0 * (1 << zoom)));
		min_tile_y = static_cast<int>(floor((1.0 - log(tan(min_y * M_PI / 180.0) + 1.0 / cos(min_y * M_PI / 180.0)) / M_PI) / 2.0 * (1 << zoom)));

		// ���������Ƭ����
		max_tile_x = static_cast<int>(floor((max_x + 180.0) / 360.0 * (1 << zoom)));
		max_tile_y = static_cast<int>(floor((1.0 - log(tan(max_y * M_PI / 180.0) + 1.0 / cos(max_y * M_PI / 180.0)) / M_PI) / 2.0 * (1 << zoom)));

		// ȷ��y����Ĵ�С��ϵ��ȷ������Ƭ����ϵ�У�yֵ���ϵ������ӣ�
		if (min_tile_y > max_tile_y) {
			std::swap(min_tile_y, max_tile_y);
		}
	}

	void SlippyMapTiler::get_tile_geo_bounds(int zoom, int tile_x, int tile_y, double& tile_min_x, double& tile_min_y, double& tile_max_x, double& tile_max_y)
	{
		// ��Ƭ��γ�ȷ�Χ
		tile_min_x = tile_x * 360.0 / (1 << zoom) - 180.0;
		tile_max_x = (tile_x + 1) * 360.0 / (1 << zoom) - 180.0;

		tile_max_y = atan(sinh(M_PI * (1 - 2 * tile_y / (1 << zoom)))) * 180.0 / M_PI;
		tile_min_y = atan(sinh(M_PI * (1 - 2 * (tile_y + 1) / (1 << zoom)))) * 180.0 / M_PI;
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

			if (!OCTTransform(inv_transform, 1, &x, &y, nullptr)) {
				OCTDestroyCoordinateTransformation(inv_transform);
				return false;
			}

			OCTDestroyCoordinateTransformation(inv_transform);
		}

		// ʹ�õ���任����������ת��Ϊ��������
		double det = geo_transform[1] * geo_transform[5] - geo_transform[2] * geo_transform[4];
		if (fabs(det) < 1e-10) {
			return false; // �޷�ת��
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

			// �߽���
			if (src_max_x < 0 || src_min_x >= img_width || src_max_y < 0 || src_min_y >= img_height) {
				return false; // ��Ƭ��Ӱ��Χ��
			}

			// �ü���Χ����
			src_min_x = std::max(0, src_min_x);
			src_min_y = std::max(0, src_min_y);
			src_max_x = std::min(img_width - 1, src_max_x);
			src_max_y = std::min(img_height - 1, src_max_y);

			// �����ȡ�����ݴ�С
			int width = src_max_x - src_min_x + 1;
			int height = src_max_y - src_min_y + 1;

			if (width <= 0 || height <= 0) {
				return false;
			}

			// ��ȡ��Ч��������
			int bands = std::min(band_count, 4); // ���֧��4������(RGBA)

			// �����ڴ�Ϊÿ�����δ���������
			size_t pixel_size = GDALGetDataTypeSize(data_type) / 8;
			size_t buffer_size = width * height * bands * pixel_size;

			// ʹ��jemalloc�����ڴ�
			void* pData = memory_allocator->allocate(buffer_size);
			if (!pData) {
				std::cerr << "�ڴ����ʧ��!" << std::endl;
				return false;
			}

			// ��ȡ����
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

			// ����Ŀ����Ƭ���ݼ�
			GDALDriverH memDriver = GDALGetDriverByName("MEM");
			GDALDatasetH memDS = GDALCreate(memDriver, "", options->tileSize, options->tileSize, bands, data_type, nullptr);

			if (!memDS) {
				memory_allocator->deallocate(pData);
				return false;
			}

			// д�����ݵ��ڴ����ݼ�����Ҫ�ز���
			err = GDALDatasetRasterIO(
				memDS, GF_Write,
				0, 0, options->tileSize, options->tileSize,
				pData, width, height,
				data_type, bands, nullptr,
				0, 0, 0
			);

			// �ͷ�ԭʼ�����ڴ�
			memory_allocator->deallocate(pData);

			if (err != CE_None) {
				GDALClose(memDS);
				return false;
			}

			// �����������
			GDALDriverH outputDriver = GDALGetDriverByName(options->outputFormat.c_str());
			if (!outputDriver) {
				GDALClose(memDS);
				return false;
			}

			// �����ڴ��ļ�
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

			// ʹ���ڴ��ļ�ϵͳ������ʱ�ļ�
			std::string vsimem_filename = "/vsimem/temp_tile_" + std::to_string(zoom) + "_" +
				std::to_string(tile_x) + "_" + std::to_string(tile_y);

			GDALDatasetH outDS = GDALCreateCopy(
				outputDriver, vsimem_filename.c_str(),
				memDS, FALSE, optionsList.List(), nullptr, nullptr
			);

			// �ر����ݼ�
			GDALClose(memDS);

			if (!outDS) {
				VSIUnlink(vsimem_filename.c_str());
				return false;
			}

			GDALClose(outDS);

			// ���ڴ��ļ�ϵͳ��ȡ����
			vsi_l_offset size = 0;
			unsigned char* data = VSIGetMemFileBuffer(vsimem_filename.c_str(), &size, TRUE);

			if (data && size > 0) {
				// ���ڴ������ƶ���vector��
				std::vector<unsigned char> buffer_data(data, data + size);
				CPLFree(data); // �ͷ�GDAL������ڴ�

				// ���ļ�����д�����
				file_buffer->write_file(tile_path, std::move(buffer_data));
				return true;
			}

			return false;
		}
		catch (const std::exception& e) {
			std::cerr << "��Ƭ�����쳣: " << e.what() << std::endl;
			return false;
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
		total_tiles_processed = 0;

		// ʹ��TBB���д���
		tbb::parallel_for(
			tbb::blocked_range2d<int, int>(min_tile_y, max_tile_y + 1, min_tile_x, max_tile_x + 1),
			[this, zoom, total_tiles,&progressInfo](const tbb::blocked_range2d<int, int>& r) {
				for (int tile_y = r.rows().begin(); tile_y != r.rows().end(); ++tile_y) {
					for (int tile_x = r.cols().begin(); tile_x != r.cols().end(); ++tile_x) {
						// ���ɵ�����Ƭ
						if (generate_tile(zoom, tile_x, tile_y)) {
							// ���½���
							int processed = ++total_tiles_processed;
							if (processed % 50 == 0 || processed == total_tiles) {
								// ��ʾ����
								progressInfo->showProgress(processed, std::to_string(zoom) + "/" + std::to_string(tile_x) + "/" + std::to_string(tile_y), "������Ƭ");
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
		// ע��GDAL����
		GDALAllRegister();

		// �����߳�����
		if (options->numThreads > 0) {
			std::cout << "�����߳�����: " << options->numThreads << std::endl;
			tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, options->numThreads);
		}

		// �����ݼ�
		GDALOpenInfo* poOpenInfo = new GDALOpenInfo(options->inputFile.c_str(), GA_ReadOnly);

		// ����Ƿ���GDAL֧�ֵĸ�ʽ
		if (!GDALIdentifyDriver(poOpenInfo->pszFilename, nullptr)) {
			std::cerr << "�޷�ʶ�������ļ���ʽ: " << options->inputFile << std::endl;
			delete poOpenInfo;
			return false;
		}

		// ʹ��VSI�ļ�ϵͳ
		if (options->useMemoryMapping) {
			delete poOpenInfo;
			std::string virtual_file = "/vsimem/input_" + std::to_string(std::time(nullptr));
			std::cout << "ʹ���ڴ�ӳ�䷽ʽ�����ļ�: " << options->inputFile << std::endl;

			// ʹ��GDAL��VSI�ļ�ϵͳ�����ļ���ȡ������ϵͳ����
			// ���ַ�����ƽ̨�����������ض�����ϵͳ��mmapϵͳ����

			// ���ļ��Զ�ȡ
			VSILFILE* fp = VSIFOpenL(options->inputFile.c_str(), "rb");
			if (fp == nullptr) {
				std::cerr << "�޷��������ļ�: " << options->inputFile << std::endl;
				return false;
			}

			// ��ȡ�ļ���С
			VSIFSeekL(fp, 0, SEEK_END);
			size_t file_size = static_cast<size_t>(VSIFTellL(fp));
			VSIFSeekL(fp, 0, SEEK_SET);

			// �����ڴ�
			GByte* buffer = static_cast<GByte*>(VSIMalloc(file_size));
			if (buffer == nullptr) {
				std::cerr << "�ڴ����ʧ��: " << options->inputFile << std::endl;
				VSIFCloseL(fp);
				return false;
			}

			// ��ȡ�����ļ����ڴ�
			if (VSIFReadL(buffer, 1, file_size, fp) != file_size) {
				std::cerr << "��ȡ�ļ����ڴ�ʧ��: " << options->inputFile << std::endl;
				VSIFree(buffer);
				VSIFCloseL(fp);
				return false;
			}

			// �ر�ԭʼ�ļ�
			VSIFCloseL(fp);

			// ����GDAL�����ļ�
			VSILFILE* vsi_mem_fp = VSIFileFromMemBuffer(virtual_file.c_str(), buffer, file_size, TRUE);

			if (vsi_mem_fp == nullptr) {
				std::cerr << "�����ڴ��ļ�ʧ��: " << options->inputFile << std::endl;
				VSIFree(buffer);
				return false;
			}

			// ����ʹ�������ļ������ݼ�
			dataset = GDALOpen(virtual_file.c_str(), GA_ReadOnly);

			if (dataset == nullptr) {
				std::cerr << "�����ݼ�ʧ��: " << virtual_file << std::endl;
				// ע�⣺��Ҫ�ر�vsi_mem_fp����ΪVSIFileFromMemBuffer�Ѿ������������
				// �����ǲ�����Ҫbufferʱ���ᱻGDAL����
				return false;
			}
		}
		else {
			// ֱ�Ӵ��ļ�
			delete poOpenInfo;
			dataset = GDALOpen(options->inputFile.c_str(), GA_ReadOnly);
		}

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
		double corners[8] = {
			0, 0,                   // ���Ͻ�
			img_width, 0,           // ���Ͻ�
			img_width, img_height,  // ���½�
			0, img_height           // ���½�
		};

		// Ӧ�õ���任
		for (int i = 0; i < 4; ++i) {
			double x = corners[i * 2];
			double y = corners[i * 2 + 1];

			// ��������תΪ��������
			double geo_x = geo_transform[0] + x * geo_transform[1] + y * geo_transform[2];
			double geo_y = geo_transform[3] + x * geo_transform[4] + y * geo_transform[5];

			corners[i * 2] = geo_x;
			corners[i * 2 + 1] = geo_y;
		}

		// ת����WGS84����ϵ
		coord_system->transform_points(corners, 4);

		// �ҳ�����Χ
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

		// ȷ����Χ����Ч�ľ�γ�ȷ�Χ��
		min_x = std::max(min_x, MIN_LONGITUDE);
		max_x = std::min(max_x, MAX_LONGITUDE);
		min_y = std::max(min_y, MIN_LATITUDE);
		max_y = std::min(max_y, MAX_LATITUDE);

		std::cout << "����Χ: "
			<< "����=" << min_x << "��" << max_x
			<< ", γ��=" << min_y << "��" << max_y << std::endl;

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

		//�������е���Ƭ�� ����������Ĵ���
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
			process_zoom_level(zoom,progressInfo);
		}

		// �ȴ������ļ�д�����
		std::cout << "�ȴ��ļ�д�����..." << std::endl;
		file_buffer->wait_completion();

		// �����ܴ���ʱ��
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

		// ���ͳ����Ϣ
		size_t total_bytes = file_buffer->get_bytes_written();
		size_t total_files = file_buffer->get_files_written();

		std::cout << "\n��Ƭ���!" << std::endl;
		std::cout << "����Ƭ��: " << total_files << std::endl;
		std::cout << "��������: " << (total_bytes / (1024.0 * 1024.0)) << " MB" << std::endl;
		std::cout << "�ܴ���ʱ��: " << duration << " ��" << std::endl;
		if (duration > 0) {
			std::cout << "ƽ���ٶ�: " << (total_files / duration) << " ��Ƭ/��, "
				<< (total_bytes / (1024.0 * 1024.0) / duration) << " MB/��" << std::endl;
		}

		return true;



		if (!dataset) {
			std::cerr << "���ݼ�δ��ʼ��" << std::endl;
			return false;
		}

		// ����TBB�߳�����
		tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, options->numThreads);

		

		// �����������ż���
		for (int zoom = options->minLevel; zoom <= options->maxLevel; zoom++) {
			std::cout << "�������ż���: " << zoom << std::endl;

			// ���������ż����Ŀ¼
			create_directories(zoom);

			// ����ü������Ƭ��Χ
			int min_tile_x, min_tile_y, max_tile_x, max_tile_y;
			get_tile_range(zoom, min_tile_x, min_tile_y, max_tile_x, max_tile_y);

			std::cout << "  ��Ƭ��Χ: X(" << min_tile_x << "-" << max_tile_x
				<< "), Y(" << min_tile_y << "-" << max_tile_y << ")" << std::endl;
			std::atomic<int> processed_tiles(0);

			// ʹ��TBB���д���
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