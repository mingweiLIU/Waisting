#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <future>
#include <memory>
#include "IOAdapter.h"
#include <filesystem>
namespace WT {

	//ר�Ŷ���ΪQuanified Mesh������Ƭ�����
	// TBBǰ������
	namespace tbb {
		class global_control;
	}


	// ǰ������
	struct TerrainTask;

	class TerrainFileParalleIOAdapter :public IOAdapter {
	public:
		// ���캯��
		TerrainFileParalleIOAdapter(const std::string& basePath, bool createDirs = true,
			int width = 256, int height = 256);

		// ��������
		~TerrainFileParalleIOAdapter() = default;

		// ��ʼ��
		bool initialize() override;

		// ��������ļ����첽��
		bool output(const IOFileInfo* fileInfo) override;

		// �������������ͬ����
		bool outputBatch(const std::vector<IOFileInfo*> files) override;

		// �����������ȫ�첽��
		bool outputBatchAsync(const std::vector<IOFileInfo*> files) override;


		std::string type() const override { return "TerrainFileParalleIOAdapter"; }

		// ������
		bool finalize();

	private:
		// ��Ա����
		std::string mBasePath;
		bool mCreateDirs;
		int mWidth;
		int mHeight;
		int ticProgressNum = 0;//ÿ������ٸ��ļ���ִ��һ��progressCallback

		// ˽�з���

		// ��������������
		bool processTerrainTask(const TerrainTask& task);

		// ������ת��Ϊ����
		bool encodeTerrain(IOFileInfo* ioFileInfo);
	};
};