#include "ImageFileIOAdapter.h"
#include <fstream>
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

	bool ImageFileIOAdapter::output(const IOFileInfo fileInfo) {
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo.filePath;
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

		// 2. 打开文件
		std::ofstream out(fullPath, std::ios::binary);
		if (!out.is_open()) {
			// 可以记录具体错误信息
			return false;
		}

		// 3. 写入数据
		out.write(reinterpret_cast<const char*> (fileInfo.data),fileInfo.dataSize);

		// 4. 确保所有操作成功
		const bool success = out.good();
		out.close(); // 显式关闭（析构函数会自动调用，但显式调用可以立即检查错误）

		return success;
	}

	bool ImageFileIOAdapter::outputBatch(const std::vector<IOFileInfo> files) {
		// 使用路径和原始数据指针+大小的pair
		std::unordered_map<std::string, std::vector<IOFileInfo>> dirGroups;

		// 按目录分组
		for (const auto& fileInfo : files) {  // 正确解包三元组
			const auto fullPath = std::filesystem::path(mBasePath) / (fileInfo.filePath);
			dirGroups[fullPath.parent_path().string()].emplace_back(fileInfo);
		}

		// 批量处理每个目录
		for (const auto& [dir, fileList] : dirGroups) {
			if (mCreateDirs) {
				std::error_code ec;
				if (!std::filesystem::create_directories(dir, ec) && ec) {
					return false;  // 目录创建失败
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
			//做个波段数映射
			std::vector<int>colorTypeMap = {0,PNG_COLOR_TYPE_GRAY ,PNG_COLOR_TYPE_GA ,PNG_COLOR_TYPE_RGB,PNG_COLOR_TYPE_RGBA };			
			int depth=ioFileInfo.dataSize / (mWidth * mHeight * mBandsNum) * 8;

			//要看看是否是需要添加透明图层
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

	bool ImageFileIOAdapter::writeRGBJPEG(IOFileInfo fileInfo, int width, int height,int quality/* = 85*/) {
		if (fileInfo.dataSize != width * height * 3) {
			fprintf(stderr, "RGB数据大小不匹配\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height,{ quality, true, JCS_RGB });
	}

	bool ImageFileIOAdapter::writeGrayscaleJPEG(IOFileInfo fileInfo, int width, int height, int quality /*= 85*/) {
		if (fileInfo.dataSize != width * height) {
			fprintf(stderr, "灰度数据大小不匹配\n");
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
			//throw std::runtime_error("无法打开文件: " + filename);
		}

		png_structp png_ptr = png_create_write_struct(
			PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png_ptr) {
			fclose(fp);
			return false;
			//throw std::runtime_error("png_create_write_struct失败");
		}

		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			png_destroy_write_struct(&png_ptr, nullptr);
			fclose(fp);
			return false;
			//throw std::runtime_error("png_create_info_struct失败");
		}

		if (setjmp(png_jmpbuf(png_ptr))) {
			png_destroy_write_struct(&png_ptr, &info_ptr);
			fclose(fp);
			return false;
			//throw std::runtime_error("PNG写入过程中出错");
		}

		png_init_io(png_ptr, fp);

		// 设置图像头信息
		png_set_IHDR(png_ptr, info_ptr, width, height,
			bit_depth, color_type, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

		png_write_info(png_ptr, info_ptr);

		// 准备行指针
		std::vector<png_bytep> row_pointers(height);

		for (int y = 0; y < height; ++y) {
			row_pointers[y] = const_cast<png_bytep>(image_data + y * width * bands);
		}

		png_write_image(png_ptr, row_pointers.data());
		png_write_end(png_ptr, nullptr);

		// 清理
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return true;		
	}

	bool ImageFileIOAdapter::nodataCheckAndTrans(unsigned char* oriData, int oriDataSize, unsigned char* newData, int& newDataSize) {
		if (mNoData.size() == 0) return false;
		
		//oriData是rgb rgb这样的形式了 
		//同时需要考虑int8 和int16这样的形式
		int dataType=oriDataSize / (mWidth * mHeight * mBandsNum);
		newDataSize = (oriDataSize / mBandsNum) * (mBandsNum + 1);

		//需要先检查
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

			//真正需要透明的一定比不透明的瓦片少 所以分开是合适的
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
						//写新的那个波段 但是要根据是否为透明决定						
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

			//真正需要透明的一定比不透明的瓦片少 所以分开是合适的
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
						//写新的那个波段 但是要根据是否为透明决定						
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