#include "ImageFileParallelIOAdapter.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <limits>
#include <memory>
#include <atomic>

// GDAL库
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

// TBB库
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/task_group.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_queue.h>
#include <tbb/global_control.h>

namespace WT {

	// 任务结构体，用于异步处理
	struct ImageTask {
		IOFileInfo* fileInfo;
		std::string fullPath;
		std::promise<bool> promise;

		ImageTask(IOFileInfo* info, std::string path)
			: fileInfo(info), fullPath(std::move(path)) {}
	};

	ImageFileParallelIOAdapter::ImageFileParallelIOAdapter(const std::string& basePath, bool createDirs
		, int width /*= 256*/, int height /*= 256*/
		, std::string format /*= "png"*/, int bands/*=3*/, std::vector<double> noData/*={}*/)
		: mBasePath(basePath), mCreateDirs(createDirs)
		, mWidth(width), mHeight(height), mBandsNum(bands)
		, mNoData(noData) {

		// 设置格式
		if ("png" == format) {
			this->mFormat = IMAGEFORMAT::PNG;
		}
		else if ("jpg" == format) {
			this->mFormat = IMAGEFORMAT::JPG;
		}
		else if ("webp" == format) {
			this->mFormat = IMAGEFORMAT::WEBP;
		}
	}

	bool ImageFileParallelIOAdapter::initialize() {
		if (mCreateDirs) {
			std::filesystem::create_directories(mBasePath);
		}
		return std::filesystem::exists(mBasePath);
	}

	bool ImageFileParallelIOAdapter::output(const IOFileInfo* fileInfo) {
		// 单个文件的异步输出
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;

		// 创建任务
		auto task = std::make_shared<ImageTask>(const_cast<IOFileInfo*>(fileInfo), fullPath.string());

		// 使用task_group进行异步处理
		::tbb::task_group tg;

		tg.run([this, task]() {
			try {
				bool success = processImageTask(*task);
				task->promise.set_value(success);
			}
			catch (const std::exception& e) {
				std::cerr << "异步处理图像时发生异常: " << e.what() << std::endl;
				task->promise.set_value(false);
			}
			});

		// 等待完成并获取结果
		auto future = task->promise.get_future();
		tg.wait();

		return future.get();
	}

	bool ImageFileParallelIOAdapter::outputBatch(const std::vector<IOFileInfo*> files) {
		if (files.empty()) {
			return true;
		}

		// 使用原子计数器跟踪成功/失败
		std::atomic<int> successCount{0};
		std::atomic<int> failureCount{0};

		// 按目录分组（保持原有逻辑）
		std::unordered_map<std::string, std::vector<IOFileInfo*>> dirGroups;
		for (const auto fileInfo : files) {
			const auto fullPath = std::filesystem::path(mBasePath) / (fileInfo->filePath);
			dirGroups[fullPath.parent_path().string()].emplace_back(fileInfo);
		}

		// 并行处理每个目录组
		::tbb::parallel_for_each(dirGroups.begin(), dirGroups.end(),
			[this, &successCount, &failureCount](auto& dirGroup) {
				const auto& [dir, fileList] = dirGroup;

				// 创建目录（线程安全）
				if (mCreateDirs) {
					std::error_code ec;
					std::filesystem::create_directories(dir, ec);
					if (ec) {
						std::cerr << "创建目录失败: " << dir << " - " << ec.message() << std::endl;
						failureCount += fileList.size();
						// 清理内存
						for (auto& file : fileList) {
							delete file;
						}
						return;
					}
				}

				// 并行处理目录内的文件
				::tbb::parallel_for_each(fileList.begin(), fileList.end(),
					[this, &successCount, &failureCount](IOFileInfo* fileInfo) {
						try {
							const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;
							ImageTask task(fileInfo, fullPath.string());

							bool success = processImageTask(task);
							if (success) {
								successCount++;
							}
							else {
								failureCount++;
							}
						}
						catch (const std::exception& e) {
							std::cerr << "处理文件时发生异常: " << e.what() << std::endl;
							failureCount++;
						}

						// 清理内存
						delete fileInfo;
					});
			});

		std::cout << "批量处理完成 - 成功: " << successCount.load()
			<< ", 失败: " << failureCount.load() << std::endl;

		return failureCount.load() == 0;
	}

