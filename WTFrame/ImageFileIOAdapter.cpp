#include "ImageFileIOAdapter.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <limits>

// GDAL库
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

namespace WT {
	ImageFileIOAdapter::ImageFileIOAdapter(const std::string& basePath, bool createDirs
		, int width /*= 256*/, int height /*= 256*/
		, std::string format /*= "png"*/, int bands/*=3*/,  std::vector<double> noData/*={}*/)
		: mBasePath(basePath), mCreateDirs(createDirs)
		,mWidth(width),mHeight(height),mBandsNum(bands)
		, mNoData(noData){
		if ("png"==format)
		{
			this->mFormat = IMAGEFORMAT::PNG;
		}
		else if ("jpg" == format) {
			this->mFormat = IMAGEFORMAT::JPG;
		}else if ("webp"==format)
		{
			this->mFormat = IMAGEFORMAT::WEBP;
		}
	}

	bool ImageFileIOAdapter::initialize() {
		if (mCreateDirs) {
			std::filesystem::create_directories(mBasePath);
		}
		return std::filesystem::exists(mBasePath);
	}

	bool ImageFileIOAdapter::output(const IOFileInfo* fileInfo) {
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;
		std::error_code ec; // 用于捕获文件系统错误而不抛出异常

		// 1. 创建目录（如果需要）
		if (mCreateDirs) {
			if (!std::filesystem::create_directories(fullPath.parent_path(), ec)) {
				if (ec) {
					// 记录或处理目录创建错误
					return false;
				}
			}
		}
		bool success=dataToImage(const_cast<IOFileInfo*>(fileInfo));
		delete fileInfo;
		fileInfo = nullptr;

		return success;
	}

	bool ImageFileIOAdapter::outputBatch(const std::vector<IOFileInfo*> files) {
		// 使用路径和原始数据指针+大小的pair
		std::unordered_map<std::string, std::vector<IOFileInfo*>> dirGroups;

		// 按目录分组
		for (const auto fileInfo : files) {  // 正确解包三元组
			const auto fullPath = std::filesystem::path(mBasePath) / (fileInfo->filePath);
			dirGroups[fullPath.parent_path().string()].emplace_back(fileInfo);
		}

		// 批量处理每个目录
		for (auto& [dir, fileList] : dirGroups) {
			if (mCreateDirs) {
				std::error_code ec;
				if (!std::filesystem::create_directories(dir, ec) && ec) {
					return false;  // 目录创建失败
				}
			}

			for (auto& oneFile : fileList) {
				dataToImage(oneFile);
				delete oneFile;
				oneFile = nullptr;
			}
		}

		return true;
	}

	bool ImageFileIOAdapter::finalize() {
		return true;
	}

	bool ImageFileIOAdapter::dataToImage(IOFileInfo* ioFileInfo)
	{
		switch (mFormat)
		{
		case IMAGEFORMAT::PNG: {
			//做个波段数映射
			int depth = ioFileInfo->dataSize / (mWidth * mHeight * mBandsNum) * 8;
			return saveGDALDataAsPNG(ioFileInfo, mWidth, mHeight, mBandsNum, depth, mNoData);
			break;
		}
		case  IMAGEFORMAT::JPG: {
			if (1==mBandsNum)
			{
				return writeGrayscaleJPEG(ioFileInfo, mWidth, mHeight);
			}else if (mBandsNum>=3)
			{
				return writeRGBJPEG(ioFileInfo, mWidth, mHeight);
			}
			break;
		}
		case IMAGEFORMAT::WEBP: {
			break;
		}
		default:
			break;
		}
		return true;
	}

	bool ImageFileIOAdapter::writeJPEG(IOFileInfo* fileInfo, int width, int height, const JPEGOptions& opts) {
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;
		std::string filename = fullPath.string();
		unsigned char* data = fileInfo->data;

		FILE* outfile = fopen((filename + ".jpg").c_str(), "wb");
		if (!outfile) {
			fprintf(stderr, "无法打开输出文件: %s\n", filename.c_str());
			return false;
		}

		jpeg_compress_struct cinfo = {};
		jpeg_error_mgr jerr;

		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);
		jpeg_stdio_dest(&cinfo, outfile);

