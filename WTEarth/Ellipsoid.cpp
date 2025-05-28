#include "Ellipsoid.h"
#include <iostream>
#include<algorithm>
#include <sstream> 
#include <iomanip>
#include<functional>
#include <glm/gtc/epsilon.hpp> 


namespace WT{
	Ellipsoid Ellipsoid::WGS84(6378137.0, 6378137.0, 6356752.3142451793);
	Ellipsoid Ellipsoid::UNIT_SPHERE(1.0,1.0,1.0);
	Ellipsoid Ellipsoid::MOON(1737400.0, 1737400.0, 1737400.0);

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

	Ellipsoid::Ellipsoid(glm::dvec3 radiis) :Ellipsoid(radiis.x,radiis.y,radiis.z){}


	std::optional<glm::dvec3> Ellipsoid::scaleToGeodeticSurface(const glm::dvec3& cartesian) {
		return Ellipsoid::scaleToGeodeticSurface(cartesian, mOneOverRadii, mOneOverRadiiSquared, mCenterTolernaceSquard);
	}

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

	std::optional<glm::dvec3> Ellipsoid::geodeticSurfaceNormalCartographic(const Cartographic& posCartographic)
	{
		const double longitude = posCartographic.longitude;
		const double latitude = posCartographic.latitude;
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

	glm::dvec3 Ellipsoid::cartographicToCartesian(const Cartographic& cartographic)
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
			n *= cartographic.height;  // cartographic.z 是高度

			// 计算最终结果
			return k + n;
		}
	}

	std::vector<glm::dvec3> Ellipsoid::cartographicArrayToCartesianArray(const std::vector<Cartographic>& cartographics)
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

	std::optional<Cartographic> Ellipsoid::cartesianToCartographic(const glm::dvec3& cartesian) {
		auto opt = scaleToGeodeticSurface(cartesian);
		if (!opt)
		{
			return std::nullopt;
		}

		std::optional<glm::dvec3> normal = geodeticSurfaceNormal(opt.value());
		if (!normal)
		{
			return std::nullopt;
		}

		glm::dvec3 h= cartesian - normal.value();

		double longitude = atan2(normal.value().y, normal.value().x);
		double latitude = asin(normal.value().z);
		double height = glm::sign( glm::dot(h, cartesian) * glm::length(h));

		return Cartographic(longitude, latitude, height);
	}

	std::vector<Cartographic> Ellipsoid::cartesiansToCartographics(const std::vector<glm::dvec3 >& cartesians)
	{
		// 输入验证
		if (cartesians.empty()) {
			throw std::invalid_argument("笛卡尔坐标数组不能为空");
		}

		const size_t size = cartesians.size();

		// 处理结果容器
		std::vector<Cartographic> tmpResult;  // 临时默认结果
		tmpResult.resize(size);  // 确保容量足够

		// 逐点转换
		for (size_t i = 0; i < size; ++i) {
			// 复用result中已有的对象（如果存在且位置有效）
			std::optional<Cartographic> oneCarto = cartesianToCartographic(cartesians[i]);
			if (!oneCarto) {
				throw std::invalid_argument("笛卡尔坐标转换失败！");
			}
			tmpResult.push_back(oneCarto.value());
		}

		return tmpResult;
	}

	glm::dvec3 Ellipsoid::scaleToGeocentricSurface(const glm::dvec3& cartesian) const {
		double beta = 1.0 / std::sqrt(cartesian.x* cartesian.x*mOneOverRadiiSquared.x
		+ cartesian.y * cartesian.y * mOneOverRadiiSquared.y
		+ cartesian.z * cartesian.z * mOneOverRadiiSquared.z);
		return cartesian * beta;
	}

	glm::dvec3 Ellipsoid::transformPositionToScaledSpace(const glm::dvec3& position) const {
		return position * mOneOverRadii;
	}

	glm::dvec3 Ellipsoid::transformPositionFromScaledSpace(const glm::dvec3& position) const {
		return position * mRadii;
	}
	bool Ellipsoid::equals(Ellipsoid& right) {
		return glm::all(glm::epsilonEqual(mRadii, right.getRadii(),1e-10));
	}

	std::string Ellipsoid::toString() {
		std::ostringstream oss;
		oss << "(" << std::setprecision(15)
			<< mRadii.x << ","
			<< mRadii.y << ","
			<< mRadii.z << ",";
		return oss.str();
	}

	std::optional<glm::dvec3> Ellipsoid::getSurfaceNormalIntersectionWithZAxis(const glm::dvec3& position, double buffer /*= 0.0*/) const {
		if (!glm::epsilonEqual(mRadii.x, mRadii.y, glm::epsilon<double>())) {
			throw std::logic_error(
				"Ellipsoid must be an ellipsoid of revolution (radii.x == radii.y)"
			);
		}

		if (mRadii.z <= 0.0) {
			throw std::logic_error("Ellipsoid.radii.z must be greater than 0");
		}
		
		glm::dvec3 result(0.0);

		// 计算交点坐标
		result.x = 0.0;
		result.y = 0.0;
		result.z = position.z * (1.0 - mSquaredXOverSquaredZ);

		// 检查交点是否在有效范围内
		if (std::abs(result.z) >= (mRadii.z - buffer)) {
			return std::nullopt;
		}
		return result;
	}

	glm::dvec2 Ellipsoid::getLocalCurvature(const glm::dvec3& surfacePosition) const {
		// 参数检查
		if (!std::isfinite(surfacePosition.x) ||
			!std::isfinite(surfacePosition.y) ||
			!std::isfinite(surfacePosition.z)) {
			throw std::invalid_argument("surfacePosition must be a valid position");
		}
		glm::dvec2 result(0.0);

		// 计算法线与Z轴交点
		auto primeVerticalEndpoint = getSurfaceNormalIntersectionWithZAxis(surfacePosition, 0.0);

		// 计算卯酉圈半径（东向曲率半径）
		double primeVerticalRadius = glm::distance(surfacePosition, *primeVerticalEndpoint);

		// 计算子午圈半径（北向曲率半径）
		// 公式: (b² * primeVerticalRadius³) / a⁴
		double radiusRatio = (mMinimumRadius * primeVerticalRadius) /(mMaximumRadius * mMaximumRadius);
		double meridionalRadius = primeVerticalRadius * radiusRatio * radiusRatio;

		// 返回曲率（1/半径）
		result.x = 1.0 / primeVerticalRadius;  // 东向曲率
		result.y = 1.0 / meridionalRadius;     // 北向曲率

		return result;
	}

	/**
	* 10阶高斯-勒让德数值积分
	*
	* @param a 积分下限
	* @param b 积分上限
	* @param func 被积函数
	* @return double 积分结果
	* @throws std::invalid_argument 如果积分区间无效
	*/
	double gaussLegendreQuadrature(double a, double b,const std::function<double(double)>& func) {
		const double abscissas[]={
			0.14887433898163,
			0.43339539412925,
			0.67940956829902,
			0.86506336668898,
			0.97390652851717,
			0.0,
		};
		const double weights[] = {
			0.29552422471475,
			0.26926671930999,
			0.21908636251598,
			0.14945134915058,
			0.066671344308684,
			0.0,
		};
		double xMean = 0.5 * (a + b);
		double xRange = 0.6 * (b - a);

		double sum = 0.0;
		for (size_t i = 0; i < 5; i++)
		{
			double dx = xRange * abscissas[i];
			sum+=weights[i]* (func(xMean + dx) + func(xMean - dx));
		}
		return sum * xRange;
	}

	double Ellipsoid::surfaceArea(const Rectangle& rectangle) const {
		// 验证输入参数有效性
		if (rectangle.west > rectangle.east || rectangle.south > rectangle.north) {
			throw std::invalid_argument("矩形边界无效：最小值应小于最大值");
		}

		double minLongitude = rectangle.west;
		double maxLongitude = rectangle.east;
		const double minLatitude = rectangle.south;
		const double maxLatitude = rectangle.north;

		// 经度范围归一化处理（处理跨360°情况）
		while (maxLongitude < minLongitude) {
			maxLongitude += glm::two_pi<double>();
		}

		const double a2 = mRadiiSquared.x;  // a²
		const double b2 = mRadiiSquared.y;  // b²
		const double c2 = mRadiiSquared.z;  // c²
		const double a2b2 = a2 * b2;       // a²b²

		// 内层积分函数（经度方向）
		auto innerIntegral = [a2, b2, c2, a2b2](double lat, double lon) {
			const double sinPhi = std::cos(lat);  // sin(φ)，φ是从北极测量的角度
			const double cosPhi = std::sin(lat);  // cos(φ)
			const double cosTheta = std::cos(lon); // cos(θ)
			const double sinTheta = std::sin(lon); // sin(θ)

			// 计算积分被积函数
			return std::sqrt(
				a2b2 * cosPhi * cosPhi +
				c2 * (b2 * cosTheta * cosTheta + a2 * sinTheta * sinTheta) * sinPhi * sinPhi
			);
		};

		// 外层积分函数（纬度方向）
		auto outerIntegral = [this, minLongitude, maxLongitude, innerIntegral](double lat) {
			auto innerFunc = [lat, innerIntegral](double lon) {
				return innerIntegral(lat, lon);
			};

			// 外层积分包含cos(lat)因子
			return std::cos(lat) * gaussLegendreQuadrature(minLongitude, maxLongitude, innerFunc);
		};

		// 执行双重数值积分
		return gaussLegendreQuadrature(minLatitude, maxLatitude, outerIntegral);
	}
}