	bool ImageFileParallelIOAdapter::outputBatchAsync(const std::vector<IOFileInfo*> files) {
		if (files.empty()) {
			return true;
		}

		// 创建任务队列
		::tbb::concurrent_queue<std::shared_ptr<ImageTask>> taskQueue;
		std::vector<std::future<bool>> futures;
		futures.reserve(files.size());

		// 将所有文件转换为任务并加入队列
		for (const auto& fileInfo : files) {
			const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;
			auto task = std::make_shared<ImageTask>(fileInfo, fullPath.string());
			futures.push_back(task->promise.get_future());
			taskQueue.push(task);
		}

		// 使用task_group异步处理所有任务
		::tbb::task_group tg;

		// 创建工作线程
		const int numWorkers = std::min(static_cast<int>(files.size()),
			static_cast<int>(std::thread::hardware_concurrency()));

		for (int i = 0; i < numWorkers; ++i) {
			tg.run([this, &taskQueue]() {
				std::shared_ptr<ImageTask> task;
				while (taskQueue.try_pop(task)) {
					try {
						bool success = processImageTask(*task);
						task->promise.set_value(success);
					}
					catch (const std::exception& e) {
						std::cerr << "异步处理图像时发生异常: " << e.what() << std::endl;
						task->promise.set_value(false);
					}
				}
				});
		}

		// 等待所有任务完成
		tg.wait();

		// 收集结果
		bool allSuccess = true;
		int successCount = 0;
		for (auto& future : futures) {
			if (future.get()) {
				successCount++;
			}
			else {
				allSuccess = false;
			}
		}

		std::cout << "异步批量处理完成 - 成功: " << successCount
			<< "/" << files.size() << std::endl;

		return allSuccess;
	}

	bool ImageFileParallelIOAdapter::finalize() {
		return true;
	}

	// 私有方法：处理单个图像任务
	bool ImageFileParallelIOAdapter::processImageTask(const ImageTask& task) {
		// 创建目录（如果需要）
		if (mCreateDirs) {
			std::error_code ec;
			const auto parentPath = std::filesystem::path(task.fullPath).parent_path();
			if (!std::filesystem::create_directories(parentPath, ec) && ec) {
				std::cerr << "创建目录失败: " << parentPath << " - " << ec.message() << std::endl;
				delete task.fileInfo;
				return false;
			}
		}

		// 处理图像数据
		bool success = dataToImage(task.fileInfo);

		// 清理内存
		delete task.fileInfo;

		return success;
	}

	bool ImageFileParallelIOAdapter::dataToImage(IOFileInfo* ioFileInfo) {
		switch (mFormat) {
		case IMAGEFORMAT::PNG: {
			int bandCount = ioFileInfo->dataSize / (mWidth * mHeight );
			return saveGDALDataAsPNG(ioFileInfo, mWidth, mHeight, bandCount);
		}
		case IMAGEFORMAT::JPG: {
			if (1 == mBandsNum) {
				return writeGrayscaleJPEG(ioFileInfo, mWidth, mHeight);
			}
			else if (mBandsNum >= 3) {
				return writeRGBJPEG(ioFileInfo, mWidth, mHeight);
			}
			break;
		}
		case IMAGEFORMAT::WEBP: {
			// TODO: 实现WEBP格式支持
			break;
		}
		default:
			break;
		}
		return true;
	}

