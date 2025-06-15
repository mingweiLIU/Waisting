#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <future>
#include <memory>
#include "IOAdapter.h"
#include <filesystem>
namespace WT {

	//专门定义为Quanified Mesh地形瓦片输出器
	// TBB前向声明
	namespace tbb {
		class global_control;
	}


	// 前向声明
	struct TerrainTask;

	class TerrainFileParalleIOAdapter :public IOAdapter {
	public:
		// 构造函数
		TerrainFileParalleIOAdapter(const std::string& basePath, bool createDirs = true,
			int width = 256, int height = 256);

		// 析构函数
		~TerrainFileParalleIOAdapter() = default;

		// 初始化
		bool initialize() override;

		// 输出单个文件（异步）
		bool output(const IOFileInfo* fileInfo) override;

		// 批量输出（并行同步）
		bool outputBatch(const std::vector<IOFileInfo*> files) override;

		// 批量输出（完全异步）
		bool outputBatchAsync(const std::vector<IOFileInfo*> files) override;


		std::string type() const override { return "TerrainFileParalleIOAdapter"; }

		// 完成输出
		bool finalize();

	private:
		// 成员变量
		std::string mBasePath;
		bool mCreateDirs;
		int mWidth;
		int mHeight;
		int ticProgressNum = 0;//每处理多少个文件就执行一次progressCallback

		// 私有方法

		// 处理单个地形任务
		bool processTerrainTask(const TerrainTask& task);

		// 将数据转换为地形
		bool encodeTerrain(IOFileInfo* ioFileInfo);
	};
};