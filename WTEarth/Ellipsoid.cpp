#include "Ellipsoid.h"
#include <iostream>
#include<algorithm>
namespace WT{
	Ellipsoid::Ellipsoid(double x = 0.0, double y = 0.0, double z = 0.0) {
		if (x <= 0 || y <= 0 || z <= 0) {
			std::cerr << "椭球体的xyz轴必须都大于0" << std::endl;
			return;
		}
		this->mRadii = glm::vec3(x, y, z);
		this->mRadiiSquared = glm::vec3(x * x, y * y, z * z);
		this->mRadiiToFourth = glm::vec3(x * x * x * x, y * y * y * y, z * z * z * z);
		this->mOneOverRadii = glm::vec3(
			x == 0.0 ? 0.0 : 1.0 / x,
			y == 0.0 ? 0.0 : 1.0 / y,
			z == 0.0 ? 0.0 : 1.0 / z
		);
		this->mOneOverRadiiSquared = glm::vec3(
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
}