	// PNG处理优化：并行处理像素数据
	void ImageFileParallelIOAdapter::processPixelDataParallel(const unsigned char* imageData,
		std::vector<unsigned char>& outData,
		int width, int height, int bandCount,
		int outBandCount, int bitDepth,
		const std::vector<double>& nodata,
		bool requiresAlpha) {
		const size_t rowSize = width * outBandCount * (bitDepth / 8);

		// 并行处理每一行
		::tbb::parallel_for(::tbb::blocked_range<int>(0, height),
			[&](const ::tbb::blocked_range<int>& range) {
				for (int y = range.begin(); y != range.end(); ++y) {
					for (int x = 0; x < width; x++) {
						int inPixelPos = (y * width + x) * bandCount;
						int outPixelPos = (y * width + x) * outBandCount;

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
									&imageData[inPixelPos * 2 + b * 2], 2);
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
			});
	}

	// nodata检测优化：并行扫描
	bool ImageFileParallelIOAdapter::hasNodataPixelsParallel(const unsigned char* imageData,
		int width, int height, int bandCount,
		int bitDepth, const std::vector<double>& nodata) {
		std::atomic<bool> foundNodata{false};

		::tbb::parallel_for(::tbb::blocked_range<int>(0, height),
			[&](const ::tbb::blocked_range<int>& range) {
				if (foundNodata.load()) return; // 早期退出

				for (int y = range.begin(); y != range.end() && !foundNodata.load(); ++y) {
					for (int x = 0; x < width && !foundNodata.load(); x++) {
						int pixelPos = (y * width + x) * bandCount;

						if (isNodataPixel(imageData, pixelPos, bandCount, bitDepth, nodata)) {
							foundNodata.store(true);
							return;
						}
					}
				}
			});

		return foundNodata.load();
	}

	// 修改saveGDALDataAsPNG函数以使用并行处理
	bool ImageFileParallelIOAdapter::saveGDALDataAsPNG(IOFileInfo* fileInfo, int width, int height,int bandCount) {
		unsigned char* imageData = fileInfo->data;
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;
		std::string outputFilename = fullPath.string() + ".png";

		// 参数检查
		if (!imageData || width <= 0 || height <= 0 ||
			bandCount<1||bandCount>4) {
			std::cerr << "参数错误" << std::endl;
			return false;
		}

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
			png_color_type = PNG_COLOR_TYPE_GRAY;
		}
		else if (2 == bandCount) {
			png_color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
		}
		else if (3 == bandCount) {
			png_color_type =  PNG_COLOR_TYPE_RGB;
		}
		else {
			png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
		}

		png_set_IHDR(png_ptr, info_ptr, width, height, 8,
			png_color_type, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		// 写入PNG头部
		png_write_info(png_ptr, info_ptr);

		// 计算每行字节数和输出的波段数
		size_t rowSize = width * bandCount;

		// 创建行指针数组
		std::vector<png_bytep> rowPointers(height);

		// 申请输出图像数据内存
		std::vector<unsigned char> outData(height * rowSize);

		// 设置行指针
		for (int y = 0; y < height; y++) {
			rowPointers[y] = &imageData[y * rowSize];
		}

		// 写入图像数据
		png_write_image(png_ptr, rowPointers.data());

		// 完成写入
		png_write_end(png_ptr, NULL);

		// 清理
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);

		std::cout << "PNG文件创建成功: " << outputFilename
			<< ((bandCount == 4 || bandCount == 2) ? " (带透明通道)" : " (不带透明通道)") << std::endl;

		return true;
	}

	// 保留原有的其他方法实现...
	// writeJPEG, writeRGBJPEG, writeGrayscaleJPEG, isNodataPixel, initPNG 等函数
	// 这些函数保持不变，只需要包含在完整的实现中

	bool ImageFileParallelIOAdapter::writeJPEG(IOFileInfo* fileInfo, int width, int height, const JPEGOptions& opts) {
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

	bool ImageFileParallelIOAdapter::writeRGBJPEG(IOFileInfo* fileInfo, int width, int height, int quality/* = 85*/) {
		if (fileInfo->dataSize != width * height * 3) {
			fprintf(stderr, "RGB数据大小不匹配\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height, { quality, true, JCS_RGB });
	}

	bool ImageFileParallelIOAdapter::writeGrayscaleJPEG(IOFileInfo* fileInfo, int width, int height, int quality /*= 85*/) {
		if (fileInfo->dataSize != width * height) {
			fprintf(stderr, "灰度数据大小不匹配\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height, { quality, true, JCS_GRAYSCALE });
	}

	bool ImageFileParallelIOAdapter::hasNodataPixels(const unsigned char* imageData, int width, int height, int bandCount, int bitDepth, const std::vector<double>& nodata)
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

	bool ImageFileParallelIOAdapter::isNodataPixel(const unsigned char* imageData, int pixelPos, int bandCount, int bitDepth, const std::vector<double>& nodata)
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

	bool ImageFileParallelIOAdapter::initPNG(FILE* fp, png_structp& png_ptr, png_infop& info_ptr)
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

	void ImageFileParallelIOAdapter::processPixelData(const unsigned char* imageData, std::vector<unsigned char>& outData, int width, int height, int bandCount, int outBandCount, int bitDepth, const std::vector<double>& nodata, bool requiresAlpha)
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
		
};