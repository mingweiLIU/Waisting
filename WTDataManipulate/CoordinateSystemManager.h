#pragma once
#include <sstream>
// GDAL��
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <cpl_vsi.h>

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
         * @brief ��ʼ������ϵͳ
         * @param dataset GDAL���ݼ�
         * @param prj_file PRJ�ļ�·��
         * @param wkt_string WKT�ַ���
         * @return �Ƿ�ɹ���ʼ��
         */
        bool initialize(GDALDatasetH dataset, const std::string& prj_file, const std::string& wkt_string) {
            cleanup();

            // ��ʼ������ϵͳת��
            const char* proj_wkt = GDALGetProjectionRef(dataset);
            bool has_projection = (proj_wkt && strlen(proj_wkt) > 0);

            // ����Դ����ϵ
            src_srs = OSRNewSpatialReference(nullptr);
            if (!src_srs) {
                std::cerr << "�޷�����Դ����ϵͳ����" << std::endl;
                return false;
            }

            bool spatial_ref_set = false;

            if (has_projection) {
                // ʹ�����ݼ��Դ���ͶӰ��Ϣ
                if (OSRImportFromWkt(src_srs, const_cast<char**>(&proj_wkt)) == OGRERR_NONE) {
                    spatial_ref_set = true;
                }
            }

            if (!spatial_ref_set && !wkt_string.empty()) {
                // ʹ���û��ṩ�� WKT �ַ���
                std::cout << "ʹ���û��ṩ�� WKT �ַ�����ΪͶӰ��Ϣ" << std::endl;
                char* wkt_copy = strdup(wkt_string.c_str());
                if (OSRImportFromWkt(src_srs, &wkt_copy) == OGRERR_NONE) {
                    spatial_ref_set = true;
                }
                free(wkt_copy);
            }

            if (!spatial_ref_set && !prj_file.empty() && fs::exists(prj_file)) {
                // ʹ���û��ṩ�� PRJ �ļ�
                std::cout << "ʹ���û��ṩ�� PRJ �ļ���ΪͶӰ��Ϣ: " << prj_file << std::endl;

                try {
                    // ��ȡ PRJ �ļ�����
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
                    std::cerr << "��ȡPRJ�ļ�����: " << e.what() << std::endl;
                }
            }

            if (!spatial_ref_set) {
                // û�п��õ�ͶӰ��Ϣ
                std::cerr << "����: �������ݼ�û�пռ�ο���Ϣ����δ�ṩ��Ч�� PRJ �ļ��� WKT �ַ���" << std::endl;
                std::cerr << "�޷���������ת�������������������Ѿ��� WGS84 ����ϵ" << std::endl;

                // ��Դ����ϵ����Ϊ WGS84
                if (OSRSetFromUserInput(src_srs, "EPSG:4326") != OGRERR_NONE) {
                    std::cerr << "�޷�����Դ����ϵΪWGS84" << std::endl;
                    cleanup();
                    return false;
                }
                needs_transform = false;
                return true;
            }

            // ����Ŀ������ϵ (WGS84)
            dst_srs = OSRNewSpatialReference(nullptr);
            if (!dst_srs) {
                std::cerr << "�޷�����Ŀ������ϵͳ����" << std::endl;
                cleanup();
                return false;
            }

            if (OSRSetFromUserInput(dst_srs, "EPSG:4326") != OGRERR_NONE) {
                std::cerr << "�޷�����Ŀ������ϵΪWGS84" << std::endl;
                cleanup();
                return false;
            }

            // ����Ƿ���Ҫ����ת��
            if (src_srs && !OSRIsSame(src_srs, dst_srs)) {
                coord_transform = OCTNewCoordinateTransformation(src_srs, dst_srs);
                needs_transform = (coord_transform != nullptr);

                if (!coord_transform) {
                    std::cerr << "����: �޷���������ת��������������Ч�Ŀռ�ο�" << std::endl;
                    needs_transform = false;
                }
            }
            else {
                needs_transform = false;
            }

            return true;
        }

        /**
         * @brief ת������
         * @param x X���꣬�����������
         * @param y Y���꣬�����������
         * @return �Ƿ�ת���ɹ�
         */
        bool transform(double& x, double& y) const {
            if (needs_transform && coord_transform) {
                return OCTTransform(coord_transform, 1, &x, &y, nullptr) != 0;
            }
            return true; // ����Ҫת��
        }

        /**
         * @brief ת����������
         * @param points ��������飬ÿ��double��ʾһ����
         * @param point_count �������
         * @return �Ƿ�ת���ɹ�
         */
        bool transform_points(double* points, int point_count) const {
            if (needs_transform && coord_transform) {
                return OCTTransform(coord_transform, point_count, points, points + point_count, nullptr) != 0;
            }
            return true; // ����Ҫת��
        }

        /**
         * @brief ��������ת������
         * @return �ɹ�����ת������ʧ�ܷ���nullptr
         */
        OGRCoordinateTransformationH create_inverse_transform() const {
            if (needs_transform && src_srs && dst_srs) {
                return OCTNewCoordinateTransformation(dst_srs, src_srs);
            }
            return nullptr;
        }

        /**
         * @brief ����Ƿ���Ҫ����ת��
         * @return �Ƿ���Ҫת��
         */
        bool requires_transform() const {
            return needs_transform;
        }
    };
};