		cinfo.image_width = width;
		cinfo.image_height = height;
		cinfo.input_components = (opts.color_space == JCS_GRAYSCALE) ? 1 : 3;
		cinfo.in_color_space = opts.color_space;

		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, opts.quality, TRUE);

		if (opts.optimize_coding) {
			cinfo.optimize_coding = TRUE;
		}

		jpeg_start_compress(&cinfo, TRUE);

		const int row_stride = width * cinfo.input_components;
		JSAMPROW row_pointer[1] = {};

		while (cinfo.next_scanline < cinfo.image_height) {
			row_pointer[0] = const_cast<JSAMPROW>(
				&data[cinfo.next_scanline * row_stride]);
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}

		jpeg_finish_compress(&cinfo);
		fclose(outfile);
		jpeg_destroy_compress(&cinfo);
		return true;
	}

	bool ImageFileIOAdapter::writeRGBJPEG(IOFileInfo* fileInfo, int width, int height,int quality/* = 85*/) {
		if (fileInfo->dataSize != width * height * 3) {
			fprintf(stderr, "RGB数据大小不匹配\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height,{ quality, true, JCS_RGB });
	}

	bool ImageFileIOAdapter::writeGrayscaleJPEG(IOFileInfo* fileInfo, int width, int height, int quality /*= 85*/) {
		if (fileInfo->dataSize != width * height) {
			fprintf(stderr, "灰度数据大小不匹配\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height,{ quality, true, JCS_GRAYSCALE });
	}
	
	bool ImageFileIOAdapter::hasNodataPixels(const unsigned char* imageData, int width, int height, int bandCount, int bitDepth, const std::vector<double>& nodata)
	{
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int pixelPos = (y * width + x) * bandCount;

				// 检查每个波段是否所有值都等于nodata
				bool isNodata = true;
				for (int b = 0; b < bandCount; b++) {
					double pixelValue;
					if (bitDepth == 8) {
						pixelValue = static_cast<double>(imageData[pixelPos + b]);
					}
					else { // 16位
						unsigned short value =
							(static_cast<unsigned short>(imageData[(pixelPos + b) * 2]) << 8) |
							static_cast<unsigned short>(imageData[(pixelPos + b) * 2 + 1]);
						pixelValue = static_cast<double>(value);
					}

					if (pixelValue != nodata[b]) {
						isNodata = false;
						break;
					}
				}

				if (isNodata) {
					return true;  // 发现nodata像素，直接返回
				}
			}
		}
		return false;  // 没有发现nodata像素
	}

	bool ImageFileIOAdapter::isNodataPixel(const unsigned char* imageData, int pixelPos, int bandCount, int bitDepth, const std::vector<double>& nodata)
	{
		for (int b = 0; b < bandCount; b++) {
			double pixelValue;
			if (bitDepth == 8) {
				pixelValue = static_cast<double>(imageData[pixelPos + b]);
			}
			else { // 16位
				unsigned short value =
					(static_cast<unsigned short>(imageData[(pixelPos + b) * 2]) << 8) |
					static_cast<unsigned short>(imageData[(pixelPos + b) * 2 + 1]);
				pixelValue = static_cast<double>(value);
			}

			if (pixelValue != nodata[b]) {
				return false;  // 只要有一个波段不等于nodata值，就不是nodata像素
			}
		}
		return true;  // 所有波段都等于nodata值
	}

	bool ImageFileIOAdapter::initPNG(FILE* fp, png_structp& png_ptr, png_infop& info_ptr)
	{
		// 初始化PNG写入结构
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr) {
			std::cerr << "无法创建PNG写入结构" << std::endl;
			return false;
		}

		// 初始化PNG信息结构
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			png_destroy_write_struct(&png_ptr, NULL);
			std::cerr << "无法创建PNG信息结构" << std::endl;
			return false;
		}

		// 设置错误处理
		if (setjmp(png_jmpbuf(png_ptr))) {
			png_destroy_write_struct(&png_ptr, &info_ptr);
			std::cerr << "PNG写入错误" << std::endl;
			return false;
		}

		// 设置输出
		png_init_io(png_ptr, fp);
		return true;
	}

	void ImageFileIOAdapter::processPixelData(const unsigned char* imageData, std::vector<unsigned char>& outData, int width, int height, int bandCount, int outBandCount, int bitDepth, const std::vector<double>& nodata, bool requiresAlpha)
	{
		size_t rowSize = width * outBandCount * (bitDepth / 8);

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int inPixelPos = (y * width + x) * bandCount;  // 输入数据的像素位置
				int outPixelPos = (y * width + x) * outBandCount;  // 输出数据的像素位置

				// 检查是否为nodata，仅当需要透明通道时
				bool isNodata = false;
				if (requiresAlpha) {
					isNodata = isNodataPixel(imageData, inPixelPos, bandCount, bitDepth, nodata);
				}

				// 复制原始数据到输出数组
				if (bitDepth == 8) {
					// 8位数据处理
					for (int b = 0; b < bandCount; b++) {
						outData[outPixelPos + b] = imageData[inPixelPos + b];
					}

					// 如果需要透明通道，设置透明度
					if (requiresAlpha) {
						outData[outPixelPos + bandCount] = isNodata ? 0 : 255;
					}
				}
				else {
					// 16位数据处理
					for (int b = 0; b < bandCount; b++) {
						memcpy(&outData[y * rowSize + (outPixelPos + b) * 2],
							&imageData[inPixelPos * 2 + b * 2],
							2);
					}

					// 如果需要透明通道，设置透明度
					if (requiresAlpha) {
						unsigned short alpha = isNodata ? 0 : 65535;
						outData[y * rowSize + (outPixelPos + bandCount) * 2] = (alpha >> 8) & 0xFF;
						outData[y * rowSize + (outPixelPos + bandCount) * 2 + 1] = alpha & 0xFF;
					}
				}
			}
		}
	}

	bool ImageFileIOAdapter::saveGDALDataAsPNG(IOFileInfo* fileInfo,  int width, int height, int bandCount, int bitDepth, const std::vector<double>& nodata)
	{
		const unsigned char* imageData = fileInfo->data;
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;
		std::string outputFilename = fullPath.string()+".png";
		// 参数检查
		if (!imageData || width <= 0 || height <= 0 ||
			(bandCount != 1 && bandCount != 3) ||
			(bitDepth != 8 && bitDepth != 16) ||
			nodata.size() != bandCount) {
			std::cerr << "参数错误" << std::endl;
			return false;
		}

		// 首先扫描一遍，检查是否存在nodata值
		bool requiresAlpha = hasNodataPixels(imageData, width, height, bandCount, bitDepth, nodata);

		// 打开输出文件
		FILE* fp = fopen(outputFilename.c_str(), "wb");
		if (!fp) {
			std::cerr << "无法创建输出文件: " << outputFilename << std::endl;
			return false;
		}

		// 初始化PNG结构
		png_structp png_ptr;
		png_infop info_ptr;
		if (!initPNG(fp, png_ptr, info_ptr)) {
			fclose(fp);
			return false;
		}

		// 设置PNG头部信息
		int png_color_type;
		if (bandCount == 1) {
			png_color_type = requiresAlpha ? PNG_COLOR_TYPE_GRAY_ALPHA : PNG_COLOR_TYPE_GRAY;
		}
		else {
			png_color_type = requiresAlpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB;
		}

		png_set_IHDR(png_ptr, info_ptr, width, height, bitDepth,
			png_color_type, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		// 写入PNG头部
		png_write_info(png_ptr, info_ptr);

		// 计算每行字节数和输出的波段数
		int outBandCount = bandCount + (requiresAlpha ? 1 : 0);  // 如果有nodata，则需要透明通道
		size_t rowSize = width * outBandCount * (bitDepth / 8);

		// 创建行指针数组
		std::vector<png_bytep> rowPointers(height);

		// 申请输出图像数据内存
		std::vector<unsigned char> outData(height * rowSize);

		// 处理像素数据
		processPixelData(imageData, outData, width, height, bandCount, outBandCount,
			bitDepth, nodata, requiresAlpha);

		// 设置行指针
		for (int y = 0; y < height; y++) {
			rowPointers[y] = &outData[y * rowSize];
		}

		// 写入图像数据
		png_write_image(png_ptr, rowPointers.data());

		// 完成写入
		png_write_end(png_ptr, NULL);

		// 清理
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);

		std::cout << "PNG文件创建成功: " << outputFilename
			<< (requiresAlpha ? " (带透明通道)" : " (不带透明通道)") << std::endl;

		return true;
	}

};