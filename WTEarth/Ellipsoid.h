#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace WT{
	class Ellipsoid {
	public:
		explicit Ellipsoid(double x = 0.0, double y = 0.0, double z = 0.0);
		explicit Ellipsoid(glm::vec3 radiis);



		// Getter 函数（const 成员函数，保证不修改对象状态）
		const glm::vec3& getRadii() const { return mRadii; }
		const glm::vec3& getRadiiSquared() const { return mRadiiSquared; }
		const glm::vec3& getRadiiToFourth() const { return mRadiiToFourth; }
		const glm::vec3& getOneOverRadii() const { return mOneOverRadii; }
		const glm::vec3& getOneOverRadiiSquared() const { return mOneOverRadiiSquared; }
		double getMinimumRadius() const { return mMinimumRadius; }
		double getMaximumRadius() const { return mMaximumRadius; }

	public:
		static Ellipsoid WGS84;
		static Ellipsoid UNIT_SPHERE;
		static Ellipsoid MOON;
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
		double mMinimumRadius=0.0;
		//最大轴向半径
		double mMaximumRadius=0.0;
		//中心限差
		double mCenterTolernaceSquard = 0.1;
		//x方/z方
		double mSquaredXOverSquaredZ=0.0;
	};
}