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

	//专门定义为图片输出器，支持png，jpg，web，直接将从gdal中读取的data转为图片
	// TBB前向声明
	namespace tbb {
		class global_control;
	}

	// 图像格式枚举
	enum class IMAGEFORMAT {
		PNG,
		JPG,
		WEBP
	};

	// JPEG选项结构体
	struct JPEGOptions {
		int quality = 85;
		bool optimize_coding = true;
		J_COLOR_SPACE color_space = JCS_RGB;
	};

	// 前向声明
	struct ImageTask;

	class ImageFileParallelIOAdapter:public IOAdapter {
	public:
		// 构造函数
		ImageFileParallelIOAdapter(const std::string& basePath, bool createDirs = true,
			int width = 256, int height = 256,
			std::string format = "png", int bands = 3,
			std::vector<double> noData = {});

		// 析构函数
		~ImageFileParallelIOAdapter() = default;

		// 初始化
		bool initialize() override;

		// 输出单个文件（异步）
		bool output(const IOFileInfo* fileInfo) override;

		// 批量输出（并行同步）
		bool outputBatch(const std::vector<IOFileInfo*> files) override;

		// 批量输出（完全异步）
		bool outputBatchAsync(const std::vector<IOFileInfo*> files) override;


		std::string type() const override { return "ImageFileParallelIOAdapter"; }

		// 完成输出
		bool finalize();

	private:
		// 成员变量
		std::string mBasePath;
		bool mCreateDirs;
		int mWidth;
		int mHeight;
		int mBandsNum;
		std::vector<double> mNoData;
		IMAGEFORMAT mFormat;

		// TBB控制（可选）
		// std::unique_ptr<tbb::global_control> mTbbControl;

		// 私有方法

		// 处理单个图像任务
		bool processImageTask(const ImageTask& task);

		// 将数据转换为图像
		bool dataToImage(IOFileInfo* ioFileInfo);

		// JPEG相关方法
		bool writeJPEG(IOFileInfo* fileInfo, int width, int height, const JPEGOptions& opts);
		bool writeRGBJPEG(IOFileInfo* fileInfo, int width, int height, int quality = 85);
		bool writeGrayscaleJPEG(IOFileInfo* fileInfo, int width, int height, int quality = 85);

		// PNG相关方法
		bool saveGDALDataAsPNG(IOFileInfo* fileInfo, int width, int height,
			int bandCount, int bitDepth, const std::vector<double>& nodata);
		bool initPNG(FILE* fp, png_structp& png_ptr, png_infop& info_ptr);

		// 原有的像素处理方法
		void processPixelData(const unsigned char* imageData, std::vector<unsigned char>& outData,
			int width, int height, int bandCount, int outBandCount, int bitDepth,
			const std::vector<double>& nodata, bool requiresAlpha);

		// 新的并行像素处理方法
		void processPixelDataParallel(const unsigned char* imageData, std::vector<unsigned char>& outData,
			int width, int height, int bandCount, int outBandCount, int bitDepth,
			const std::vector<double>& nodata, bool requiresAlpha);

		// nodata检测方法
		bool hasNodataPixels(const unsigned char* imageData, int width, int height,
			int bandCount, int bitDepth, const std::vector<double>& nodata);

		// 并行nodata检测方法
		bool hasNodataPixelsParallel(const unsigned char* imageData, int width, int height,
			int bandCount, int bitDepth, const std::vector<double>& nodata);

		bool isNodataPixel(const unsigned char* imageData, int pixelPos, int bandCount,
			int bitDepth, const std::vector<double>& nodata);
	};
};