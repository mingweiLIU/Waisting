#include "SlippyTiler.h"
namespace WT{
	SlippyMapTiler::SlippyMapTiler(std::shared_ptr<SlippyMapTilerOptions> options) {
		this->options = options;
		// ��ʼ��GDAL
		GDALAllRegister();

		// �����ڴ��
		memory_pool = std::make_shared<MemoryPool>(1024 * 1024 * 10); // 10MB ���С

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

	void SlippyMapTiler::geo_to_pixel(const double& geo_x, const double& geo_y, int& pixel_x, int& pixel_y)
	{
		pixel_x = static_cast<int>((geo_x - geo_transform[0]) / geo_transform[1]);
		pixel_y = static_cast<int>((geo_y - geo_transform[3]) / geo_transform[5]);
	}

	bool SlippyMapTiler::generate_tile(int zoom, int tile_x, int tile_y)
	{
		try {
			// ������Ƭ�ĵ���Χ
			double tile_min_x, tile_min_y, tile_max_x, tile_max_y;
			get_tile_geo_bounds(zoom, tile_x, tile_y, tile_min_x, tile_min_y, tile_max_x, tile_max_y);

			// ������Ƭ·��
			std::string outputDir = options->outputDir;
			fs::path x_dir = fs::path(outputDir) / std::to_string(zoom) / std::to_string(tile_x);
			if (!fs::exists(x_dir)) {
				fs::create_directories(x_dir);
			}

			std::string tile_path = (x_dir / (std::to_string(tile_y) + "." + options->outputFormat)).string();

			// ������Ƭ��ԭʼӰ���е����ط�Χ
			int src_min_x, src_min_y, src_max_x, src_max_y;
			geo_to_pixel(tile_min_x, tile_max_y, src_min_x, src_min_y);
			geo_to_pixel(tile_max_x, tile_min_y, src_max_x, src_max_y);

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

			// �����ڴ����ݼ�
			void* pData = memory_pool->allocate(width * height * 3); // ����RGB
			if (!pData) {
				std::cerr << "�ڴ����ʧ��!" << std::endl;
				return false;
			}

			// ��ȡ����
			int bands = GDALGetRasterCount(dataset);
			bands = std::min(bands, 3); // ����Ϊ���3������

			// ʹ�� RasterIO ��ȡ����
			CPLErr err = GDALDatasetRasterIO(
				dataset, GF_Read,
				src_min_x, src_min_y, width, height,
				pData, width, height,
				GDT_Byte, bands, nullptr,
				0, 0, 0
			);

			if (err != CE_None) {
				memory_pool->deallocate(pData);
				return false;
			}

			// ����Ŀ����Ƭ���ݼ�
			GDALDriverH memDriver = GDALGetDriverByName("MEM");
			GDALDatasetH memDS = GDALCreate(memDriver, "", config.tile_size, config.tile_size, bands, GDT_Byte, nullptr);

			if (!memDS) {
				memory_pool->deallocate(pData);
				return false;
			}

			// д�����ݵ��ڴ����ݼ�
			err = GDALDatasetRasterIO(
				memDS, GF_Write,
				0, 0, config.tile_size, config.tile_size,
				pData, width, height,
				GDT_Byte, bands, nullptr,
				0, 0, 0
			);

			if (err != CE_None) {
				GDALClose(memDS);
				memory_pool->deallocate(pData);
				return false;
			}

			// �ͷ�ԭʼ�����ڴ�
			memory_pool->deallocate(pData);

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

			// ʹ���ڴ��ļ�ϵͳ������ʱ�ļ�
			std::string vsimem_filename = "/vsimem/temp_tile_" + std::to_string(zoom) + "_" +
				std::to_string(tile_x) + "_" + std::to_string(tile_y);

			GDALDatasetH outDS = GDALCreateCopy(
				outputDriver, vsimem_filename.c_str(),
				memDS, FALSE, optionsList.List(), nullptr, nullptr
			);

			// �ر����ݼ�
			GDALClose(memDS);
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

	bool SlippyMapTiler::initialize()
	{
		// �����ݼ�
		dataset = GDALOpen(options->inputFile.c_str(), GA_ReadOnly);
		if (!dataset) {
			std::cerr << "�޷��������ļ�: " << options->inputFile << std::endl;
			return false;
		}

		// ��ȡ����任����
		if (GDALGetGeoTransform(dataset, geo_transform) != CE_None) {
			std::cerr << "�޷���ȡ����任����" << std::endl;
			GDALClose(dataset);
			dataset = nullptr;
			return false;
		}

		// ��ȡӰ��ߴ�
		img_width = GDALGetRasterXSize(dataset);
		img_height = GDALGetRasterYSize(dataset);

		// ����Ӱ��ı߽緶Χ (WGS84��γ��)
		min_x = geo_transform[0];
		max_y = geo_transform[3];
		max_x = geo_transform[0] + img_width * geo_transform[1] + img_height * geo_transform[2];
		min_y = geo_transform[3] + img_width * geo_transform[4] + img_height * geo_transform[5];

		// ���������Ŀ¼
		fs::create_directories(options->outputDir);

		return true;
	}

	bool SlippyMapTiler::process(std::shared_ptr<IProgressInfo> progressInfo)
	{
		if (!dataset) {
			std::cerr << "���ݼ�δ��ʼ��" << std::endl;
			return false;
		}

		// ����TBB�߳�����
		tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism, options->numThreads);

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

							// ��ʾ����
							progressInfo->showProgress(processed_tiles, std::to_string(zoom)+"/"+std::to_string(x)+"/"+std::to_string(y), "������Ƭ");
						}
					}
				}
			);

			progressInfo->finished();
		}

		return true;
	}
}