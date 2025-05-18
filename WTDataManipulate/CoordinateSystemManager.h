#pragma once
#include <sstream>
#include <string>
#include <iostream>
#include <fstream> 
// GDAL库
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <cpl_vsi.h>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX // 在包含Windows.h之前定义
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;
namespace WT {
    /**
     * @brief 坐标系统封装类，处理空间参考和转换
     */
    class CoordinateSystem {
    private:
        OGRSpatialReferenceH src_srs;
        OGRSpatialReferenceH dst_srs;
        OGRCoordinateTransformationH coord_transform;
        bool needs_transform;

        void cleanup();

        fs::path getExecutablePath();

        void setGDALEnv();

    public:
        CoordinateSystem() : src_srs(nullptr), dst_srs(nullptr),
            coord_transform(nullptr), needs_transform(false) {
            setGDALEnv();
        }

        ~CoordinateSystem() {
            cleanup();
        }

        /**
         * @brief 初始化坐标系统
         * @param dataset GDAL数据集
         * @param prj_file PRJ文件路径
         * @param wkt_string WKT字符串
         * @return 是否成功初始化
         */
        bool initialize(GDALDatasetH dataset, const std::string& prj_file, const std::string& wkt_string);
        /**
         * @brief 转换坐标
         * @param x X坐标，输入输出参数
         * @param y Y坐标，输入输出参数
         * @return 是否转换成功
         */
        bool transform(double& x, double& y) const;

        /**
         * @brief 转换多个坐标点
         * @param points 坐标点数组，每对double表示一个点
         * @param point_count 点的数量
         * @return 是否转换成功
         */
        bool transform_points(double* x, double* y, int point_count) const;
        /**
         * @brief 创建逆向转换对象
         * @return 成功返回转换对象，失败返回nullptr
         */
        OGRCoordinateTransformationH create_inverse_transform() const;

        /**
         * @brief 检查是否需要坐标转换
         * @return 是否需要转换
         */
		bool requires_transform() const {
			return needs_transform;
		}

        // 计算WGS84地理坐标系下的分辨率（度 → 米）
        void calculateGeographicResolution(double lat, double& xResDeg, double& yResDeg, double& xResMeter, double& yResMeter);
    };
};