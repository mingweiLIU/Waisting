#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "Ellipsoid.h"

namespace WT{
	class WebMercatorProjection {
	private:
		const Ellipsoid& mEllipsoid;
		double mSemimajorAxis;
		double mOneOverSemimajorAxis;
		// 墨卡托投影的理论最大/最小纬度。
		// Cesium 通常定义这个值为 85.05112878 度，转换为弧度是 atan(sinh(PI))。
		// 这是一个经验值，确保投影不会发散到无穷大。
		constexpr double MAXIMUM_LATITUDE = std::atan(std::sinh(glm::pi)); // 约 1.4844155弧度，或 85.05112878 度
	public:
		WebMercatorProjection(const Ellipsoid& ellipsoid=Ellipsoid::WGS84):mEllipsoid(ellipsoid){
			mSemimajorAxis = ellipsoid.getMaximumRadius();
			mOneOverSemimajorAxis = 1.0 / mSemimajorAxis;
		}

		double getMAXIMUM_LATITUDE() { return MAXIMUM_LATITUDE; }

		const Ellipsoid& getEllipsoid()const (return mEllipsoid);

		/**
		 * @brief 将墨卡托投影角度（范围 -PI 到 PI）转换为大地纬度（范围 -PI/2 到 PI/2）。
		 *
		 * 墨卡托投影在地理信息系统中广泛使用，它将地球表面投影到一个平面上。
		 * 这个函数实现了从墨卡托投影的Y轴（或称“墨卡托角度”）反算出对应的地理纬度。
		 * 这种转换在处理Web墨卡托坐标系（如OpenStreetMap、Google Maps等）时非常有用。
		 *
		 * @param mercatorAngle 待转换的墨卡托角度，单位为弧度，范围在 -PI 到 PI 之间。
		 * @returns 转换后的大地纬度，单位为弧度，范围在 -PI/2 到 PI/2 之间。
 */
		double mercatorAngleToGeodeticLatitude(double mercatorAngle);

		/**
		 * @brief 将大地纬度（弧度，范围 -PI/2 到 PI/2）转换为墨卡托角度（范围 -PI 到 PI）。
		 *
		 * 此函数执行墨卡托投影的正向转换，将地理纬度（通常是地球表面的实际纬度）
		 * 映射到 Web 墨卡托坐标系中的 Y 轴值（即“墨卡托角度”）。
		 * 在 GIS 应用中，这常用于将地理坐标转换为屏幕或瓦片地图坐标。
		 *
		 * @param latitude 待转换的大地纬度，单位为弧度，范围在 -π/2 到 π/2 之间。
		 * @returns 转换后的墨卡托角度，单位为弧度，范围在 -π 到 π 之间。
		 */
		double geodeticLatitudeToMercatorAngle(double latitude);

        /**
		 * @brief 将大地椭球坐标（弧度）转换为等效的 Web 墨卡托 X、Y、Z 坐标。
		 * Z 坐标（高度）直接复制。
		 *
		 * @param cartographic 地理坐标，单位为弧度。
		 * @returns 等效的 Web 墨卡托 X、Y、Z 坐标，单位为米。
		 */
		glm::dvec3 project(const Cartographic& cartographic) const;

        /**
         * @brief 将 Web 墨卡托 X、Y 坐标（单位米）转换为包含大地椭球坐标的 Cartographic。
         * Z 坐标（高度）直接复制。
         *
         * @param cartesian Web 墨卡托笛卡尔坐标，其中 Z 坐标是高度（单位米）。
         * @returns 等效的地理坐标。
         */
		Cartographic unproject(const glm::dvec3& cartesian) const;
	};
}