#pragma once
// ��Ƭ������
#include <filesystem>
#include <atomic>
#include <mutex>
// GDAL��
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

#include "FileBatchOutput.h"
#include "IDataM.h"
#include "CoordinateSystemManager.h"
#include "IProgressInfo.h"
#include "ImageFileParallelIOAdapter.h"

namespace fs = std::filesystem;

namespace WT{
	class SlippyMapTilerOptions :public IDataOptions {
	public:
		std::string inputFile="";//������ļ�
		std::string outputDir="";//���·��
		int minLevel = -9999;//��С��Ƭ����
		int maxLevel = -9999;//�����Ƭ����
		int tileSize = 256;//��Ƭ��С
		std::vector<double> nodata;// = { 255,255,255 };//NoData���� ���Ӱ��û��nodata��ʹ�����
		IMAGEFORMAT outputFormat = IMAGEFORMAT::JPG;//�����Ƭ��׺
		int numThreads = std::thread::hardware_concurrency()-1;//ʹ�õ��߳���
		std::string prjFilePath = "";//�ⲿprj�ļ�·�� ����Ϊ��
		std::string wktString = "";//�ⲿwkt�ַ��� ����Ϊ��
	};

	// �����Ƭ�߽���Ϣ�ṹ
	struct TileBounds {
		int dst_min_x, dst_min_y, dst_max_x, dst_max_y;
		int clipped_src_min_x, clipped_src_min_y, clipped_src_max_x, clipped_src_max_y;
		int dst_width, dst_height;
		int read_width, read_height;
	};

	// Ӱ����Ϣ�ṹ
	struct ImageInfo {
		bool has_palette;
		int output_band_count;
		GDALColorTableH color_table;
		size_t pixel_size;
		GDALDataType data_type;
	};

	class SlippyMapTiler :public IDataProcessor {
	public:
		SlippyMapTiler(std::shared_ptr<SlippyMapTilerOptions> options);
		~SlippyMapTiler();
		bool process(std::shared_ptr<IProgressInfo> progressInfo) override;
		//��ȡ����������
		virtual std::string getName()const { return "Ӱ����Ƭ��"; };
		bool initialize();
		void setOptions(std::shared_ptr<IDataOptions> options);
	private:
		// ��������
		int DEFAULT_TILE_SIZE = 256;
		int DEFAULT_MAX_QUEUE_SIZE = 5000;
		int DEFAULT_JPEG_QUALITY = 90;
		double MAX_LATITUDE = 85.0511;
		double MIN_LATITUDE = -85.0511;
		double MAX_LONGITUDE = 180.0;
		double MIN_LONGITUDE = -180.0;
		double EARTH_RADIUS = 6378137;
	private:
		//������ʹ�õ�Options
		std::shared_ptr<SlippyMapTilerOptions> options;
		GDALDatasetH dataset=nullptr;
		double geo_transform[6];
		int img_width, img_height;
		double min_x, min_y, max_x, max_y;
		int band_count;
		GDALDataType data_type;
		ImageInfo image_info;

		std::vector<double> maxs;//ÿ�����ε����ֵ
		std::vector<double> mins;//ÿ�����ε���Сֵ

		// ����ϵͳ��ת��
		std::unique_ptr<CoordinateSystem> coord_system;

		/*std::shared_ptr<JemallocAllocator> memory_allocator;
		std::shared_ptr<FileBufferManager> file_buffer;*/
		std::shared_ptr<FileBatchOutput> fileBatchOutputer;

		// ͳ����Ϣ
		std::atomic<int> total_tiles_processed;
		std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

		// ���߳�ͬ���õĻ�����
		std::mutex gdal_mutex;
		std::mutex progress_mutex;
		std::mutex fs_mutex;


		// �������Ŀ¼
		void create_directories(int zoom);

