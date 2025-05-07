#pragma once
// ��Ƭ������
#include <filesystem>

// GDAL��
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <cpl_vsi.h>

// TBB��
#include <tbb/task_group.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#include <tbb/global_control.h>
#include <tbb/concurrent_queue.h>

#include "MemoryPool.h"
#include "FileBufferManager.h"
#include "IDataM.h"

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
	//������ʹ�õ�Options
	std::shared_ptr<SlippyMapTilerOptions> options;
	GDALDatasetH dataset;
	double geo_transform[6];
	int img_width, img_height;
	double min_x, min_y, max_x, max_y;

	std::shared_ptr<MemoryPool> memory_pool;
	std::shared_ptr<FileBufferManager> file_buffer;

	// ʹ�÷������Ļ������洢���õı���
	struct {
		int tile_size = 256;
	} config;

	// �������Ŀ¼
	void create_directories(int zoom);

	// ��ȡĳһ�������Ƭ��Χ
	void get_tile_range(int zoom, int& min_tile_x, int& min_tile_y, int& max_tile_x, int& max_tile_y);

	// ����ָ����Ƭ��ԭʼӰ���еķ�Χ
	void get_tile_geo_bounds(int zoom, int tile_x, int tile_y, double& tile_min_x, double& tile_min_y, double& tile_max_x, double& tile_max_y);

	// ����������ת��Ϊ��������
	void geo_to_pixel(const double& geo_x, const double& geo_y, int& pixel_x, int& pixel_y);

	// ���ɵ�����Ƭ
	bool generate_tile(int zoom, int tile_x, int tile_y);
};
}