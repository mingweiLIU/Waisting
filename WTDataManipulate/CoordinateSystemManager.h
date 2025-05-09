#pragma once
#include <sstream>
// GDAL库
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <cpl_vsi.h>

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

        void cleanup() {
            if (coord_transform) {
                OCTDestroyCoordinateTransformation(coord_transform);
                coord_transform = nullptr;
            }
            if (src_srs) {
                OSRDestroySpatialReference(src_srs);
                src_srs = nullptr;
            }
            if (dst_srs) {
                OSRDestroySpatialReference(dst_srs);
                dst_srs = nullptr;
            }
        }

    public:
        CoordinateSystem() : src_srs(nullptr), dst_srs(nullptr),
            coord_transform(nullptr), needs_transform(false) {}

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
        bool initialize(GDALDatasetH dataset, const std::string& prj_file, const std::string& wkt_string) {
            cleanup();

            // 初始化坐标系统转换
            const char* proj_wkt = GDALGetProjectionRef(dataset);
            bool has_projection = (proj_wkt && strlen(proj_wkt) > 0);

            // 创建源坐标系
            src_srs = OSRNewSpatialReference(nullptr);
            if (!src_srs) {
                std::cerr << "无法创建源坐标系统对象" << std::endl;
                return false;
            }

            bool spatial_ref_set = false;

            if (has_projection) {
                // 使用数据集自带的投影信息
                if (OSRImportFromWkt(src_srs, const_cast<char**>(&proj_wkt)) == OGRERR_NONE) {
                    spatial_ref_set = true;
                }
            }

            if (!spatial_ref_set && !wkt_string.empty()) {
                // 使用用户提供的 WKT 字符串
                std::cout << "使用用户提供的 WKT 字符串作为投影信息" << std::endl;
                char* wkt_copy = strdup(wkt_string.c_str());
                if (OSRImportFromWkt(src_srs, &wkt_copy) == OGRERR_NONE) {
                    spatial_ref_set = true;
                }
                free(wkt_copy);
            }

            if (!spatial_ref_set && !prj_file.empty() && fs::exists(prj_file)) {
                // 使用用户提供的 PRJ 文件
                std::cout << "使用用户提供的 PRJ 文件作为投影信息: " << prj_file << std::endl;

                try {
                    // 读取 PRJ 文件内容
                    std::ifstream prj_stream(prj_file);
                    if (prj_stream) {
                        std::stringstream buffer;
                        buffer << prj_stream.rdbuf();
                        std::string prj_content = buffer.str();
                        prj_stream.close();

                        if (!prj_content.empty()) {
                            char* prj_copy = strdup(prj_content.c_str());
                            if (OSRImportFromWkt(src_srs, &prj_copy) == OGRERR_NONE) {
                                spatial_ref_set = true;
                            }
                            free(prj_copy);
                        }
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "读取PRJ文件出错: " << e.what() << std::endl;
                }
            }

            if (!spatial_ref_set) {
                // 没有可用的投影信息
                std::cerr << "警告: 输入数据集没有空间参考信息，且未提供有效的 PRJ 文件或 WKT 字符串" << std::endl;
                std::cerr << "无法进行坐标转换，将假设输入数据已经是 WGS84 坐标系" << std::endl;

                // 将源坐标系设置为 WGS84
                if (OSRSetFromUserInput(src_srs, "EPSG:4326") != OGRERR_NONE) {
                    std::cerr << "无法设置源坐标系为WGS84" << std::endl;
                    cleanup();
                    return false;
                }
                needs_transform = false;
                return true;
            }

            // 创建目标坐标系 (WGS84)
            dst_srs = OSRNewSpatialReference(nullptr);
            if (!dst_srs) {
                std::cerr << "无法创建目标坐标系统对象" << std::endl;
                cleanup();
                return false;
            }

            if (OSRSetFromUserInput(dst_srs, "EPSG:4326") != OGRERR_NONE) {
                std::cerr << "无法设置目标坐标系为WGS84" << std::endl;
                cleanup();
                return false;
            }

            // 检查是否需要坐标转换
            if (src_srs && !OSRIsSame(src_srs, dst_srs)) {
                coord_transform = OCTNewCoordinateTransformation(src_srs, dst_srs);
                needs_transform = (coord_transform != nullptr);

                if (!coord_transform) {
                    std::cerr << "警告: 无法创建坐标转换，可能由于无效的空间参考" << std::endl;
                    needs_transform = false;
                }
            }
            else {
                needs_transform = false;
            }

            return true;
        }

        /**
         * @brief 转换坐标
         * @param x X坐标，输入输出参数
         * @param y Y坐标，输入输出参数
         * @return 是否转换成功
         */
        bool transform(double& x, double& y) const {
            if (needs_transform && coord_transform) {
                return OCTTransform(coord_transform, 1, &x, &y, nullptr) != 0;
            }
            return true; // 不需要转换
        }

        /**
         * @brief 转换多个坐标点
         * @param points 坐标点数组，每对double表示一个点
         * @param point_count 点的数量
         * @return 是否转换成功
         */
        bool transform_points(double* points, int point_count) const {
            if (needs_transform && coord_transform) {
                return OCTTransform(coord_transform, point_count, points, points + point_count, nullptr) != 0;
            }
            return true; // 不需要转换
        }

        /**
         * @brief 创建逆向转换对象
         * @return 成功返回转换对象，失败返回nullptr
         */
        OGRCoordinateTransformationH create_inverse_transform() const {
            if (needs_transform && src_srs && dst_srs) {
                return OCTNewCoordinateTransformation(dst_srs, src_srs);
            }
            return nullptr;
        }

        /**
         * @brief 检查是否需要坐标转换
         * @return 是否需要转换
         */
        bool requires_transform() const {
            return needs_transform;
        }
    };
};