#pragma once
// ��Ƭ������
#include <filesystem>
// GDAL��
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

// TBB��
#include "tbb/task_group.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"
#include "tbb/global_control.h"
#include "tbb/concurrent_queue.h"

#include "MemoryPool.h"
#include "FileBufferManager.h"
#include "IDataM.h"
#include "CoordinateSystemManager.h"

namespace fs = std::filesystem;

namespace WT{
class SlippyMapTilerOptions :public IDataOptions {
public:
	std::string inputFile="";//������ļ�
	std::string outputDir="";//���·��
	int minLevel = 0;//��С��Ƭ����
	int maxLevel = 5;//�����Ƭ����
	int tileSize = 255;//��Ƭ��С
	bool useVRT = true;//�Ƿ�ʹ��VRT
	bool useMemoryMapping = true;//�Ƿ������ļ�ӳ��
	std::string outputFormat = "png";//�����Ƭ��׺
	int numThreads = 5;//ʹ�õ��߳���
	std::string prjFilePath = "";//�ⲿprj�ļ�·�� ����Ϊ��
	std::string wktString = "";//�ⲿwkt�ַ��� ����Ϊ��
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
private:
	//������ʹ�õ�Options
	std::shared_ptr<SlippyMapTilerOptions> options;
	GDALDatasetH dataset;
	double geo_transform[6];
	int img_width, img_height;
	double min_x, min_y, max_x, max_y;
	int band_count;
	GDALDataType data_type;

	// ����ϵͳ��ת��
	std::unique_ptr<CoordinateSystem> coord_system;

	std::shared_ptr<JemallocAllocator> memory_allocator;
	std::shared_ptr<FileBufferManager> file_buffer;

	// ͳ����Ϣ
	std::atomic<int> total_tiles_processed;
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

	//// ʹ�÷������Ļ������洢���õı���
	//struct {
	//	int tile_size = 256;
	//} config;

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
	bool generate_tile(int zoom, int tile_x, int tile_y);

	/**
	* @brief ����ָ�����ż����������Ƭ
	* @param zoom ���ż���
	*/
	void process_zoom_level(int zoom,std::shared_ptr<IProgressInfo> progressInfo);
};
}