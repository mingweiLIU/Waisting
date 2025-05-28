#include "Rectangle.h"

namespace WT {
	Rectangle Rectangle::MAX_VALUE = Rectangle(-glm::pi<float>(), -glm::half_pi<float>(), glm::pi<float>(), glm::half_pi<float>());

	double Rectangle::computeWidth() {
		if (east<west)
		{
			east += (glm::pi<float>() * 2.0);
		}
		return (east - west);
	}
	double Rectangle::computeHeight() {
		return (north - south);
	}

	Cartographic Rectangle::getCenter() const
	{
		double east_adjusted = this->east;
		const double west_val = this->west;

		if (east_adjusted < west_val) {
			east_adjusted += glm::two_pi<double>();
		}

		const double longitude = glm::mod((west_val + east_adjusted) * 0.5 + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>();
		// glm::wrap((west_val + east_adjusted) * 0.5, -glm::pi<double>(), glm::pi<double>()));
		const double latitude = (this->south + this->north) * 0.5;

		return Cartographic(longitude, latitude, 0.0);
	}

	std::optional<Rectangle> Rectangle::intersection(const Rectangle& otherRectangle) const
	{
		double thisRectangleEast = this->east;
		double thisRectangleWest = this->west;

		double otherRectangleEast = otherRectangle.east;
		double otherRectangleWest = otherRectangle.west;

		// 步骤 1: 调整经度以处理潜在的反子午线跨越。
		// 逻辑与静态版本相同，只是现在使用 `this` 和 `otherRectangle` 的成员。

		if (thisRectangleEast < thisRectangleWest && otherRectangleEast > 0.0) {
			thisRectangleEast += glm::two_pi<double>();
		}
		else if (otherRectangleEast < otherRectangleWest && thisRectangleEast > 0.0) {
			otherRectangleEast += glm::two_pi<double>();
		}

		if (thisRectangleEast < thisRectangleWest && otherRectangleWest < 0.0) {
			otherRectangleWest += glm::two_pi<double>();
		}
		else if (otherRectangleEast < otherRectangleWest && thisRectangleWest < 0.0) {
			thisRectangleWest += glm::two_pi<double>();
		}

		// 计算交集的经度边界。
		double temp = std::max(thisRectangleWest, otherRectangleWest);
		const double intersectionWest = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>(); 
		temp = std::min(thisRectangleEast, otherRectangleEast);
		const double intersectionEast = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>();

		// 检查经度交集的有效性。
		// 这里需要特别注意 `this->west < this->east` 和 `otherRectangle.west < otherRectangle.east`
		// 它们检查原始矩形是否没有跨越反子午线。
		if ((this->west < this->east || otherRectangle.west < otherRectangle.east) &&intersectionEast <= intersectionWest) {
			return std::nullopt; // 没有经度交集
		}

		// 计算交集的纬度边界。
		const double intersectionSouth = std::max(this->south, otherRectangle.south);
		const double intersectionNorth = std::min(this->north, otherRectangle.north);

		// 检查纬度交集的有效性。
		if (intersectionSouth >= intersectionNorth) {
			return std::nullopt; // 没有纬度交集
		}
		return Rectangle(intersectionWest, intersectionSouth, intersectionEast, intersectionNorth);
	}

	std::optional<WT::Rectangle> Rectangle::simpleIntersection(const Rectangle& otherRectangle) const
	{
		// 计算交集的边界
		const double intersectionWest = std::max(this->west, otherRectangle.west);
		const double intersectionSouth = std::max(this->south, otherRectangle.south);
		const double intersectionEast = std::min(this->east, otherRectangle.east);
		const double intersectionNorth = std::min(this->north, otherRectangle.north);

		// 检查是否存在有效的交集：如果南边界大于等于北边界，或西边界大于等于东边界，则没有交集。
		if (intersectionSouth >= intersectionNorth || intersectionWest >= intersectionEast) {
			return std::nullopt; // 没有交集
		}

		// 如果存在交集，则根据 `result` 参数存储或创建新矩形。
		return Rectangle(intersectionWest, intersectionSouth, intersectionEast, intersectionNorth);
	}

	Rectangle Rectangle::computeUnion(const Rectangle& otherRectangle) const
	{
		// 复制经度值，以便在处理跨反子午线情况时进行调整，而不修改原始矩形
		double thisRectangleEast = this->east;
		double thisRectangleWest = this->west;

		double otherRectangleEast = otherRectangle.east;
		double otherRectangleWest = otherRectangle.west;

		// 调整经度以处理潜在的反子午线跨越。
		// 这部分逻辑与 `intersection` 函数中的调整逻辑相同，目的是将经度规范化到
		// 一个适合比较和计算的 2π 范围内。
		if (thisRectangleEast < thisRectangleWest && otherRectangleEast > 0.0) {
			thisRectangleEast += glm::two_pi<double>();
		}
		else if (otherRectangleEast < otherRectangleWest && thisRectangleEast > 0.0) {
			otherRectangleEast += glm::two_pi<double>();
		}

		if (thisRectangleEast < thisRectangleWest && otherRectangleWest < 0.0) {
			otherRectangleWest += glm::two_pi<double>();
		}
		else if (otherRectangleEast < otherRectangleWest && thisRectangleWest < 0.0) {
			thisRectangleWest += glm::two_pi<double>();
		}

		// 计算并集的经度边界。
		// 对于并集，西经取最小值，东经取最大值。
		// 同样，使用 CesiumMath::negativePiToPi 将结果规范化回 [-PI, PI)。
		double temp = std::min(thisRectangleWest, otherRectangleWest);
		const double unionWest = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>();
		temp = std::max(thisRectangleEast, otherRectangleEast);
		const double unionEast = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>();

		// 计算并集的纬度边界。
		// 对于并集，南纬取最小值，北纬取最大值。
		Rectangle result;
		result.west = unionWest;
		result.south = std::min(this->south, otherRectangle.south);
		result.east = unionEast;
		result.north = std::max(this->north, otherRectangle.north);

		return result;
	}

	Rectangle Rectangle::expand(const Cartographic& cartographic) const
	{
		// 计算扩展后的新边界：
		// 新的西边界是当前矩形西边界与地理坐标点经度中的最小值。
		// 新的南边界是当前矩形南边界与地理坐标点纬度中的最小值。
		// 新的东边界是当前矩形东边界与地理坐标点经度中的最大值。
		// 新的北边界是当前矩形北边界与地理坐标点纬度中的最大值。
		Rectangle result;
		result.west = std::min(this->west, cartographic.longitude);
		result.south = std::min(this->south, cartographic.latitude);
		result.east = std::max(this->east, cartographic.longitude);
		result.north = std::max(this->north, cartographic.latitude);

		return result;
	}

	bool Rectangle::contains(const Cartographic& cartographic) const
	{
		// 使用局部变量复制经度值，以便在处理跨反子午线情况时进行调整，
		// 而不修改原始矩形或地理坐标点。
		double longitude = cartographic.longitude;
		const double latitude = cartographic.latitude;

		const double west = this->west;
		double east = this->east; // east 可能被修改

		// 步骤 1: 如果矩形跨越反子午线 (east < west)，则调整经度。
		// 如果矩形跨越反子午线，我们将其“东”边界扩大 2π，
		// 以便将其视为一个连续的范围（例如，从 170 度到 190 度，而不是 170 度到 -170 度）。
		// 如果测试点的经度是负数，也会进行调整以匹配这个扩展的范围。
		if (east < west) {
			east += glm::two_pi<double>();
			// 如果测试点的经度是负数（例如 -175 度），加上 2π 将其转换为
			// 等效的正数范围（例如 185 度），以便进行一致的比较。
			if (longitude < 0.0) {
				longitude += glm::two_pi<double>();
			}
		}

		// 步骤 2: 执行包含性检查。
		// 经度必须在调整后的 [west, east] 范围内（包括边界，使用 epsilon 进行比较）。
		// 纬度必须在 [south, north] 范围内（包括边界）。
		return (
			(longitude > west || std::abs(longitude- west)<= 1e-14) &&
			(longitude < east || std::abs(longitude - east) <= 1e-14) &&
			latitude >= this->south &&
			latitude <= this->north);
	}

	std::vector<glm::dvec3> Rectangle::subsample(Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/, double surfaceHeight /*= 0.0*/) const
	{
		// `result` 的默认值处理在函数体内部。
		std::vector<glm::dvec3>result;

		const double north = this->north;
		const double south = this->south;
		const double east = this->east;
		const double west = this->west;

		// 使用局部 Cartographic 临时变量，避免频繁创建对象
		Cartographic subsampleLlaScratch;
		subsampleLlaScratch.height = surfaceHeight;

		// 添加矩形的四个角点
		subsampleLlaScratch.longitude = west;
		subsampleLlaScratch.latitude = north;
		result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));

