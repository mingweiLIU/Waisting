#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "Ellipsoid.h"

namespace WT{
	class WebMercatorProjection {
	private:
		const Ellipsoid& mEllipsoid;
		double mSemimajorAxis;
		double mOneOverSemimajorAxis;
		// ī����ͶӰ���������/��Сγ�ȡ�
		// Cesium ͨ���������ֵΪ 85.05112878 �ȣ�ת��Ϊ������ atan(sinh(PI))��
		// ����һ������ֵ��ȷ��ͶӰ���ᷢɢ�������
		constexpr double MAXIMUM_LATITUDE = std::atan(std::sinh(glm::pi)); // Լ 1.4844155���ȣ��� 85.05112878 ��
	public:
		WebMercatorProjection(const Ellipsoid& ellipsoid=Ellipsoid::WGS84):mEllipsoid(ellipsoid){
			mSemimajorAxis = ellipsoid.getMaximumRadius();
			mOneOverSemimajorAxis = 1.0 / mSemimajorAxis;
		}

		double getMAXIMUM_LATITUDE() { return MAXIMUM_LATITUDE; }

		const Ellipsoid& getEllipsoid()const (return mEllipsoid);

		/**
		 * @brief ��ī����ͶӰ�Ƕȣ���Χ -PI �� PI��ת��Ϊ���γ�ȣ���Χ -PI/2 �� PI/2����
		 *
		 * ī����ͶӰ�ڵ�����Ϣϵͳ�й㷺ʹ�ã������������ͶӰ��һ��ƽ���ϡ�
		 * �������ʵ���˴�ī����ͶӰ��Y�ᣨ��ơ�ī���нǶȡ����������Ӧ�ĵ���γ�ȡ�
		 * ����ת���ڴ���Webī��������ϵ����OpenStreetMap��Google Maps�ȣ�ʱ�ǳ����á�
		 *
		 * @param mercatorAngle ��ת����ī���нǶȣ���λΪ���ȣ���Χ�� -PI �� PI ֮�䡣
		 * @returns ת����Ĵ��γ�ȣ���λΪ���ȣ���Χ�� -PI/2 �� PI/2 ֮�䡣
 */
		double mercatorAngleToGeodeticLatitude(double mercatorAngle);

		/**
		 * @brief �����γ�ȣ����ȣ���Χ -PI/2 �� PI/2��ת��Ϊī���нǶȣ���Χ -PI �� PI����
		 *
		 * �˺���ִ��ī����ͶӰ������ת����������γ�ȣ�ͨ���ǵ�������ʵ��γ�ȣ�
		 * ӳ�䵽 Web ī��������ϵ�е� Y ��ֵ������ī���нǶȡ�����
		 * �� GIS Ӧ���У��ⳣ���ڽ���������ת��Ϊ��Ļ����Ƭ��ͼ���ꡣ
		 *
		 * @param latitude ��ת���Ĵ��γ�ȣ���λΪ���ȣ���Χ�� -��/2 �� ��/2 ֮�䡣
		 * @returns ת�����ī���нǶȣ���λΪ���ȣ���Χ�� -�� �� �� ֮�䡣
		 */
		double geodeticLatitudeToMercatorAngle(double latitude);

        /**
		 * @brief ������������꣨���ȣ�ת��Ϊ��Ч�� Web ī���� X��Y��Z ���ꡣ
		 * Z ���꣨�߶ȣ�ֱ�Ӹ��ơ�
		 *
		 * @param cartographic �������꣬��λΪ���ȡ�
		 * @returns ��Ч�� Web ī���� X��Y��Z ���꣬��λΪ�ס�
		 */
		glm::dvec3 project(const Cartographic& cartographic) const;

        /**
         * @brief �� Web ī���� X��Y ���꣨��λ�ף�ת��Ϊ���������������� Cartographic��
         * Z ���꣨�߶ȣ�ֱ�Ӹ��ơ�
         *
         * @param cartesian Web ī���еѿ������꣬���� Z �����Ǹ߶ȣ���λ�ף���
         * @returns ��Ч�ĵ������ꡣ
         */
		Cartographic unproject(const glm::dvec3& cartesian) const;
	};
}