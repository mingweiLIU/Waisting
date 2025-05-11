#pragma once
#include "IOAdapter.h"
#include <filesystem>
#include <jpeglib.h>
#include <libpng16/png.h>
namespace WT {

	//ר�Ŷ���ΪͼƬ�������֧��png��jpg��web��ֱ�ӽ���gdal�ж�ȡ��dataתΪͼƬ
	class ImageFileIOAdapter : public IOAdapter {
	public:
		struct JPEGOptions
		{
			int quality = 85;//ѹ������0-100
			bool optimize_coding = true;//�Ż�Huffman��
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
		// ��RGB������д��
		bool writeRGBJPEG(IOFileInfo fileInfo, int width, int height, int quality = 85);
		// �ӻҶȻ�����д��
		bool writeGrayscaleJPEG(IOFileInfo fileInfo, int width, int height, int quality = 85);

		//д��png
		bool write_png(IOFileInfo fileInfo, int width, int height,int color_type = PNG_COLOR_TYPE_RGB,int bit_depth = 8);

	private:
		std::string mBasePath;
		bool mCreateDirs;
		IMAGEFORMAT mFormat;
		int mWidth;
		int mHeight;
	};
};