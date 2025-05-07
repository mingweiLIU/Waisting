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
		//��������İ뾶
		glm::vec3 mRadii;
		//��������뾶��ƽ��
		glm::vec3 mRadiiSquared;
		//��������뾶���Ĵη�
		glm::vec3 mRadiiToFourth;
		//��������뾶��֮һ
		glm::vec3 mOneOverRadii;
		//��������뾶ƽ��֮һ
		glm::vec3 mOneOverRadiiSquared;
		//��С����뾶
		double mMinimumRadius;
		//�������뾶
		double mMaximumRadius;
	};
}