#include "ImageFileParallelIOAdapter.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <limits>
#include <memory>
#include <atomic>

// GDAL��
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_vsi.h"

// TBB��
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/task_group.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_queue.h>
#include <tbb/global_control.h>

namespace WT {

	// ����ṹ�壬�����첽����
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

		// ���ø�ʽ
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
		// �����ļ����첽���
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;

		// ��������
		auto task = std::make_shared<ImageTask>(const_cast<IOFileInfo*>(fileInfo), fullPath.string());

		// ʹ��task_group�����첽����
		::tbb::task_group tg;

		tg.run([this, task]() {
			try {
				bool success = processImageTask(*task);
				task->promise.set_value(success);
			}
			catch (const std::exception& e) {
				std::cerr << "�첽����ͼ��ʱ�����쳣: " << e.what() << std::endl;
				task->promise.set_value(false);
			}
			});

		// �ȴ���ɲ���ȡ���
		auto future = task->promise.get_future();
		tg.wait();

		return future.get();
	}

	bool ImageFileParallelIOAdapter::outputBatch(const std::vector<IOFileInfo*> files) {
		if (files.empty()) {
			return true;
		}

		// ʹ��ԭ�Ӽ��������ٳɹ�/ʧ��
		std::atomic<int> successCount{0};
		std::atomic<int> failureCount{0};

		// ��Ŀ¼���飨����ԭ���߼���
		std::unordered_map<std::string, std::vector<IOFileInfo*>> dirGroups;
		for (const auto fileInfo : files) {
			const auto fullPath = std::filesystem::path(mBasePath) / (fileInfo->filePath);
			dirGroups[fullPath.parent_path().string()].emplace_back(fileInfo);
		}

		// ���д���ÿ��Ŀ¼��
		::tbb::parallel_for_each(dirGroups.begin(), dirGroups.end(),
			[this, &successCount, &failureCount](auto& dirGroup) {
				const auto& [dir, fileList] = dirGroup;

				// ����Ŀ¼���̰߳�ȫ��
				if (mCreateDirs) {
					std::error_code ec;
					std::filesystem::create_directories(dir, ec);
					if (ec) {
						std::cerr << "����Ŀ¼ʧ��: " << dir << " - " << ec.message() << std::endl;
						failureCount += fileList.size();
						// �����ڴ�
						for (auto& file : fileList) {
							delete file;
						}
						return;
					}
				}

				// ���д���Ŀ¼�ڵ��ļ�
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
							std::cerr << "�����ļ�ʱ�����쳣: " << e.what() << std::endl;
							failureCount++;
						}

						// �����ڴ�
						delete fileInfo;
					});
			});

		std::cout << "����������� - �ɹ�: " << successCount.load()
			<< ", ʧ��: " << failureCount.load() << std::endl;

		return failureCount.load() == 0;
	}

	bool ImageFileParallelIOAdapter::outputBatchAsync(const std::vector<IOFileInfo*> files) {
		if (files.empty()) {
			return true;
		}

		// �����������
		::tbb::concurrent_queue<std::shared_ptr<ImageTask>> taskQueue;
		std::vector<std::future<bool>> futures;
		futures.reserve(files.size());

		// �������ļ�ת��Ϊ���񲢼������
		for (const auto& fileInfo : files) {
			const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;
			auto task = std::make_shared<ImageTask>(fileInfo, fullPath.string());
			futures.push_back(task->promise.get_future());
			taskQueue.push(task);
		}

		// ʹ��task_group�첽������������
		::tbb::task_group tg;

		// ���������߳�
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
						std::cerr << "�첽����ͼ��ʱ�����쳣: " << e.what() << std::endl;
						task->promise.set_value(false);
					}
				}
				});
		}

		// �ȴ������������
		tg.wait();

		// �ռ����
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

		std::cout << "�첽����������� - �ɹ�: " << successCount
			<< "/" << files.size() << std::endl;

		return allSuccess;
	}

	bool ImageFileParallelIOAdapter::finalize() {
		return true;
	}

	// ˽�з�����������ͼ������
	bool ImageFileParallelIOAdapter::processImageTask(const ImageTask& task) {
		// ����Ŀ¼�������Ҫ��
		if (mCreateDirs) {
			std::error_code ec;
			const auto parentPath = std::filesystem::path(task.fullPath).parent_path();
			if (!std::filesystem::create_directories(parentPath, ec) && ec) {
				std::cerr << "����Ŀ¼ʧ��: " << parentPath << " - " << ec.message() << std::endl;
				delete task.fileInfo;
				return false;
			}
		}

		// ����ͼ������
		bool success = dataToImage(task.fileInfo);

		// �����ڴ�
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
			// TODO: ʵ��WEBP��ʽ֧��
			break;
		}
		default:
			break;
		}
		return true;
	}

	// PNG�����Ż������д�����������
	void ImageFileParallelIOAdapter::processPixelDataParallel(const unsigned char* imageData,
		std::vector<unsigned char>& outData,
		int width, int height, int bandCount,
		int outBandCount, int bitDepth,
		const std::vector<double>& nodata,
		bool requiresAlpha) {
		const size_t rowSize = width * outBandCount * (bitDepth / 8);

		// ���д���ÿһ��
		::tbb::parallel_for(::tbb::blocked_range<int>(0, height),
			[&](const ::tbb::blocked_range<int>& range) {
				for (int y = range.begin(); y != range.end(); ++y) {
					for (int x = 0; x < width; x++) {
						int inPixelPos = (y * width + x) * bandCount;
						int outPixelPos = (y * width + x) * outBandCount;

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
									&imageData[inPixelPos * 2 + b * 2], 2);
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
			});
	}

	// nodata����Ż�������ɨ��
	bool ImageFileParallelIOAdapter::hasNodataPixelsParallel(const unsigned char* imageData,
		int width, int height, int bandCount,
		int bitDepth, const std::vector<double>& nodata) {
		std::atomic<bool> foundNodata{false};

		::tbb::parallel_for(::tbb::blocked_range<int>(0, height),
			[&](const ::tbb::blocked_range<int>& range) {
				if (foundNodata.load()) return; // �����˳�

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

	// �޸�saveGDALDataAsPNG������ʹ�ò��д���
	bool ImageFileParallelIOAdapter::saveGDALDataAsPNG(IOFileInfo* fileInfo, int width, int height,int bandCount) {
		unsigned char* imageData = fileInfo->data;
		const auto fullPath = std::filesystem::path(mBasePath) / fileInfo->filePath;
		std::string outputFilename = fullPath.string() + ".png";

		// �������
		if (!imageData || width <= 0 || height <= 0 ||
			bandCount<1||bandCount>4) {
			std::cerr << "��������" << std::endl;
			return false;
		}

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

		// д��PNGͷ��
		png_write_info(png_ptr, info_ptr);

		// ����ÿ���ֽ���������Ĳ�����
		size_t rowSize = width * bandCount;

		// ������ָ������
		std::vector<png_bytep> rowPointers(height);

		// �������ͼ�������ڴ�
		std::vector<unsigned char> outData(height * rowSize);

		// ������ָ��
		for (int y = 0; y < height; y++) {
			rowPointers[y] = &imageData[y * rowSize];
		}

		// д��ͼ������
		png_write_image(png_ptr, rowPointers.data());

		// ���д��
		png_write_end(png_ptr, NULL);

		// ����
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);

		std::cout << "PNG�ļ������ɹ�: " << outputFilename
			<< ((bandCount == 4 || bandCount == 2) ? " (��͸��ͨ��)" : " (����͸��ͨ��)") << std::endl;

		return true;
	}

	// ����ԭ�е���������ʵ��...
	// writeJPEG, writeRGBJPEG, writeGrayscaleJPEG, isNodataPixel, initPNG �Ⱥ���
	// ��Щ�������ֲ��䣬ֻ��Ҫ������������ʵ����

	bool ImageFileParallelIOAdapter::writeJPEG(IOFileInfo* fileInfo, int width, int height, const JPEGOptions& opts) {
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

	bool ImageFileParallelIOAdapter::writeRGBJPEG(IOFileInfo* fileInfo, int width, int height, int quality/* = 85*/) {
		if (fileInfo->dataSize != width * height * 3) {
			fprintf(stderr, "RGB���ݴ�С��ƥ��\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height, { quality, true, JCS_RGB });
	}

	bool ImageFileParallelIOAdapter::writeGrayscaleJPEG(IOFileInfo* fileInfo, int width, int height, int quality /*= 85*/) {
		if (fileInfo->dataSize != width * height) {
			fprintf(stderr, "�Ҷ����ݴ�С��ƥ��\n");
			return false;
		}
		return writeJPEG(fileInfo, width, height, { quality, true, JCS_GRAYSCALE });
	}

	bool ImageFileParallelIOAdapter::hasNodataPixels(const unsigned char* imageData, int width, int height, int bandCount, int bitDepth, const std::vector<double>& nodata)
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

	bool ImageFileParallelIOAdapter::isNodataPixel(const unsigned char* imageData, int pixelPos, int bandCount, int bitDepth, const std::vector<double>& nodata)
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

	bool ImageFileParallelIOAdapter::initPNG(FILE* fp, png_structp& png_ptr, png_infop& info_ptr)
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

	void ImageFileParallelIOAdapter::processPixelData(const unsigned char* imageData, std::vector<unsigned char>& outData, int width, int height, int bandCount, int outBandCount, int bitDepth, const std::vector<double>& nodata, bool requiresAlpha)
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
		
};