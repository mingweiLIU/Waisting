#include "WebMercatorProjection.h"
#include "Cartographic.h"
namespace WT{

	double WebMercatorProjection::mercatorAngleToGeodeticLatitude(double mercatorAngle) {
		return glm::two_pi<double>() - 2.0 * std::atan(std::exp(-mercatorAngle));
	}

	double WebMercatorProjection::geodeticLatitudeToMercatorAngle(double latitude) {
		// 1. 限制纬度坐标到有效的墨卡托投影边界。
		// 这可以防止由于数值精度问题导致的结果发散，或者处理超出 Web 墨卡托约定范围的纬度。
		latitude = glm::clamp(latitude,-MAXIMUM_LATITUDE,MAXIMUM_LATITUDE);

		// 2. 计算纬度的正弦值。
		const double sinLatitude = std::sin(latitude);

		// 3. 应用墨卡托投影公式：0.5 * ln((1 + sin(lat)) / (1 - sin(lat)))
		return 0.5 * std::log((1.0 + sinLatitude) / (1.0 - sinLatitude));
	}

    glm::dvec3 WebMercatorProjection::project(const Cartographic& cartographic) {
        // 计算 X 坐标：经度乘以地球半长轴
        double x = cartographic.longitude * mSemimajorAxis;

        // 计算 Y 坐标：大地纬度转换为墨卡托角度后乘以地球半长轴
        double y = geodeticLatitudeToMercatorAngle(cartographic.latitude) * mSemimajorAxis;

        // Z 坐标直接复制高度
        double z = cartographic.height;
        return glm::dvec3(x, y, z);     
    }

    Cartographic WebMercatorProjection::unproject(const glm::dvec3& cartesian)  {
        // 计算经度：X 坐标乘以地球半长轴的倒数
        const double longitude = cartesian.x * mOneOverSemimajorAxis;

        // 计算纬度：墨卡托角度（Y 坐标乘以半长轴倒数）转换为大地纬度
        const double latitude = mercatorAngleToGeodeticLatitude(cartesian.y * mOneOverSemimajorAxis);

        // 高度直接复制 Z 坐标
        const double height = cartesian.z;
        return Cartographic(longitude, latitude, height);
    }
}