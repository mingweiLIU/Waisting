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
		explicit ImageFileIOAdapter(const std::string& basePath, bool createDirs = true,int width = 256,int height=256, std::string format="png");

		bool initialize() override;
		bool output(const std::string& virtualPath, void* data, size_t dataSize) override;
		bool outputBatch(const std::vector<IOFileInfo> files) override;
		bool finalize() override;
		std::string type() const override { return "filesystem"; }
	private:
		bool dataToImage(IOFileInfo ioFileInfo);

		bool writeJPEG(IOFileInfo fileInfo, int width, int height, const JPEGOptions& opts = {});
		// 从RGB缓冲区写入
		bool writeRGBJPEG(IOFileInfo fileInfo, int width, int height, int quality = 85);
		// 从灰度缓冲区写入
		bool writeGrayscaleJPEG(IOFileInfo fileInfo, int width, int height, int quality = 85);

		//写出png
		bool write_png(IOFileInfo fileInfo, int width, int height,int color_type = PNG_COLOR_TYPE_RGB,int bit_depth = 8);

	private:
		std::string mBasePath;
		bool mCreateDirs;
		IMAGEFORMAT mFormat;
		int mWidth;
		int mHeight;
	};
};