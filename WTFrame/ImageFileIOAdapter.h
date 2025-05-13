#pragma once
#include "IOAdapter.h"
#include <filesystem>
#include <jpeglib.h>
#include <libpng16/png.h>
namespace WT {

	//专门定义为图片输出器，支持png，jpg，web，直接将从gdal中读取的data转为图片
	class ImageFileIOAdapter : public IOAdapter {
	public:
		struct JPEGOptions
		{
			int quality = 85;//压缩质量0-100
			bool optimize_coding = true;//优化Huffman表
			J_COLOR_SPACE color_space = JCS_RGB;
		};
		enum class IMAGEFORMAT
		{
			PNG,
			JPG,
			WEBP
		};
	public:
		explicit ImageFileIOAdapter(const std::string& basePath, bool createDirs = true
			,int width = 256,int height=256, std::string format="png"
			, int bands = 3, std::vector<double> noData = {});

		bool initialize() override;
		bool output(const IOFileInfo fileInfo) override;
		bool outputBatch(const std::vector<IOFileInfo> files) override;
		bool finalize() override;
		std::string type() const override { return "ImageFileIOAdapter"; }
	private:
		bool dataToImage(IOFileInfo ioFileInfo);

		bool writeJPEG(IOFileInfo fileInfo, int width, int height, const JPEGOptions& opts = {});
		// 从RGB缓冲区写入
		bool writeRGBJPEG(IOFileInfo fileInfo, int width, int height, int quality = 85);
		// 从灰度缓冲区写入
		bool writeGrayscaleJPEG(IOFileInfo fileInfo, int width, int height, int quality = 85);

		//写出png
		bool write_png(IOFileInfo fileInfo, int width, int height,int bands,int color_type = PNG_COLOR_TYPE_RGB,int bit_depth = 8);

		//根据Nodata情况拼装透明图层 如果需要半透明 则返回为true 不需要则false
		bool nodataCheckAndTrans(unsigned char* oriData, int oriDataSize, unsigned char* newData, int& newDataSize);

	private:
		std::string mBasePath;
		bool mCreateDirs;
		IMAGEFORMAT mFormat;
		int mWidth;
		int mHeight;
		int mBandsNum;//波段数
		std::vector<double> mNoData;
	};
};