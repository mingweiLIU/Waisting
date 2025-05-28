#include "Cartographic.h"
#include "Ellipsoid.h"
#include <sstream> 
#include <iomanip>
namespace WT {

	std::string Cartographic::toString() const
	{
		std::ostringstream oss;
		oss << "(" << std::setprecision(15)
			<< longitude << ", "
			<< latitude << ", "
			<< height << ")";
		return oss.str();
	}

	Cartographic Cartographic::fromRadians(double longtitude /*= 0*/, double latitude /*= 0*/, double height /*= 0*/)
	{
		return Cartographic(longtitude, latitude, height);
	}

	Cartographic Cartographic::fromDegrees(double longtitude, double latitude, double height /*= 0*/)
	{
		longtitude = glm::radians(longtitude);
		latitude = glm::radians(latitude);
		return Cartographic(longtitude, latitude, height);
	}

	std::optional<Cartographic> Cartographic::fromCartesian(const glm::dvec3& cartesian, const Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/)
	{
		const glm::dvec3& oneOverRadii = ellipsoid ? ellipsoid.oneOverRadii : wgs84OneOverRadii;
		const glm::dvec3& oneOverRadiiSquared = ellipsoid ? ellipsoid.oneOverRadiiSquared : wgs84OneOverRadiiSquared;
		const double centerToleranceSquared = ellipsoid ? ellipsoid._centerToleranceSquared : wgs84CenterToleranceSquared;

		// scaleToGeodeticSurface会在cartesian无效时抛出异常
		const auto p = Ellipsoid::scaleToGeodeticSurface(
			cartesian,
			oneOverRadii,
			oneOverRadiiSquared,
			centerToleranceSquared,
			cartesianToCartographicP
		);

		if (!p) {
			return std::nullopt;
		}

		auto n = glm::compMul(*p, oneOverRadiiSquared, cartesianToCartographicN);
		n = glm::normalize(n);

		const auto h = glm::subtract(cartesian, *p, cartesianToCartographicH);

		const double longitude = std::atan2(n.y, n.x);
		const double latitude = std::asin(n.z);
		const double height =
			CesiumMath::sign(glm::dot(h, cartesian)) * glm::length(h);

		if (!result) {
			return Cartographic(longitude, latitude, height);
		}

		result->longitude = longitude;
		result->latitude = latitude;
		result->height = height;
		return *result;
	}

	glm::dvec3 Cartographic::toCartesian(const Cartographic& cartographic, const Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/)
	{
		// 参数检查
		if (!std::isfinite(cartographic.longitude) ||
			!std::isfinite(cartographic.latitude) ||
			!std::isfinite(cartographic.height)) {
			throw std::invalid_argument("cartographic must have valid finite values");
		}

		return Cartesian3::fromRadians(
			cartographic.longitude,
			cartographic.latitude,
			cartographic.height,
			ellipsoid
		);
	}

	bool Cartographic::equals(const Cartographic& left, const Cartographic& right)
	{
		return left->longitude == right->longitude &&
			left->latitude == right->latitude &&
			left->height == right->height;
	}

	bool Cartographic::equalsEpsilon(const Cartographic& left, const Cartographic& right, double epsilon /*= 0.0*/)
	{
		return std::abs(left->longitude - right->longitude) <= epsilon &&
			std::abs(left->latitude - right->latitude) <= epsilon &&
			std::abs(left->height - right->height) <= epsilon;
	}

};