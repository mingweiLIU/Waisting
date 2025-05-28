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

		// ���� 1: ���������Դ���Ǳ�ڵķ������߿�Խ��
		// �߼��뾲̬�汾��ͬ��ֻ������ʹ�� `this` �� `otherRectangle` �ĳ�Ա��

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

		// ���㽻���ľ��ȱ߽硣
		double temp = std::max(thisRectangleWest, otherRectangleWest);
		const double intersectionWest = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>(); 
		temp = std::min(thisRectangleEast, otherRectangleEast);
		const double intersectionEast = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>();

		// ��龭�Ƚ�������Ч�ԡ�
		// ������Ҫ�ر�ע�� `this->west < this->east` �� `otherRectangle.west < otherRectangle.east`
		// ���Ǽ��ԭʼ�����Ƿ�û�п�Խ�������ߡ�
		if ((this->west < this->east || otherRectangle.west < otherRectangle.east) &&intersectionEast <= intersectionWest) {
			return std::nullopt; // û�о��Ƚ���
		}

		// ���㽻����γ�ȱ߽硣
		const double intersectionSouth = std::max(this->south, otherRectangle.south);
		const double intersectionNorth = std::min(this->north, otherRectangle.north);

		// ���γ�Ƚ�������Ч�ԡ�
		if (intersectionSouth >= intersectionNorth) {
			return std::nullopt; // û��γ�Ƚ���
		}
		return Rectangle(intersectionWest, intersectionSouth, intersectionEast, intersectionNorth);
	}

	std::optional<WT::Rectangle> Rectangle::simpleIntersection(const Rectangle& otherRectangle) const
	{
		// ���㽻���ı߽�
		const double intersectionWest = std::max(this->west, otherRectangle.west);
		const double intersectionSouth = std::max(this->south, otherRectangle.south);
		const double intersectionEast = std::min(this->east, otherRectangle.east);
		const double intersectionNorth = std::min(this->north, otherRectangle.north);

		// ����Ƿ������Ч�Ľ���������ϱ߽���ڵ��ڱ��߽磬�����߽���ڵ��ڶ��߽磬��û�н�����
		if (intersectionSouth >= intersectionNorth || intersectionWest >= intersectionEast) {
			return std::nullopt; // û�н���
		}

		// ������ڽ���������� `result` �����洢�򴴽��¾��Ρ�
		return Rectangle(intersectionWest, intersectionSouth, intersectionEast, intersectionNorth);
	}

	Rectangle Rectangle::computeUnion(const Rectangle& otherRectangle) const
	{
		// ���ƾ���ֵ���Ա��ڴ���練���������ʱ���е����������޸�ԭʼ����
		double thisRectangleEast = this->east;
		double thisRectangleWest = this->west;

		double otherRectangleEast = otherRectangle.east;
		double otherRectangleWest = otherRectangle.west;

		// ���������Դ���Ǳ�ڵķ������߿�Խ��
		// �ⲿ���߼��� `intersection` �����еĵ����߼���ͬ��Ŀ���ǽ����ȹ淶����
		// һ���ʺϱȽϺͼ���� 2�� ��Χ�ڡ�
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

		// ���㲢���ľ��ȱ߽硣
		// ���ڲ���������ȡ��Сֵ������ȡ���ֵ��
		// ͬ����ʹ�� CesiumMath::negativePiToPi ������淶���� [-PI, PI)��
		double temp = std::min(thisRectangleWest, otherRectangleWest);
		const double unionWest = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>();
		temp = std::max(thisRectangleEast, otherRectangleEast);
		const double unionEast = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>();

		// ���㲢����γ�ȱ߽硣
		// ���ڲ�������γȡ��Сֵ����γȡ���ֵ��
		Rectangle result;
		result.west = unionWest;
		result.south = std::min(this->south, otherRectangle.south);
		result.east = unionEast;
		result.north = std::max(this->north, otherRectangle.north);

		return result;
	}

	Rectangle Rectangle::expand(const Cartographic& cartographic) const
	{
		// ������չ����±߽磺
		// �µ����߽��ǵ�ǰ�������߽����������㾭���е���Сֵ��
		// �µ��ϱ߽��ǵ�ǰ�����ϱ߽�����������γ���е���Сֵ��
		// �µĶ��߽��ǵ�ǰ���ζ��߽����������㾭���е����ֵ��
		// �µı��߽��ǵ�ǰ���α��߽�����������γ���е����ֵ��
		Rectangle result;
		result.west = std::min(this->west, cartographic.longitude);
		result.south = std::min(this->south, cartographic.latitude);
		result.east = std::max(this->east, cartographic.longitude);
		result.north = std::max(this->north, cartographic.latitude);

		return result;
	}

	bool Rectangle::contains(const Cartographic& cartographic) const
	{
		// ʹ�þֲ��������ƾ���ֵ���Ա��ڴ���練���������ʱ���е�����
		// �����޸�ԭʼ���λ��������㡣
		double longitude = cartographic.longitude;
		const double latitude = cartographic.latitude;

		const double west = this->west;
		double east = this->east; // east ���ܱ��޸�

		// ���� 1: ������ο�Խ�������� (east < west)����������ȡ�
		// ������ο�Խ�������ߣ����ǽ��䡰�����߽����� 2�У�
		// �Ա㽫����Ϊһ�������ķ�Χ�����磬�� 170 �ȵ� 190 �ȣ������� 170 �ȵ� -170 �ȣ���
		// ������Ե�ľ����Ǹ�����Ҳ����е�����ƥ�������չ�ķ�Χ��
		if (east < west) {
			east += glm::two_pi<double>();
			// ������Ե�ľ����Ǹ��������� -175 �ȣ������� 2�� ����ת��Ϊ
			// ��Ч��������Χ������ 185 �ȣ����Ա����һ�µıȽϡ�
			if (longitude < 0.0) {
				longitude += glm::two_pi<double>();
			}
		}

		// ���� 2: ִ�а����Լ�顣
		// ���ȱ����ڵ������ [west, east] ��Χ�ڣ������߽磬ʹ�� epsilon ���бȽϣ���
		// γ�ȱ����� [south, north] ��Χ�ڣ������߽磩��
		return (
			(longitude > west || std::abs(longitude- west)<= 1e-14) &&
			(longitude < east || std::abs(longitude - east) <= 1e-14) &&
			latitude >= this->south &&
			latitude <= this->north);
	}

	std::vector<glm::dvec3> Rectangle::subsample(Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/, double surfaceHeight /*= 0.0*/) const
	{
		// `result` ��Ĭ��ֵ�����ں������ڲ���
		std::vector<glm::dvec3>result;

		const double north = this->north;
		const double south = this->south;
		const double east = this->east;
		const double west = this->west;

		// ʹ�þֲ� Cartographic ��ʱ����������Ƶ����������
		Cartographic subsampleLlaScratch;
		subsampleLlaScratch.height = surfaceHeight;

		// ��Ӿ��ε��ĸ��ǵ�
		subsampleLlaScratch.longitude = west;
		subsampleLlaScratch.latitude = north;
		result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));

		subsampleLlaScratch.longitude = east;
		result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));

		subsampleLlaScratch.latitude = south;
		result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));

		subsampleLlaScratch.longitude = west;
		result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));

		// ���ݾ��ο�Խ����򼫵���������Ӷ���Ĳ�����
		if (north < 0.0) { // ������ȫ���ϰ���
			subsampleLlaScratch.latitude = north;
		}
		else if (south > 0.0) { // ������ȫ�ڱ�����
			subsampleLlaScratch.latitude = south;
		}
		else { // ���ο�Խ�����������߽���
			subsampleLlaScratch.latitude = 0.0; // �ڳ���ϲ���
		}

		// �س������α�/�ϱ߽磨�����ȫ�ڰ����ڣ���Ӷ���Ĳ�����
		// ��Щ�����ڴ����Խ 0��+/-PI/2��+/-PI ����Ҫ���ߵ����
		for (int i = 1; i < 8; ++i) { // ѭ����� 7 ���� (PI/2 �ı���)
			subsampleLlaScratch.longitude = -glm::pi<double>() + i * glm::half_pi<double>();
			if (this->contains(subsampleLlaScratch)) { // �����Ƿ��ھ�����
				result.push_back(ellipsoid.cartographicToCartesian(subsampleLlaScratch));
			}
		}

		// ������ο�Խ��������ڳ������Ӿ������Ҿ��ߵĵ�
		if (subsampleLlaScratch.latitude == 0.0) { // ������ if/else if/else ���� lat ������Ϊ 0.0
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

		// �˺�����ʹ�� CesiumMath.lerp����Ϊ������ʼ�ͽ���ֵ��ͬ�� t �仯ʱ���ڸ��㾫�����⡣
		// �������� A �� B ���ʱ��lerp(A, B, t) Ӧ��ʼ���� A��������������ܵ���΢Сƫ���

		// ������ (���ǿ�Խ�������ߵ����)
		if (this->west <= this->east) {
			// ���β���Խ�������� (�������)
			const double width = this->east - this->west;
			result.west = this->west + westLerp * width;
			result.east = this->west + eastLerp * width;
		}
		else {
			// ���ο�Խ��������
			// ���Ϊ 2�� ���϶�����ȥ����
			const double width = glm::two_pi<double>() + this->east - this->west;
			double temp = this->west + westLerp * width;
			result.west = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>(); 
			temp = this->west + eastLerp * width;
			result.east = glm::mod(temp + glm::pi<double>(), glm::two_pi<double>()) - glm::pi<double>();
		}

		// ����γ�� (���漰���ƣ�ֱ�Ӽ���)
		const double height = this->north - this->south;
		result.south = this->south + southLerp * height;
		result.north = this->south + northLerp * height;

		// �������㾫�����⣬����ֵ����Ϊ 1.0 ʱ��ȷ�������ȷ����ԭʼ���εĶ�Ӧ�߽硣
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
		// ��ʼ���߽�ֵ
		double west = std::numeric_limits<double>::max();     // ��С���� (��ʼ����Ϊ���ֵ)
		double east = std::numeric_limits<double>::lowest();   // ��󶫾� (��ʼ����Ϊ��Сֵ)
		double south = std::numeric_limits<double>::max();    // ��С��γ
		double north = std::numeric_limits<double>::lowest();  // ���γ

		// �����������ڱ���ߵ��������Ķ������
		// westOverIDL�������о��ȹ淶���� [0, 2PI) ��Χ�����С����
		double westOverIDL = std::numeric_limits<double>::max();
		// eastOverIDL�������о��ȹ淶���� [0, 2PI) ��Χ�����󶫾�
		double eastOverIDL = std::numeric_limits<double>::lowest();

		// �������е��������
		for (const auto& position : cartographics) {
			// ���³���ľ�γ�ȱ߽�
			west = std::min(west, position.longitude);
			east = std::max(east, position.longitude);
			south = std::min(south, position.latitude);
			north = std::max(north, position.latitude);

			// �������ȵ� [0, 2PI) ��Χ�����ڴ����Խ�������ڱ���ߵ����
			// �������Ϊ��������� TWO_PI �õ�����ĵ�Чֵ
			const double lonAdjusted = (position.longitude >= 0)
				? position.longitude
				: position.longitude + glm::two_pi<double>();
			westOverIDL = std::min(westOverIDL, lonAdjusted);
			eastOverIDL = std::max(eastOverIDL, lonAdjusted);
		}

		// �Ƚ����ְ�Χ�еĿ�ȣ������Χ�кͿ�Խ�������ڱ���ߣ�IDL���İ�Χ��
		// ��������Χ�еĿ�ȴ��ڵ�����İ�Χ�п�ȣ�˵�����ݿ��ܿ�Խ�� IDL��
		// ��ʱ��ʹ�õ�����ľ�����������α߽磬������ӳ��� [-PI, PI] ��Χ��
		if ((east - west) > (eastOverIDL - westOverIDL)) {
			west = westOverIDL;
			east = eastOverIDL;

			// ���������Ķ������� PI�����ȥ 2PI������ӳ��� [-PI, PI] ��Χ
			if (east > glm::pi<double>()) {
				east = east - glm::two_pi<double>();
			}
			// ������������������ PI�����ȥ 2PI������ӳ��� [-PI, PI] ��Χ
			if (west > glm::pi<double>()) {
				west = west - glm::two_pi<double>();
			}
		}

		return Rectangle(west, south, east, north);
	}

	Rectangle Rectangle::fromCartesianArray(const std::vector<glm::dvec3>& cartesians,  Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/)
	{
		// ��ʼ���߽�ֵ
		double west = std::numeric_limits<double>::max();     // ��С���� (��ʼ����Ϊ���ֵ)
		double east = std::numeric_limits<double>::lowest();   // ��󶫾� (��ʼ����Ϊ��Сֵ)
		double south = std::numeric_limits<double>::max();    // ��С��γ
		double north = std::numeric_limits<double>::lowest();  // ���γ

		// �����������ڱ���ߵ��������Ķ������
		// westOverIDL�������о��ȹ淶���� [0, 2PI) ��Χ�����С����
		double westOverIDL = std::numeric_limits<double>::max();
		// eastOverIDL�������о��ȹ淶���� [0, 2PI) ��Χ�����󶫾�
		double eastOverIDL = std::numeric_limits<double>::lowest();

		// �������еѿ��������
		for (const auto& cartesian_position : cartesians) {
			// ���ѿ�������ת��Ϊ��������
			std::optional<Cartographic> opt= ellipsoid.cartesianToCartographic(cartesian_position);
			if (!opt)
			{
				continue;
			}
			Cartographic position = opt.value();

			// ���³���ľ�γ�ȱ߽�
			west = std::min(west, position.longitude);
			east = std::max(east, position.longitude);
			south = std::min(south, position.latitude);
			north = std::max(north, position.latitude);

			// �������ȵ� [0, 2PI) ��Χ�����ڴ����Խ�������ڱ���ߵ����
			// �������Ϊ��������� TWO_PI �õ�����ĵ�Чֵ
			const double lonAdjusted = (position.longitude >= 0)
				? position.longitude
				: position.longitude + glm::two_pi<double>();
			westOverIDL = std::min(westOverIDL, lonAdjusted);
			eastOverIDL = std::max(eastOverIDL, lonAdjusted);
		}

		// �Ƚ����ְ�Χ�еĿ�ȣ������Χ�кͿ�Խ�������ڱ���ߣ�IDL���İ�Χ��
		// ��������Χ�еĿ�ȴ��ڵ�����İ�Χ�п�ȣ�˵�����ݿ��ܿ�Խ�� IDL��
		// ��ʱ��ʹ�õ�����ľ�����������α߽磬������ӳ��� [-PI, PI] ��Χ��
		if ((east - west) > (eastOverIDL - westOverIDL)) {
			west = westOverIDL;
			east = eastOverIDL;

			// ���������Ķ������� PI�����ȥ 2PI������ӳ��� [-PI, PI] ��Χ
			if (east > glm::pi<double>()) {
				east = east - glm::two_pi<double>();
			}
			// ������������������ PI�����ȥ 2PI������ӳ��� [-PI, PI] ��Χ
			if (west > glm::pi<double>()) {
				west = west - glm::two_pi<double>();
			}
		}

		return Rectangle(west, south, east, north);
	}

	bool Rectangle::equalsEpsilon(const Rectangle& left, const Rectangle& right, double absoluteEpsilon /*= 0.0*/)
	{
		// ���ıȽ��߼������ÿ�������Ƿ����ݲΧ��
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