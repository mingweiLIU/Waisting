#include "CoordinateSystemManager.h"

namespace WT {
	void CoordinateSystem::cleanup()
	{
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

	fs::path CoordinateSystem::getExecutablePath()
	{
		fs::path executablePath;

#ifdef _WIN32
		// Windowsƽ̨
		wchar_t path[MAX_PATH] = { 0 };
		GetModuleFileNameW(NULL, path, MAX_PATH);
		executablePath = path;
#elif defined(__APPLE__)
		// macOSƽ̨
		char path[PATH_MAX];
		uint32_t size = sizeof(path);
		if (_NSGetExecutablePath(path, &size) == 0) {
			executablePath = path;
		}
#elif defined(__linux__)
		// Linuxƽ̨
		char path[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
		if (count != -1) {
			path[count] = '\0';
			executablePath = path;
		}
#else
		// ����ƽ̨������Ҫ�ض���ʵ��
		throw std::runtime_error("��֧�ֵ�ƽ̨");
#endif

		return fs::canonical(executablePath);
	}

	void CoordinateSystem::setGDALEnv()
	{
		std::filesystem::path exePath = getExecutablePath();
		std::filesystem::path exeFolder = exePath.parent_path();
		std::filesystem::path gdalDir = exeFolder / "gdal-data";
		CPLSetConfigOption("GDAL_DATA", gdalDir.string().c_str());
		std::string projPath = (exeFolder / "proj9" / "share").string();
		const char* proj_path[] = { projPath.c_str(), nullptr };
		OSRSetPROJSearchPaths(proj_path);
	}

	bool CoordinateSystem::initialize(GDALDatasetH dataset, const std::string& prj_file, const std::string& wkt_string)
	{
		cleanup();

		// ��ʼ������ϵͳת��
		const char* proj_wkt = GDALGetProjectionRef(dataset);
		bool has_projection = (proj_wkt && strlen(proj_wkt) > 0);

		// ����Դ����ϵ
		src_srs = OSRNewSpatialReference(nullptr);
		if (!src_srs) {
			//std::cerr << "�޷�����Դ����ϵͳ����" << std::endl;
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
			//std::cerr << "�޷�����Ŀ������ϵͳ����" << std::endl;
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


	bool CoordinateSystem::transform(double& x, double& y) const
	{
		if (needs_transform && coord_transform) {
			return OCTTransform(coord_transform, 1, &x, &y, nullptr) != 0;
		}
		return true; // ����Ҫת��
	}

	bool CoordinateSystem::transform_points(double* x, double* y, int point_count) const
	{
		if (needs_transform && coord_transform) {
			return OCTTransform(coord_transform, point_count, x, y, nullptr);
		}
		return true; // ����Ҫת��
	}


	OGRCoordinateTransformationH CoordinateSystem::create_inverse_transform() const
	{
		if (needs_transform && src_srs && dst_srs) {
			return OCTNewCoordinateTransformation(dst_srs, src_srs);
		}
		return nullptr;
	}

	void CoordinateSystem::calculateGeographicResolution(double lat, double& xResDeg, double& yResDeg, double& xResMeter, double& yResMeter)
	{
		const double EARTH_RADIUS = 6378137.0; // WGS84 ���򳤰��ᣨ�ף�
		const double DEG_TO_RAD = M_PI / 180.0;

		// ˮƽ�ֱ��ʣ��ף�����γ�ȱ仯
		xResMeter = xResDeg * (EARTH_RADIUS * DEG_TO_RAD * std::cos(lat * DEG_TO_RAD));
		// ��ֱ�ֱ��ʣ��ף����̶�
		yResMeter = yResDeg * (EARTH_RADIUS * DEG_TO_RAD);
	}

};