		/**
		 * @brief ��ȡĳһ�������Ƭ��Χ
		 * @param zoom ���ż���
		 * @param min_tile_x ��СX��Ƭ����
		 * @param min_tile_y ��СY��Ƭ����
		 * @param max_tile_x ���X��Ƭ����
		 * @param max_tile_y ���Y��Ƭ����
		 */
		void get_tile_range(int zoom, int& min_tile_x, int& min_tile_y, int& max_tile_x, int& max_tile_y);

		/**
		 * @brief ����ָ����Ƭ��WGS84�еľ�γ�ȷ�Χ
		 * @param zoom ���ż���
		 * @param tile_x ��ƬX����
		 * @param tile_y ��ƬY����
		 * @param tile_min_x ��Ƭ��С����
		 * @param tile_min_y ��Ƭ��Сγ��
		 * @param tile_max_x ��Ƭ��󾭶�
		 * @param tile_max_y ��Ƭ���γ��
		 */
		void get_tile_geo_bounds(int zoom, int tile_x, int tile_y, double& tile_min_x, double& tile_min_y, double& tile_max_x, double& tile_max_y);

		/**
		 * @brief ����Ƭ�������굽ԭʼӰ�����������ת��
		 * @param geo_x ����X����
		 * @param geo_y ����Y����
		 * @param pixel_x ���������X����
		 * @param pixel_y ���������Y����
		 * @return �Ƿ�ת���ɹ�
		 */
		bool geo_to_pixel(const double& geo_x, const double& geo_y, int& pixel_x, int& pixel_y);

		/**
		* @brief ���ɵ�����Ƭ
		* @param zoom ���ż���
		* @param tile_x ��ƬX����
		* @param tile_y ��ƬY����
		* @return �Ƿ�ɹ�������Ƭ
		*/
		bool generate_tile(int zoom, int tile_x, int tile_y, GDALDatasetH local_dataset);

		/**
		* @brief ����ָ�����ż����������Ƭ
		* @param zoom ���ż���
		*/
		void process_zoom_level(int zoom,std::shared_ptr<IProgressInfo> progressInfo);


		/**
		 * ������ת��Ϊ��Ƭ���� X ֵ������ Web Mercator ͶӰ��
		 *
		 * @param lon ���ȣ���λ���ȣ���Χ -180 �� 180��
		 * @param z ���ż���ͨ�� 0-20��ȡ���ڵ�ͼ����
		 * @return ��Ƭ X ���꣨�� 0 ��ʼ��
		 */
		int long2tilex(double lon, int z);

		/**
		 * ��γ��ת��Ϊ��Ƭ���� Y ֵ������ Web Mercator ͶӰ��
		 *
		 * @param lat γ�ȣ���λ���ȣ���Χ -85.0511 �� 85.0511�������ᱻͶӰ�ضϣ�
		 * @param z ���ż���ͨ�� 0-20��ȡ���ڵ�ͼ����
		 * @return ��Ƭ Y ���꣨�� 0 ��ʼ��
		 */
		int lat2tiley(double lat, int z);

		/**
		 * ����Ƭ X ����ת���ؾ��ȣ����� Web Mercator ͶӰ��
		 *
		 * @param x ��Ƭ X ���꣨�� 0 ��ʼ��
		 * @param z ���ż��𣨱�������Ƭ��������ʱһ�£�
		 * @return ���ȣ���λ���ȣ���Χ -180 �� 180��
		 */
		double tilex2long(int x, int z);

		/**
		 * ����Ƭ Y ����ת����γ�ȣ����� Web Mercator ͶӰ��
		 *
		 * @param y ��Ƭ Y ���꣨�� 0 ��ʼ��
		 * @param z ���ż��𣨱�������Ƭ��������ʱһ�£�
		 * @return γ�ȣ���λ���ȣ���Χ -85.0511 �� 85.0511��
		 */
		double tiley2lat(int y, int z);

		//�����ض��㼶�� �ж�������
		double mapSize(int level, int tileSize);

