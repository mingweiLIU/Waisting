#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <future>
#include <memory>
#include "IOAdapter.h"
#include <filesystem>
#include <jpeglib.h>
#include <libpng16/png.h>
namespace WT {

	//ר�Ŷ���ΪͼƬ�������֧��png��jpg��web��ֱ�ӽ���gdal�ж�ȡ��dataתΪͼƬ
	// TBBǰ������
	namespace tbb {
		class global_control;
	}

	// ͼ���ʽö��
	enum class IMAGEFORMAT {
		PNG,
		JPG,
		WEBP
	};

	// JPEGѡ��ṹ��
	struct JPEGOptions {
		int quality = 85;
		bool optimize_coding = true;
		J_COLOR_SPACE color_space = JCS_RGB;
	};

	// ǰ������
	struct ImageTask;

	class ImageFileParallelIOAdapter:public IOAdapter {
	public:
		// ���캯��
		ImageFileParallelIOAdapter(const std::string& basePath, bool createDirs = true,
			int width = 256, int height = 256,
			std::string format = "png", int bands = 3,
			std::vector<double> noData = {});

		// ��������
		~ImageFileParallelIOAdapter() = default;

		// ��ʼ��
		bool initialize() override;

		// ��������ļ����첽��
		bool output(const IOFileInfo* fileInfo) override;

		// �������������ͬ����
		bool outputBatch(const std::vector<IOFileInfo*> files) override;

		// �����������ȫ�첽��
		bool outputBatchAsync(const std::vector<IOFileInfo*> files) override;


		std::string type() const override { return "ImageFileParallelIOAdapter"; }

		// ������
		bool finalize();

	private:
		// ��Ա����
		std::string mBasePath;
		bool mCreateDirs;
		int mWidth;
		int mHeight;
		int mBandsNum;
		std::vector<double> mNoData;
		IMAGEFORMAT mFormat;

		// TBB���ƣ���ѡ��
		// std::unique_ptr<tbb::global_control> mTbbControl;

		// ˽�з���

		// ������ͼ������
		bool processImageTask(const ImageTask& task);

		// ������ת��Ϊͼ��
		bool dataToImage(IOFileInfo* ioFileInfo);

		// JPEG��ط���
		bool writeJPEG(IOFileInfo* fileInfo, int width, int height, const JPEGOptions& opts);
		bool writeRGBJPEG(IOFileInfo* fileInfo, int width, int height, int quality = 85);
		bool writeGrayscaleJPEG(IOFileInfo* fileInfo, int width, int height, int quality = 85);

		// PNG��ط���
		bool saveGDALDataAsPNG(IOFileInfo* fileInfo, int width, int height,
			int bandCount, int bitDepth, const std::vector<double>& nodata);
		bool initPNG(FILE* fp, png_structp& png_ptr, png_infop& info_ptr);

		// ԭ�е����ش�����
		void processPixelData(const unsigned char* imageData, std::vector<unsigned char>& outData,
			int width, int height, int bandCount, int outBandCount, int bitDepth,
			const std::vector<double>& nodata, bool requiresAlpha);

		// �µĲ������ش�����
		void processPixelDataParallel(const unsigned char* imageData, std::vector<unsigned char>& outData,
			int width, int height, int bandCount, int outBandCount, int bitDepth,
			const std::vector<double>& nodata, bool requiresAlpha);

		// nodata��ⷽ��
		bool hasNodataPixels(const unsigned char* imageData, int width, int height,
			int bandCount, int bitDepth, const std::vector<double>& nodata);

		// ����nodata��ⷽ��
		bool hasNodataPixelsParallel(const unsigned char* imageData, int width, int height,
			int bandCount, int bitDepth, const std::vector<double>& nodata);

		bool isNodataPixel(const unsigned char* imageData, int pixelPos, int bandCount,
			int bitDepth, const std::vector<double>& nodata);
	};
};