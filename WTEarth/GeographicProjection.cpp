#include "GeographicProjection.h"
#include "Cartographic.h"
namespace WT{

	glm::dvec3 GeographicProjection::project(const Cartographic& cartographic) {
		double x = cartographic.longitude * mSemimajorAxis;
		double y = cartographic.latitude * mSemimajorAxis;
		double z = cartographic.height;
		return glm::dvec3(x, y, z);
	}

	Cartographic GeographicProjection::unproject(const glm::dvec3& cartesian) {
		double longitude = cartesian.x * mOneOverSemimajorAxis;
		double latitude = cartesian.y * mOneOverSemimajorAxis;
		double height = cartesian.z;
		return Cartographic(longitude, latitude, height);
	}
}