#pragma once
#include "../WTDataManipulate/SlippyTiler.h"
#include "../WTDataManipulate/IDataM.h"

void main() {

    // 创建切片器
    std::shared_ptr< WT::SlippyMapTilerOptions> options = std::make_shared<WT::SlippyMapTilerOptions>();
	options->inputFile = "D:\\Data\\DOM1\\yangjiabaDOM.tif";
	options->outputDir = "D:\\Data\\DOM1\\test";
    //rgb真彩色 nodata为0 0 0
	//options->inputFile = "E:\\Data\\dq\\yangjiaba\\initial\\DOM1\\yangjiabaDOM.tif";
	//options->outputDir = "E:\\Data\\dq\\yangjiaba\\initial\\DOM1\\test";
    //单波段灰白色 nodata自带 像素为32位
    //options->inputFile = "E:\\Data\\dq\\DEM\\DEM_1000h.img";
    //options->outputDir = "E:\\Data\\23";
    //带调色板的八位影像 nodata 255 255 255
    //options->inputFile = "C:\\Users\\ROG\\Desktop\\11.tif";
    //options->outputDir = "C:\\Users\\ROG\\Desktop\\12";
	//options->inputFile = "C:\\Users\\MW\\Desktop\\11.tif";
	//options->outputDir = "D:\\Data\\DOM1\\palete";
    
    std::shared_ptr<WT::IProgressInfo> p = std::make_shared<WT::IProgressInfo>();
    WT::SlippyMapTiler cutter(options);

    // 初始化
    if (!cutter.initialize()) {
        std::cerr << "cuowu" << std::endl;
        return;
    }

    // 处理切片
    std::cout << "开始生成切片..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();

    bool success = cutter.process(p);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    if (success) {
        std::cout << "total time: " << duration << " s" << std::endl;
        return ;
    }
    else {
        std::cerr << "error,total time: " << duration << " s" << std::endl;
        return ;
    }
}