		//����ָ��γ����ָ���㼶�µĵ���ֱ��� ��λm
		double groundResolution(double latitude, double level, int tileSize);
	
		//��������ֱ����� �����Ƭ�㼶
		int getProperLevel(double groundResolution, int tileSize);

		// ����һ���̱߳��ص�GDAL���ݼ�
		GDALDatasetH create_local_dataset();

		// ����һ���̰߳�ȫ�Ľ��ȸ��º���
		void update_progress(int zoom, int tile_x, int tile_y, int total_tiles, std::shared_ptr<IProgressInfo> progressInfo);

		// ������Ƭ�߽��ӳ���ϵ
		bool calculate_tile_bounds(int src_min_x, int src_min_y, int src_max_x, int src_max_y, TileBounds& bounds);
		

		// ��ȡӰ����Ϣ����ɫ�塢�������ȣ�
		ImageInfo get_image_info(GDALDatasetH dataset);
		

		// �������ɫ��ĵ�����Ӱ��TBB���л���
		bool process_palette_image(GDALDatasetH dataset, const TileBounds& bounds,
			const ImageInfo& info, size_t dataBufferSize, unsigned char* dataBuffer,unsigned char* alphaBuffer);
		

		// ������ͨ�ನ�λ򵥲���Ӱ��TBB���л���
		bool process_regular_image(GDALDatasetH dataset, const TileBounds& bounds,
			const ImageInfo& info, size_t dataBufferSize, unsigned char* output_buffer, unsigned char* alphaBuffer);
		

		// ȷ�����Ŀ¼���ڣ��̰߳�ȫ��
		bool ensure_output_directory(const fs::path& x_dir);

		//���������ŵ�8λ
		bool scaleDataRange(unsigned char* pData,unsigned char* outData,std::vector<double>& statisticMax, std::vector<double>& statisticMin);
		
		//�жϸ�������������������ֵ�Ƿ���nodataֵ��Ӧ��� �Ӷ��ж��Ƿ�Ϊnodata
		bool checkNodata(unsigned char* pData, size_t pos, int pixelSize, int bandNum);

		//�ϲ������ݺ�͸��ͼ������ �����finalData���ڲ����� ����ֵ�����Ƿ�ϲ� �Ӷ�������false��pData����ture��finalData
		bool mergeDataAndAlpha(unsigned char* pData, unsigned char* alphaData,unsigned char*& finalData,int bandCount,int pixelCount);

		//�����ɫ���е���ɫ�����Сֵ
		void calcuPaletteColorValueRange();
		// �Ƚϸ������Ƿ���ȵĸ�������
		template<typename T>
		bool isEqual(T a, T b, T epsilon = 0.000001) {
			if (std::is_floating_point<T>::value) {
				return std::fabs(a - b) <= epsilon;
			}
			return a == b;
		}
		//���Ÿ�������
		template<typename T>
		void scaleValue(unsigned char* pData,unsigned char* outData,int pixelCount,int bands,std::vector<double>& statisticMin,std::vector<double>& statisticMax) {
			T* srcData = reinterpret_cast<T*>(pData);

			//// ��ÿ�����ηֱ���
			std::vector<double> rangeEachBand;
			std::vector<double> scaleEachBand;
			for (int i = 0; i < bands; i++)
			{
				double range = statisticMax[i] - statisticMin[i];
				double scale = (range > 0) ? 255.0 / range : 0.0;
				scaleEachBand.push_back(scale);
			}

			for (int i = 0; i < pixelCount; ++i) {
				int pixelPos = bands * i;
				for (int j = 0; j < bands; ++j)
				{
					int valuePos = pixelPos + j;
					T value = srcData[valuePos];
					// �������ŵ�0-255��Χ
					double scaled = (value - statisticMin[j]) * scaleEachBand[j];
					// ������0-255��Χ��
					outData[valuePos] = static_cast<unsigned char>(
						std::max(0.0, std::min(255.0, scaled)));
				}
			}

		}
	};
};