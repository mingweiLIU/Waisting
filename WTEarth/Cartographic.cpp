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

	std::optional<Cartographic> Cartographic::fromCartesian(const glm::dvec3& cartesian, Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/)
	{
		const glm::dvec3& oneOverRadii = ellipsoid.getOneOverRadii();
		const glm::dvec3& oneOverRadiiSquared = ellipsoid.getOneOverRadiiSquared();
		const double centerToleranceSquared = ellipsoid.getCenterTolernaceSquard();

		// scaleToGeodeticSurface����cartesian��Чʱ�׳��쳣
		const auto p = Ellipsoid::scaleToGeodeticSurface(
			cartesian,
			oneOverRadii,
			oneOverRadiiSquared,
			centerToleranceSquared
		);

		if (!p) {
			return std::nullopt;
		}

		auto n = (*p)*oneOverRadiiSquared;
		n = glm::normalize(n);

		const auto h = cartesian-(*p);

		const double longitude = std::atan2(n.y, n.x);
		const double latitude = std::asin(n.z);
		const double height =glm::sign(glm::dot(h, cartesian)) * glm::length(h);

		return Cartographic(longitude, latitude, height);
	}

	glm::dvec3 Cartographic::toCartesian(Cartographic& cartographic, Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/)
	{
		double longitude = cartographic.longitude;
		double latitude = cartographic.latitude;
		double height = cartographic.height;

		// ��ȡ������İ뾶ƽ��
		glm::dvec3 radiiSquared = ellipsoid.getRadiiSquared();

		static glm::dvec3 scratchN; // ���淨�ߣ���һ����ĵ������꣩
		static glm::dvec3 scratchK; // ͶӰ��������ĵ�

		const double cosLatitude = std::cos(latitude);
		scratchN.x = cosLatitude * std::cos(longitude);
		scratchN.y = cosLatitude * std::sin(longitude);
		scratchN.z = std::sin(latitude);

		// ��һ�� scratchN
		// ������Ҫһ����һ��������glm::normalize ��һ���ܺõ�ѡ��
		scratchN = glm::normalize(scratchN);

		scratchK.x = radiiSquared.x * scratchN.x;
		scratchK.y = radiiSquared.y * scratchN.y;
		scratchK.z = radiiSquared.z * scratchN.z;

		const double gamma = std::sqrt(glm::dot(scratchN, scratchK));
		scratchK /= gamma;
		scratchN *= height;

		return	scratchK + scratchN;
	}


	bool Cartographic::equals(const Cartographic& left, const Cartographic& right)
	{
		return left.longitude == right.longitude &&
			   left.latitude ==  right.latitude &&
			   left.height ==    right.height;
	}

	bool Cartographic::equalsEpsilon(const Cartographic& left, const Cartographic& right, double epsilon /*= 0.0*/)
	{
		return std::abs(left.longitude - right.longitude) <= epsilon &&
			   std::abs(left.latitude -  right.latitude) <= epsilon &&
			   std::abs(left.height -    right.height) <= epsilon;
	}

};