		subsampleLlaScratch.longitude = east;
		result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));

		subsampleLlaScratch.latitude = south;
		result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));

		subsampleLlaScratch.longitude = west;
		result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));

		// 根据矩形跨越赤道或极点的情况，添加额外的采样点
		if (north < 0.0) { // 矩形完全在南半球
			subsampleLlaScratch.latitude = north;
		}
		else if (south > 0.0) { // 矩形完全在北半球
			subsampleLlaScratch.latitude = south;
		}
		else { // 矩形跨越赤道或赤道在其边界上
			subsampleLlaScratch.latitude = 0.0; // 在赤道上采样
		}

		// 沿赤道或矩形北/南边界（如果完全在半球内）添加额外的采样点
		// 这些点用于处理跨越 0、+/-PI/2、+/-PI 等重要经线的情况
		for (int i = 1; i < 8; ++i) { // 循环添加 7 个点 (PI/2 的倍数)
			subsampleLlaScratch.longitude = -glm::pi<double>() + i * glm::half_pi<double>();
			if (this->contains(subsampleLlaScratch)) { // 检查点是否在矩形内
				result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));
			}
		}

		// 如果矩形跨越赤道，则在赤道上添加矩形左右经线的点
		if (subsampleLlaScratch.latitude == 0.0) { // 即上面 if/else if/else 块中 lat 被设置为 0.0
			subsampleLlaScratch.longitude = west;
			result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));

			subsampleLlaScratch.longitude = east;
			result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));
		}

		return result;
	}

	Rectangle Rectangle::subsection(double westLerp, double southLerp, double eastLerp, double northLerp) const
	{
		Rectangle result;

		// 此函数不使用 CesiumMath.lerp，因为它在起始和结束值相同但 t 变化时存在浮点精度问题。
		// （即，当 A 和 B 相等时，lerp(A, B, t) 应该始终是 A，但浮点运算可能导致微小偏差）。

		// 处理经度 (考虑跨越反子午线的情况)
		if (this->west <= this->east) {
			// 矩形不跨越反子午线 (正常情况)
			const double width = this->east - this->west;
			result.west = this->west + westLerp * width;
			result.east = this->west + eastLerp * width;
		}
		else {
			// 矩形跨越反子午线
			// 宽度为 2π 加上东经减去西经
			const double width = glm::two_pi<double>() + this->east - this->west;
			double temp = this->west + westLerp * width;
			result.west = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>(); 
			temp = this->west + eastLerp * width;
			result.east = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>();
		}

		// 处理纬度 (不涉及环绕，直接计算)
		const double height = this->north - this->south;
		result.south = this->south + southLerp * height;
		result.north = this->south + northLerp * height;

		// 修正浮点精度问题，当插值因子为 1.0 时，确保结果精确等于原始矩形的对应边界。
		if (westLerp == 1.0) {
			result.west = this->east;
		}
		if (eastLerp == 1.0) {
			result.east = this->east;
		}
		if (southLerp == 1.0) {
			result.south = this->north;
		}
		if (northLerp == 1.0) {
			result.north = this->north;
		}

		return result;
	}

	Rectangle Rectangle::fromDegrees(double west_degrees /*= 0.0*/, double south_degrees /*= 0.0*/, double east_degrees /*= 0.0*/, double north_degrees /*= 0.0*/)
	{
		double west_rad = glm::radians(west_degrees);
		double south_rad = glm::radians(south_degrees);
		double east_rad = glm::radians(east_degrees);
		double north_rad = glm::radians(north_degrees);
		return Rectangle(west_rad, south_rad, east_rad, north_rad);
	}

	Rectangle Rectangle::fromRadians(double west_radians /*= 0.0*/, double south_radians /*= 0.0*/, double east_radians /*= 0.0*/, double north_radians /*= 0.0*/)
	{
		return Rectangle(west_radians, south_radians, east_radians, north_radians);
	}

	Rectangle Rectangle::fromCartographicArray(const std::vector<Cartographic>& cartographics)
	{
		// 初始化边界值
		double west = std::numeric_limits<double>::max();     // 最小西经 (初始设置为最大值)
		double east = std::numeric_limits<double>::lowest();   // 最大东经 (初始设置为最小值)
		double south = std::numeric_limits<double>::max();    // 最小南纬
		double north = std::numeric_limits<double>::lowest();  // 最大北纬

		// 处理跨国际日期变更线的情况所需的额外变量
		// westOverIDL：将所有经度规范化到 [0, 2PI) 范围后的最小西经
		double westOverIDL = std::numeric_limits<double>::max();
		// eastOverIDL：将所有经度规范化到 [0, 2PI) 范围后的最大东经
		double eastOverIDL = std::numeric_limits<double>::lowest();

		// 遍历所有地理坐标点
		for (const auto& position : cartographics) {
			// 更新常规的经纬度边界
			west = std::min(west, position.longitude);
			east = std::max(east, position.longitude);
			south = std::min(south, position.latitude);
			north = std::max(north, position.latitude);

			// 调整经度到 [0, 2PI) 范围，用于处理跨越国际日期变更线的情况
			// 如果经度为负，则加上 TWO_PI 得到正向的等效值
			const double lonAdjusted = (position.longitude >= 0)
				? position.longitude
				: position.longitude + glm::two_pi<double>();
			westOverIDL = std::min(westOverIDL, lonAdjusted);
			eastOverIDL = std::max(eastOverIDL, lonAdjusted);
		}

		// 比较两种包围盒的宽度：常规包围盒和跨越国际日期变更线（IDL）的包围盒
		// 如果常规包围盒的宽度大于调整后的包围盒宽度，说明数据可能跨越了 IDL。
		// 此时，使用调整后的经度来定义矩形边界，并将其映射回 [-PI, PI] 范围。
		if ((east - west) > (eastOverIDL - westOverIDL)) {
			west = westOverIDL;
			east = eastOverIDL;

			// 如果调整后的东经超过 PI，则减去 2PI，将其映射回 [-PI, PI] 范围
			if (east > glm::pi<double>()) {
				east = east - glm::two_pi<double>();
			}
			// 如果调整后的西经超过 PI，则减去 2PI，将其映射回 [-PI, PI] 范围
			if (west > glm::pi<double>()) {
				west = west - glm::two_pi<double>();
			}
		}

		return Rectangle(west, south, east, north);
	}

	Rectangle Rectangle::fromCartesianArray(const std::vector<glm::dvec3>& cartesians,  Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/)
	{
		// 初始化边界值
		double west = std::numeric_limits<double>::max();     // 最小西经 (初始设置为最大值)
		double east = std::numeric_limits<double>::lowest();   // 最大东经 (初始设置为最小值)
		double south = std::numeric_limits<double>::max();    // 最小南纬
		double north = std::numeric_limits<double>::lowest();  // 最大北纬

		// 处理跨国际日期变更线的情况所需的额外变量
		// westOverIDL：将所有经度规范化到 [0, 2PI) 范围后的最小西经
		double westOverIDL = std::numeric_limits<double>::max();
		// eastOverIDL：将所有经度规范化到 [0, 2PI) 范围后的最大东经
		double eastOverIDL = std::numeric_limits<double>::lowest();

		// 遍历所有笛卡尔坐标点
		for (const auto& cartesian_position : cartesians) {
			// 将笛卡尔坐标转换为地理坐标
			std::optional<Cartographic> opt= ellipsoid.cartesianToCartographic(cartesian_position);
			if (!opt)
			{
				continue;
			}
			Cartographic position = opt.value();

			// 更新常规的经纬度边界
			west = std::min(west, position.longitude);
			east = std::max(east, position.longitude);
			south = std::min(south, position.latitude);
			north = std::max(north, position.latitude);

			// 调整经度到 [0, 2PI) 范围，用于处理跨越国际日期变更线的情况
			// 如果经度为负，则加上 TWO_PI 得到正向的等效值
			const double lonAdjusted = (position.longitude >= 0)
				? position.longitude
				: position.longitude + glm::two_pi<double>();
			westOverIDL = std::min(westOverIDL, lonAdjusted);
			eastOverIDL = std::max(eastOverIDL, lonAdjusted);
		}

		// 比较两种包围盒的宽度：常规包围盒和跨越国际日期变更线（IDL）的包围盒
		// 如果常规包围盒的宽度大于调整后的包围盒宽度，说明数据可能跨越了 IDL。
		// 此时，使用调整后的经度来定义矩形边界，并将其映射回 [-PI, PI] 范围。
		if ((east - west) > (eastOverIDL - westOverIDL)) {
			west = westOverIDL;
			east = eastOverIDL;

			// 如果调整后的东经超过 PI，则减去 2PI，将其映射回 [-PI, PI] 范围
			if (east > glm::pi<double>()) {
				east = east - glm::two_pi<double>();
			}
			// 如果调整后的西经超过 PI，则减去 2PI，将其映射回 [-PI, PI] 范围
			if (west > glm::pi<double>()) {
				west = west - glm::two_pi<double>();
			}
		}

		return Rectangle(west, south, east, north);
	}

	bool Rectangle::equalsEpsilon(const Rectangle& left, const Rectangle& right, double absoluteEpsilon /*= 0.0*/)
	{
		// 核心比较逻辑：检查每个分量是否在容差范围内
		return (std::abs(left.west - right.west) <= absoluteEpsilon &&
			std::abs(left.south - right.south) <= absoluteEpsilon &&
			std::abs(left.east - right.east) <= absoluteEpsilon &&
			std::abs(left.north - right.north) <= absoluteEpsilon);
	}

	bool Rectangle::equalsEpsilon(const Rectangle& right, double absoluteEpsilon /*= 0.0*/)
	{
		return Rectangle::equalsEpsilon(*this, right, absoluteEpsilon);
	}

	bool Rectangle::equals(const Rectangle& left, const Rectangle& right)
	{
		return (left.west == right.west &&
			left.south == right.south &&
			left.east == right.east &&
			left.north == right.north);
	}

	bool Rectangle::equals(const Rectangle& right)
	{
		return Rectangle::equals(*this, right);
	}

};