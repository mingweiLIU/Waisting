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
		// Windows平台
		wchar_t path[MAX_PATH] = { 0 };
		GetModuleFileNameW(NULL, path, MAX_PATH);
		executablePath = path;
#elif defined(__APPLE__)
		// macOS平台
		char path[PATH_MAX];
		uint32_t size = sizeof(path);
		if (_NSGetExecutablePath(path, &size) == 0) {
			executablePath = path;
		}
#elif defined(__linux__)
		// Linux平台
		char path[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
		if (count != -1) {
			path[count] = '\0';
			executablePath = path;
		}
#else
		// 其他平台可能需要特定的实现
		throw std::runtime_error("不支持的平台");
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

		// 初始化坐标系统转换
		const char* proj_wkt = GDALGetProjectionRef(dataset);
		bool has_projection = (proj_wkt && strlen(proj_wkt) > 0);

		// 创建源坐标系
		src_srs = OSRNewSpatialReference(nullptr);
		if (!src_srs) {
			//std::cerr << "无法创建源坐标系统对象" << std::endl;
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
			//std::cerr << "无法创建目标坐标系统对象" << std::endl;
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


	bool CoordinateSystem::transform(double& x, double& y) const
	{
		if (needs_transform && coord_transform) {
			return OCTTransform(coord_transform, 1, &x, &y, nullptr) != 0;
		}
		return true; // 不需要转换
	}

	bool CoordinateSystem::transform_points(double* x, double* y, int point_count) const
	{
		if (needs_transform && coord_transform) {
			return OCTTransform(coord_transform, point_count, x, y, nullptr);
		}
		return true; // 不需要转换
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
		const double EARTH_RADIUS = 6378137.0; // WGS84 椭球长半轴（米）
		const double DEG_TO_RAD = M_PI / 180.0;

		// 水平分辨率（米）：随纬度变化
		xResMeter = xResDeg * (EARTH_RADIUS * DEG_TO_RAD * std::cos(lat * DEG_TO_RAD));
		// 垂直分辨率（米）：固定
		yResMeter = yResDeg * (EARTH_RADIUS * DEG_TO_RAD);
	}

};