#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace WT{
	class Ellipsoid {
	public:
		explicit Ellipsoid(double x = 0.0, double y = 0.0, double z = 0.0);
		explicit Ellipsoid(glm::vec3 radiis);



		// Getter ������const ��Ա��������֤���޸Ķ���״̬��
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
		double mMinimumRadius=0.0;
		//�������뾶
		double mMaximumRadius=0.0;
		//�����޲�
		double mCenterTolernaceSquard = 0.1;
		//x��/z��
		double mSquaredXOverSquaredZ=0.0;
	};
}