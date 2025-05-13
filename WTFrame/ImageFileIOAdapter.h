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
		// ��RGB������д��
		bool writeRGBJPEG(IOFileInfo fileInfo, int width, int height, int quality = 85);
		// �ӻҶȻ�����д��
		bool writeGrayscaleJPEG(IOFileInfo fileInfo, int width, int height, int quality = 85);

		//д��png
		bool write_png(IOFileInfo fileInfo, int width, int height,int bands,int color_type = PNG_COLOR_TYPE_RGB,int bit_depth = 8);

		//����Nodata���ƴװ͸��ͼ�� �����Ҫ��͸�� �򷵻�Ϊtrue ����Ҫ��false
		bool nodataCheckAndTrans(unsigned char* oriData, int oriDataSize, unsigned char* newData, int& newDataSize);

		/**
		 * ���ͼ�����Ƿ����nodata����
		 *
		 * @param imageData Ӱ������
		 * @param width ͼ����
		 * @param height ͼ��߶�
		 * @param bandCount ������
		 * @param bitDepth λ���
		 * @param nodata nodataֵ����
		 * @return �������nodata�����򷵻�true�����򷵻�false
		 */
		bool hasNodataPixels(const unsigned char* imageData,
			int width, int height,
			int bandCount, int bitDepth,
			const std::vector<double>& nodata);
		/**
		 * ��������Ƿ�Ϊnodata
		 *
		 * @param imageData Ӱ������
		 * @param pixelPos ����λ��
		 * @param bandCount ������
		 * @param bitDepth λ���
		 * @param nodata nodataֵ����
		 * @return �����nodata�����򷵻�true�����򷵻�false
		 */
		bool isNodataPixel(const unsigned char* imageData,
			int pixelPos,
			int bandCount, int bitDepth,
			const std::vector<double>& nodata);

		/**
		 * ��ʼ��PNGд��ṹ����Ϣ�ṹ
		 *
		 * @param fp �ļ�ָ��
		 * @param png_ptr ���PNGд��ṹָ��
		 * @param info_ptr ���PNG��Ϣ�ṹָ��
		 * @return �ɹ�����true��ʧ�ܷ���false
		 */
		bool initPNG(FILE* fp, png_structp& png_ptr, png_infop& info_ptr);

		/**
		 * �����������ݣ�������������
		 *
		 * @param imageData ����Ӱ������
		 * @param outData ������ݻ�����
		 * @param width ͼ����
		 * @param height ͼ��߶�
		 * @param bandCount ������
		 * @param outBandCount ���������(�������ܵ�͸��ͨ��)
		 * @param bitDepth λ���
		 * @param nodata nodataֵ����
		 * @param requiresAlpha �Ƿ���Ҫ͸��ͨ��
		 */
		void processPixelData(const unsigned char* imageData,
			std::vector<unsigned char>& outData,
			int width, int height,
			int bandCount, int outBandCount,
			int bitDepth,
			const std::vector<double>& nodata,
			bool requiresAlpha);


		/**
		 * ��GDAL��ȡ������ת��ΪPNGͼ�񣬲�����nodataֵΪ͸������
		 * ����������ض�����nodata���򴴽�����͸��ͨ����PNG
		 *
		 * @param imageData GDAL��ȡ��Ӱ�����ݣ���RGBRGB...��ʽ����
		 * @param width ͼ����
		 * @param height ͼ��߶�
		 * @param bandCount ������(1��3)
		 * @param bitDepth ����λ���(8��16)
		 * @param nodata nodataֵ���飬ÿ�����ζ�Ӧһ��nodataֵ
		 * @param outputFilename ���PNG�ļ�·��
		 * @return �ɹ�����true��ʧ�ܷ���false
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
		int mBandsNum;//������
		std::vector<double> mNoData;
	};
};