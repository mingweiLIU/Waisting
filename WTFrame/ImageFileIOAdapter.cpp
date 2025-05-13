#include "ImageFileIOAdapter.h"
#include <fstream>
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

	bool ImageFileIOAdapter::output(const IOFileInfo fileInfo) {
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo.filePath;
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

		// 2. ���ļ�
		std::ofstream out(fullPath, std::ios::binary);
		if (!out.is_open()) {
			// ���Լ�¼���������Ϣ
			return false;
		}

		// 3. д������
		out.write(reinterpret_cast<const char*> (fileInfo.data),fileInfo.dataSize);

		// 4. ȷ�����в����ɹ�
		const bool success = out.good();
		out.close(); // ��ʽ�رգ������������Զ����ã�����ʽ���ÿ�������������

		return success;
	}

	bool ImageFileIOAdapter::outputBatch(const std::vector<IOFileInfo> files) {
		// ʹ��·����ԭʼ����ָ��+��С��pair
		std::unordered_map<std::string, std::vector<IOFileInfo>> dirGroups;

		// ��Ŀ¼����
		for (const auto& fileInfo : files) {  // ��ȷ�����Ԫ��
			const auto fullPath = std::filesystem::path(mBasePath) / (fileInfo.filePath);
			dirGroups[fullPath.parent_path().string()].emplace_back(fileInfo);
		}

		// ��������ÿ��Ŀ¼
		for (const auto& [dir, fileList] : dirGroups) {
			if (mCreateDirs) {
				std::error_code ec;
				if (!std::filesystem::create_directories(dir, ec) && ec) {
					return false;  // Ŀ¼����ʧ��
				}
			}

			for (auto& oneFile : fileList) {
				dataToImage(oneFile);
			}
		}

		return true;
	}

	bool ImageFileIOAdapter::finalize() {
		return true;
	}

	bool ImageFileIOAdapter::dataToImage(IOFileInfo ioFileInfo)
	{
		switch (mFormat)
		{
		case IMAGEFORMAT::PNG: {
			//����������ӳ��
			std::vector<int>colorTypeMap = {0,PNG_COLOR_TYPE_GRAY ,PNG_COLOR_TYPE_GA ,PNG_COLOR_TYPE_RGB,PNG_COLOR_TYPE_RGBA };			
			int depth=ioFileInfo.dataSize / (mWidth * mHeight * mBandsNum) * 8;

			//Ҫ�����Ƿ�����Ҫ���͸��ͼ��
			unsigned char* tempNewData = nullptr;
			int tempNewDataSize = 0;
			if (nodataCheckAndTrans(ioFileInfo.data, ioFileInfo.dataSize, tempNewData, tempNewDataSize)) {
				free(ioFileInfo.data);
				ioFileInfo.data = tempNewData;
				ioFileInfo.dataSize = tempNewDataSize;
				return write_png(ioFileInfo, mWidth, mHeight, mBandsNum + 1, colorTypeMap[mBandsNum+1], depth);
			}

			return write_png(ioFileInfo, mWidth, mHeight, mBandsNum,colorTypeMap[mBandsNum],depth);
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

	bool ImageFileIOAdapter::writeJPEG(IOFileInfo fileInfo, int width, int height, const JPEGOptions& opts) {
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo.filePath;
		std::string filename = fullPath.string();
		unsigned char* data = fileInfo.data;

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

	bool ImageFileIOAdapter::writeRGBJPEG(IOFileInfo fileInfo, int width, int height,int quality/* = 85*/) {
		if (fileInfo.dataSize != width * height * 3) {
			fprintf(stderr, "RGB���ݴ�С��ƥ��\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height,{ quality, true, JCS_RGB });
	}

	bool ImageFileIOAdapter::writeGrayscaleJPEG(IOFileInfo fileInfo, int width, int height, int quality /*= 85*/) {
		if (fileInfo.dataSize != width * height) {
			fprintf(stderr, "�Ҷ����ݴ�С��ƥ��\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height,{ quality, true, JCS_GRAYSCALE });
	}
	
	bool ImageFileIOAdapter::write_png(IOFileInfo fileInfo, int width, int height,int bands, int color_type /*= PNG_COLOR_TYPE_RGB*/, int bit_depth /*= 8*/)
	{
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo.filePath;
		std::string filename = fullPath.string();
		const png_byte* image_data = fileInfo.data;

		FILE* fp = fopen((filename + ".png").c_str(), "wb");
		if (!fp) {
			return false;
			//throw std::runtime_error("�޷����ļ�: " + filename);
		}

		png_structp png_ptr = png_create_write_struct(
			PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png_ptr) {
			fclose(fp);
			return false;
			//throw std::runtime_error("png_create_write_structʧ��");
		}

		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			png_destroy_write_struct(&png_ptr, nullptr);
			fclose(fp);
			return false;
			//throw std::runtime_error("png_create_info_structʧ��");
		}

		if (setjmp(png_jmpbuf(png_ptr))) {
			png_destroy_write_struct(&png_ptr, &info_ptr);
			fclose(fp);
			return false;
			//throw std::runtime_error("PNGд������г���");
		}

		png_init_io(png_ptr, fp);

		// ����ͼ��ͷ��Ϣ
		png_set_IHDR(png_ptr, info_ptr, width, height,
			bit_depth, color_type, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

		png_write_info(png_ptr, info_ptr);

		// ׼����ָ��
		std::vector<png_bytep> row_pointers(height);

		for (int y = 0; y < height; ++y) {
			row_pointers[y] = const_cast<png_bytep>(image_data + y * width * bands);
		}

		png_write_image(png_ptr, row_pointers.data());
		png_write_end(png_ptr, nullptr);

		// ����
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return true;		
	}

	bool ImageFileIOAdapter::nodataCheckAndTrans(unsigned char* oriData, int oriDataSize, unsigned char* newData, int& newDataSize) {
		if (mNoData.size() == 0) return false;
		
		//oriData��rgb rgb��������ʽ�� 
		//ͬʱ��Ҫ����int8 ��int16��������ʽ
		int dataType=oriDataSize / (mWidth * mHeight * mBandsNum);
		newDataSize = (oriDataSize / mBandsNum) * (mBandsNum + 1);

		//��Ҫ�ȼ��
		bool needTrans = false;
		if (1 == dataType) {
			uint8_t* data = static_cast<uint8_t*>(oriData);
			for (int j = 0; j < mWidth; j++)
			{
				for (int k = 0; k < mHeight; k++)
				{
					int cellPos=mWidth* j + k;
					bool cellIsNodata = true;
					for (int i = 0; i < mBandsNum; i++)
					{
						cellIsNodata = cellIsNodata && (data[cellPos*mBandsNum + i] == mNoData[i]);
					}
					if (cellIsNodata)
					{
						needTrans = true;
						break;
					}
				}
				if (needTrans) break;
			}

			//������Ҫ͸����һ���Ȳ�͸������Ƭ�� ���Էֿ��Ǻ��ʵ�
			if (needTrans)
			{
				newData = (unsigned char* )malloc(newDataSize);
				for (int i = 0; i < mWidth; i++)
				{
					for (int j = 0; j < mHeight; j++)
					{
						int cellPos = mWidth * i + j;
						bool cellIsNodata = true;
						for (int k = 0; k < mBandsNum; k++)
						{
							cellIsNodata = cellIsNodata && (data[cellPos * mBandsNum + k] == mNoData[k]);
							*(newData + cellPos * (mBandsNum + 1)+k) = *(data + cellPos * mBandsNum+k);
						}
						//д�µ��Ǹ����� ����Ҫ�����Ƿ�Ϊ͸������						
						*(newData + cellPos * (mBandsNum + 1) + mBandsNum) = cellIsNodata ? 0 : 255;
					}
				}
			}
		}else if(2==dataType) {
			uint16_t* data = reinterpret_cast<uint16_t*>(oriData);
			for (int j = 0; j < mWidth; j++)
			{
				for (int k = 0; k < mHeight; k++)
				{
					int cellPos = mWidth * j + k;
					bool cellIsNodata = true;
					for (int i = 0; i < mBandsNum; i++)
					{
						cellIsNodata = cellIsNodata && (data[cellPos * mBandsNum + i] == mNoData[i]);
					}
					if (cellIsNodata)
					{
						needTrans = true;
						break;
					}
				}
				if (needTrans) break;
			}

			//������Ҫ͸����һ���Ȳ�͸������Ƭ�� ���Էֿ��Ǻ��ʵ�
			if (needTrans)
			{
				uint16_t*  newDataTemp = (uint16_t*)malloc(newDataSize);
				for (int i = 0; i < mWidth; i++)
				{
					for (int j = 0; j < mHeight; j++)
					{
						int cellPos = mWidth * i + j;
						bool cellIsNodata = true;
						for (int k = 0; k < mBandsNum; k++)
						{
							cellIsNodata = cellIsNodata && (data[cellPos * mBandsNum + i] == mNoData[i]);
							*(newDataTemp + cellPos * (mBandsNum + 1) + k) = *(data + cellPos * mBandsNum + k);
						}
						//д�µ��Ǹ����� ����Ҫ�����Ƿ�Ϊ͸������						
						*(newDataTemp + cellPos * (mBandsNum + 1) + mBandsNum) = cellIsNodata ? 0 : std::numeric_limits<uint16_t>::max();
					}
				}
				newData = (unsigned char*)malloc(newDataSize);
				std::memmove(newData, newDataTemp, newDataSize);
				free(newDataTemp);
			}
		}
		return needTrans;
	}
};