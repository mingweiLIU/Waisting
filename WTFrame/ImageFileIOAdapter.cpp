#include "ImageFileIOAdapter.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <limits>

// GDAL��
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
		std::error_code ec; // ���ڲ����ļ�ϵͳ��������׳��쳣

		// 1. ����Ŀ¼�������Ҫ��
		if (mCreateDirs) {
			if (!std::filesystem::create_directories(fullPath.parent_path(), ec)) {
				if (ec) {
					// ��¼����Ŀ¼��������
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
		// ʹ��·����ԭʼ����ָ��+��С��pair
		std::unordered_map<std::string, std::vector<IOFileInfo*>> dirGroups;

		// ��Ŀ¼����
		for (const auto fileInfo : files) {  // ��ȷ�����Ԫ��
			const auto fullPath = std::filesystem::path(mBasePath) / (fileInfo->filePath);
			dirGroups[fullPath.parent_path().string()].emplace_back(fileInfo);
		}

		// ��������ÿ��Ŀ¼
		for (auto& [dir, fileList] : dirGroups) {
			if (mCreateDirs) {
				std::error_code ec;
				if (!std::filesystem::create_directories(dir, ec) && ec) {
					return false;  // Ŀ¼����ʧ��
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
			//����������ӳ��
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
			fprintf(stderr, "�޷�������ļ�: %s\n", filename.c_str());
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
			fprintf(stderr, "RGB���ݴ�С��ƥ��\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height,{ quality, true, JCS_RGB });
	}

	bool ImageFileIOAdapter::writeGrayscaleJPEG(IOFileInfo* fileInfo, int width, int height, int quality /*= 85*/) {
		if (fileInfo->dataSize != width * height) {
			fprintf(stderr, "�Ҷ����ݴ�С��ƥ��\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height,{ quality, true, JCS_GRAYSCALE });
	}
	
	bool ImageFileIOAdapter::hasNodataPixels(const unsigned char* imageData, int width, int height, int bandCount, int bitDepth, const std::vector<double>& nodata)
	{
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int pixelPos = (y * width + x) * bandCount;

				// ���ÿ�������Ƿ�����ֵ������nodata
				bool isNodata = true;
				for (int b = 0; b < bandCount; b++) {
					double pixelValue;
					if (bitDepth == 8) {
						pixelValue = static_cast<double>(imageData[pixelPos + b]);
					}
					else { // 16λ
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
					return true;  // ����nodata���أ�ֱ�ӷ���
				}
			}
		}
		return false;  // û�з���nodata����
	}

	bool ImageFileIOAdapter::isNodataPixel(const unsigned char* imageData, int pixelPos, int bandCount, int bitDepth, const std::vector<double>& nodata)
	{
		for (int b = 0; b < bandCount; b++) {
			double pixelValue;
			if (bitDepth == 8) {
				pixelValue = static_cast<double>(imageData[pixelPos + b]);
			}
			else { // 16λ
				unsigned short value =
					(static_cast<unsigned short>(imageData[(pixelPos + b) * 2]) << 8) |
					static_cast<unsigned short>(imageData[(pixelPos + b) * 2 + 1]);
				pixelValue = static_cast<double>(value);
			}

			if (pixelValue != nodata[b]) {
				return false;  // ֻҪ��һ�����β�����nodataֵ���Ͳ���nodata����
			}
		}
		return true;  // ���в��ζ�����nodataֵ
	}

	bool ImageFileIOAdapter::initPNG(FILE* fp, png_structp& png_ptr, png_infop& info_ptr)
	{
		// ��ʼ��PNGд��ṹ
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr) {
			std::cerr << "�޷�����PNGд��ṹ" << std::endl;
			return false;
		}

		// ��ʼ��PNG��Ϣ�ṹ
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			png_destroy_write_struct(&png_ptr, NULL);
			std::cerr << "�޷�����PNG��Ϣ�ṹ" << std::endl;
			return false;
		}

		// ���ô�����
		if (setjmp(png_jmpbuf(png_ptr))) {
			png_destroy_write_struct(&png_ptr, &info_ptr);
			std::cerr << "PNGд�����" << std::endl;
			return false;
		}

		// �������
		png_init_io(png_ptr, fp);
		return true;
	}

	void ImageFileIOAdapter::processPixelData(const unsigned char* imageData, std::vector<unsigned char>& outData, int width, int height, int bandCount, int outBandCount, int bitDepth, const std::vector<double>& nodata, bool requiresAlpha)
	{
		size_t rowSize = width * outBandCount * (bitDepth / 8);

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int inPixelPos = (y * width + x) * bandCount;  // �������ݵ�����λ��
				int outPixelPos = (y * width + x) * outBandCount;  // ������ݵ�����λ��

				// ����Ƿ�Ϊnodata��������Ҫ͸��ͨ��ʱ
				bool isNodata = false;
				if (requiresAlpha) {
					isNodata = isNodataPixel(imageData, inPixelPos, bandCount, bitDepth, nodata);
				}

				// ����ԭʼ���ݵ��������
				if (bitDepth == 8) {
					// 8λ���ݴ���
					for (int b = 0; b < bandCount; b++) {
						outData[outPixelPos + b] = imageData[inPixelPos + b];
					}

					// �����Ҫ͸��ͨ��������͸����
					if (requiresAlpha) {
						outData[outPixelPos + bandCount] = isNodata ? 0 : 255;
					}
				}
				else {
					// 16λ���ݴ���
					for (int b = 0; b < bandCount; b++) {
						memcpy(&outData[y * rowSize + (outPixelPos + b) * 2],
							&imageData[inPixelPos * 2 + b * 2],
							2);
					}

					// �����Ҫ͸��ͨ��������͸����
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
		// �������
		if (!imageData || width <= 0 || height <= 0 ||
			(bandCount != 1 && bandCount != 3) ||
			(bitDepth != 8 && bitDepth != 16) ||
			nodata.size() != bandCount) {
			std::cerr << "��������" << std::endl;
			return false;
		}

		// ����ɨ��һ�飬����Ƿ����nodataֵ
		bool requiresAlpha = hasNodataPixels(imageData, width, height, bandCount, bitDepth, nodata);

		// ������ļ�
		FILE* fp = fopen(outputFilename.c_str(), "wb");
		if (!fp) {
			std::cerr << "�޷���������ļ�: " << outputFilename << std::endl;
			return false;
		}

		// ��ʼ��PNG�ṹ
		png_structp png_ptr;
		png_infop info_ptr;
		if (!initPNG(fp, png_ptr, info_ptr)) {
			fclose(fp);
			return false;
		}

		// ����PNGͷ����Ϣ
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

		// д��PNGͷ��
		png_write_info(png_ptr, info_ptr);

		// ����ÿ���ֽ���������Ĳ�����
		int outBandCount = bandCount + (requiresAlpha ? 1 : 0);  // �����nodata������Ҫ͸��ͨ��
		size_t rowSize = width * outBandCount * (bitDepth / 8);

		// ������ָ������
		std::vector<png_bytep> rowPointers(height);

		// �������ͼ�������ڴ�
		std::vector<unsigned char> outData(height * rowSize);

		// ������������
		processPixelData(imageData, outData, width, height, bandCount, outBandCount,
			bitDepth, nodata, requiresAlpha);

		// ������ָ��
		for (int y = 0; y < height; y++) {
			rowPointers[y] = &outData[y * rowSize];
		}

		// д��ͼ������
		png_write_image(png_ptr, rowPointers.data());

		// ���д��
		png_write_end(png_ptr, NULL);

		// ����
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);

		std::cout << "PNG�ļ������ɹ�: " << outputFilename
			<< (requiresAlpha ? " (��͸��ͨ��)" : " (����͸��ͨ��)") << std::endl;

		return true;
	}

};