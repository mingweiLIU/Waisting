#include "WebMercatorProjection.h"
#include "Cartographic.h"
namespace WT{

	double WebMercatorProjection::mercatorAngleToGeodeticLatitude(double mercatorAngle) {
		return glm::two_pi<double>() - 2.0 * std::atan(std::exp(-mercatorAngle));
	}

	double WebMercatorProjection::geodeticLatitudeToMercatorAngle(double latitude) {
		// 1. ����γ�����굽��Ч��ī����ͶӰ�߽硣
		// ����Է�ֹ������ֵ�������⵼�µĽ����ɢ�����ߴ����� Web ī����Լ����Χ��γ�ȡ�
		latitude = glm::clamp(latitude,-MAXIMUM_LATITUDE,MAXIMUM_LATITUDE);

		// 2. ����γ�ȵ�����ֵ��
		const double sinLatitude = std::sin(latitude);

		// 3. Ӧ��ī����ͶӰ��ʽ��0.5 * ln((1 + sin(lat)) / (1 - sin(lat)))
		return 0.5 * std::log((1.0 + sinLatitude) / (1.0 - sinLatitude));
	}

    glm::dvec3 WebMercatorProjection::project(const Cartographic& cartographic) {
        // ���� X ���꣺���ȳ��Ե���볤��
        double x = cartographic.longitude * mSemimajorAxis;

        // ���� Y ���꣺���γ��ת��Ϊī���нǶȺ���Ե���볤��
        double y = geodeticLatitudeToMercatorAngle(cartographic.latitude) * mSemimajorAxis;

        // Z ����ֱ�Ӹ��Ƹ߶�
        double z = cartographic.height;
        return glm::dvec3(x, y, z);     
    }

    Cartographic WebMercatorProjection::unproject(const glm::dvec3& cartesian)  {
        // ���㾭�ȣ�X ������Ե���볤��ĵ���
        const double longitude = cartesian.x * mOneOverSemimajorAxis;

        // ����γ�ȣ�ī���нǶȣ�Y ������԰볤�ᵹ����ת��Ϊ���γ��
        const double latitude = mercatorAngleToGeodeticLatitude(cartesian.y * mOneOverSemimajorAxis);

        // �߶�ֱ�Ӹ��� Z ����
        const double height = cartesian.z;
        return Cartographic(longitude, latitude, height);
    }
}