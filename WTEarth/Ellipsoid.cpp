#include "Ellipsoid.h"
#include <iostream>
#include<algorithm>
namespace WT{
	Ellipsoid Ellipsoid::WGS84(6378137.0, 6378137.0, 6356752.3142451793);
	Ellipsoid Ellipsoid::UNIT_SPHERE(1.0,1.0,1.0);
	Ellipsoid Ellipsoid::MOON(1737400.0, 1737400.0, 1737400.0);


	std::optional<glm::dvec3> Ellipsoid::scaleToGeodeticSurface(const glm::dvec3& cartesian
		, const glm::dvec3& oneOverRadii, const glm::dvec3& oneOverRadiiSquared, double centerToleranceSquared)
	{
		const double positionX = cartesian.x;
		const double positionY = cartesian.y;
		const double positionZ = cartesian.z;

		const double oneOverRadiiX = oneOverRadii.x;
		const double oneOverRadiiY = oneOverRadii.y;
		const double oneOverRadiiZ = oneOverRadii.z;

		const double x2 = positionX * positionX * oneOverRadiiX * oneOverRadiiX;
		const double y2 = positionY * positionY * oneOverRadiiY * oneOverRadiiY;
		const double z2 = positionZ * positionZ * oneOverRadiiZ * oneOverRadiiZ;

		// 计算椭球体范数的平方
		const double squaredNorm = x2 + y2 + z2;
		const double ratio = std::sqrt(1.0 / squaredNorm);

		// 初始近似：假设径向交点为投影点
		glm::dvec3 intersection = cartesian * ratio;

		// 如果位置接近中心，迭代将不收敛
		if (squaredNorm < centerToleranceSquared) {
			return !std::isfinite(ratio) ? std::nullopt :  std::optional(intersection);
		}

		const double oneOverRadiiSquaredX = oneOverRadiiSquared.x;
		const double oneOverRadiiSquaredY = oneOverRadiiSquared.y;
		const double oneOverRadiiSquaredZ = oneOverRadiiSquared.z;

		// 使用交点处的梯度代替真实单位法线
		// 幅度的差异将被乘数吸收
		glm::dvec3 gradient;
		gradient.x = intersection.x * oneOverRadiiSquaredX * 2.0;
		gradient.y = intersection.y * oneOverRadiiSquaredY * 2.0;
		gradient.z = intersection.z * oneOverRadiiSquaredZ * 2.0;

		// 计算法向量乘数lambda的初始猜测值
		double lambda = ((1.0 - ratio) * glm::length(cartesian)) / (0.5 * glm::length(gradient));
		double correction = 0.0;

		double func;
		double denominator;
		double xMultiplier, yMultiplier, zMultiplier;
		double xMultiplier2, yMultiplier2, zMultiplier2;
		double xMultiplier3, yMultiplier3, zMultiplier3;

		do {
			lambda -= correction;

			xMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredX);
			yMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredY);
			zMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredZ);

			xMultiplier2 = xMultiplier * xMultiplier;
			yMultiplier2 = yMultiplier * yMultiplier;
			zMultiplier2 = zMultiplier * zMultiplier;

			xMultiplier3 = xMultiplier2 * xMultiplier;
			yMultiplier3 = yMultiplier2 * yMultiplier;
			zMultiplier3 = zMultiplier2 * zMultiplier;

			func = x2 * xMultiplier2 + y2 * yMultiplier2 + z2 * zMultiplier2 - 1.0;

			// 这里的"denominator"指的是在后续计算中使用的表达式
			denominator = x2 * xMultiplier3 * oneOverRadiiSquaredX +
				y2 * yMultiplier3 * oneOverRadiiSquaredY +
				z2 * zMultiplier3 * oneOverRadiiSquaredZ;

			const double derivative = -2.0 * denominator;
			correction = func / derivative;
		} while (std::abs(func) > 1e-12);

		glm::dvec3 scaledPosition(
			positionX * xMultiplier,
			positionY * yMultiplier,
			positionZ * zMultiplier
		);
		return scaledPosition;
	}

	Ellipsoid::Ellipsoid(double x = 0.0, double y = 0.0, double z = 0.0) {
		if (x <= 0 || y <= 0 || z <= 0) {
			std::cerr << "椭球体的xyz轴必须都大于0" << std::endl;
			return;
		}
		this->mRadii = glm::dvec3(x, y, z);
		this->mRadiiSquared = glm::dvec3(x * x, y * y, z * z);
		this->mRadiiToFourth = glm::dvec3(x * x * x * x, y * y * y * y, z * z * z * z);
		this->mOneOverRadii = glm::dvec3(
			x == 0.0 ? 0.0 : 1.0 / x,
			y == 0.0 ? 0.0 : 1.0 / y,
			z == 0.0 ? 0.0 : 1.0 / z
		);
		this->mOneOverRadiiSquared = glm::dvec3(
			x == 0.0 ? 0.0 : 1.0 / (x * x),
			y == 0.0 ? 0.0 : 1.0 / (y * y),
			z == 0.0 ? 0.0 : 1.0 / (z * z)
		);
		this->mMinimumRadius = std::min({ x,y,z });
		this->mMaximumRadius = std::max({ x,y,z });
		if (this->mRadiiSquared.z != 0)
		{
			this->mSquaredXOverSquaredZ = this->mRadiiSquared.x / this->mRadiiSquared.z;
		}
	}

	std::optional<glm::dvec3> Ellipsoid::geocentricSurfaceNormal(const glm::dvec3& pos)
	{
		const float magnitude=glm::length(pos);
		glm::dvec3 result;
		result.x = pos.x / magnitude;
		result.y = pos.y / magnitude;
		result.z = pos.z / magnitude;

		if (std::isnan(result.x)|| std::isnan(result.y)|| std::isnan(result.z))
		{
			return std::nullopt;
		}
		return result;
	}

	std::optional<glm::dvec3> Ellipsoid::geodeticSurfaceNormalCartographic(const glm::dvec3& posCartographic)
	{
		const double longitude = posCartographic.x;
		const double latitude = posCartographic.y;
		const double cosLatitude = cos(latitude);

		// 计算未归一化的法向量
		glm::dvec3 normal;
		normal.x = cosLatitude * cos(longitude);
		normal.y = cosLatitude * sin(longitude);
		normal.z = sin(latitude);

		return glm::normalize(normal);
	}

	std::optional<glm::dvec3> Ellipsoid::geodeticSurfaceNormal(const glm::dvec3& cartesian)
	{
		if (std::isnan(cartesian.x)|| std::isnan(cartesian.y) || std::isnan(cartesian.z) )
		{
			throw std::invalid_argument("Cartesian position contains NaN component");
		}

		constexpr double EPSILON14 = 1e-14;
		if (std::abs(cartesian.x)<EPSILON14&& std::abs(cartesian.y) < EPSILON14&& std::abs(cartesian.z) < EPSILON14)
		{
			return std::nullopt;
		}

		glm::dvec3 result = cartesian * this->mOneOverRadiiSquared;
		return glm::normalize(result);
	}

	glm::dvec3 Ellipsoid::cartographicToCartesian(const glm::dvec3& cartographic)
	{
		glm::dvec3 n,k;
		std::optional<glm::dvec3> nOpt=this->geodeticSurfaceNormalCartographic(cartographic);
		if (nOpt)
		{
			n = *nOpt;
			k = this->mRadiiSquared* n;
			double gamma = sqrt(glm::dot(n, k));
			k /= gamma;

			// 计算高度分量
			n *= cartographic.z;  // cartographic.z 是高度

			// 计算最终结果
			return k + n;
		}
	}

	std::vector<glm::dvec3> Ellipsoid::cartographicArrayToCartesianArray(const std::vector<glm::dvec3>& cartographics)
	{	
		// 准备结果容器
		std::vector<glm::dvec3> output;
		output.resize(cartographics.size());

		// 批量转换
		for (size_t i = 0; i < cartographics.size(); ++i) {
			output[i] = cartographicToCartesian(cartographics[i]);
		}
		return output;
	}

}