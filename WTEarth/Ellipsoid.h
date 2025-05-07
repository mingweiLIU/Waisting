#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace WT{
	class Ellipsoid {
	public:
		Ellipsoid(double x, double y, double z) {

		}
	private:
		void initialize(double x, double y, double z);
	private:
		//三个轴向的半径
		glm::vec3 mRadii;
		//三个轴向半径的平方
		glm::vec3 mRadiiSquared;
		//三个轴向半径的四次方
		glm::vec3 mRadiiToFourth;
		//三个轴向半径分之一
		glm::vec3 mOneOverRadii;
		//三个轴向半径平方之一
		glm::vec3 mOneOverRadiiSquared;
		//最小轴向半径
		double mMinimumRadius;
		//最大轴向半径
		double mMaximumRadius;
	};
}