#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include "Ellipsoid.h"
#include "Projection.h"

namespace WT{
	class GeographicProjection final :public Projection{
	private:
	public:
		GeographicProjection(const Ellipsoid& ellipsoid=Ellipsoid::WGS84):Projection(ellipsoid){}
		/**
		  * @brief 将大地椭球坐标（弧度）转换为等效的笛卡尔 X、Y、Z 坐标。
		  * Z 坐标（高度）直接复制。
		  *
		  * @param cartographic 地理坐标，单位为弧度。
		  * @returns 等效的笛卡尔坐标 X、Y、Z 坐标，单位为米。
		  */
		glm::dvec3 project(const Cartographic& cartographic) override;

		/**
		 * @brief 将笛卡尔坐标 X、Y 坐标（单位米）转换为包含大地椭球坐标的 Cartographic。
		 * Z 坐标（高度）直接复制。
		 *
		 * @param cartesian 笛卡尔坐标，其中 Z 坐标是高度（单位米）。
		 * @returns 等效的地理坐标。
		 */
		Cartographic unproject(const glm::dvec3& cartesian)override;
	};
}