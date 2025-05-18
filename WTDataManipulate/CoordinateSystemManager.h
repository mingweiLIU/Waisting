#pragma once
#include <sstream>
#include <string>
#include <iostream>
#include <fstream> 
// GDAL��
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <cpl_vsi.h>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX // �ڰ���Windows.h֮ǰ����
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
     * @brief ����ϵͳ��װ�࣬����ռ�ο���ת��
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
         * @brief ��ʼ������ϵͳ
         * @param dataset GDAL���ݼ�
         * @param prj_file PRJ�ļ�·��
         * @param wkt_string WKT�ַ���
         * @return �Ƿ�ɹ���ʼ��
         */
        bool initialize(GDALDatasetH dataset, const std::string& prj_file, const std::string& wkt_string);
        /**
         * @brief ת������
         * @param x X���꣬�����������
         * @param y Y���꣬�����������
         * @return �Ƿ�ת���ɹ�
         */
        bool transform(double& x, double& y) const;

        /**
         * @brief ת����������
         * @param points ��������飬ÿ��double��ʾһ����
         * @param point_count �������
         * @return �Ƿ�ת���ɹ�
         */
        bool transform_points(double* x, double* y, int point_count) const;
        /**
         * @brief ��������ת������
         * @return �ɹ�����ת������ʧ�ܷ���nullptr
         */
        OGRCoordinateTransformationH create_inverse_transform() const;

        /**
         * @brief ����Ƿ���Ҫ����ת��
         * @return �Ƿ���Ҫת��
         */
		bool requires_transform() const {
			return needs_transform;
		}

        // ����WGS84��������ϵ�µķֱ��ʣ��� �� �ף�
        void calculateGeographicResolution(double lat, double& xResDeg, double& yResDeg, double& xResMeter, double& yResMeter);
    };
};