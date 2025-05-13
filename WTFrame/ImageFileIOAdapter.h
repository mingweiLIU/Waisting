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

		/**
		 * 检查图像中是否包含nodata像素
		 *
		 * @param imageData 影像数据
		 * @param width 图像宽度
		 * @param height 图像高度
		 * @param bandCount 波段数
		 * @param bitDepth 位深度
		 * @param nodata nodata值数组
		 * @return 如果包含nodata像素则返回true，否则返回false
		 */
		bool hasNodataPixels(const unsigned char* imageData,
			int width, int height,
			int bandCount, int bitDepth,
			const std::vector<double>& nodata);
		/**
		 * 检查像素是否为nodata
		 *
		 * @param imageData 影像数据
		 * @param pixelPos 像素位置
		 * @param bandCount 波段数
		 * @param bitDepth 位深度
		 * @param nodata nodata值数组
		 * @return 如果是nodata像素则返回true，否则返回false
		 */
		bool isNodataPixel(const unsigned char* imageData,
			int pixelPos,
			int bandCount, int bitDepth,
			const std::vector<double>& nodata);

		/**
		 * 初始化PNG写入结构和信息结构
		 *
		 * @param fp 文件指针
		 * @param png_ptr 输出PNG写入结构指针
		 * @param info_ptr 输出PNG信息结构指针
		 * @return 成功返回true，失败返回false
		 */
		bool initPNG(FILE* fp, png_structp& png_ptr, png_infop& info_ptr);

		/**
		 * 处理像素数据，填充输出缓冲区
		 *
		 * @param imageData 输入影像数据
		 * @param outData 输出数据缓冲区
		 * @param width 图像宽度
		 * @param height 图像高度
		 * @param bandCount 波段数
		 * @param outBandCount 输出波段数(包含可能的透明通道)
		 * @param bitDepth 位深度
		 * @param nodata nodata值数组
		 * @param requiresAlpha 是否需要透明通道
		 */
		void processPixelData(const unsigned char* imageData,
			std::vector<unsigned char>& outData,
			int width, int height,
			int bandCount, int outBandCount,
			int bitDepth,
			const std::vector<double>& nodata,
			bool requiresAlpha);


		/**
		 * 将GDAL读取的数据转换为PNG图像，并处理nodata值为透明区域
		 * 如果所有像素都不是nodata，则创建不带透明通道的PNG
		 *
		 * @param imageData GDAL读取的影像数据，按RGBRGB...方式排列
		 * @param width 图像宽度
		 * @param height 图像高度
		 * @param bandCount 波段数(1或3)
		 * @param bitDepth 数据位深度(8或16)
		 * @param nodata nodata值数组，每个波段对应一个nodata值
		 * @param outputFilename 输出PNG文件路径
		 * @return 成功返回true，失败返回false
		 */
		bool saveGDALDataAsPNG(IOFileInfo fileInfo, 
			int width, int height,
			int bandCount, int bitDepth,
			const std::vector<double>& nodata);

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