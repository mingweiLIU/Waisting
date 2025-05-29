#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include "Ellipsoid.h"
#include "Projection.h"

namespace WT{
	class GeographicProjection final :public Projection{
	private:
	public:
		GeographicProjection(const Ellipsoid& ellipsoid=Ellipsoid::WGS84):Projection(ellipsoid){}
		/**
		  * @brief ������������꣨���ȣ�ת��Ϊ��Ч�ĵѿ��� X��Y��Z ���ꡣ
		  * Z ���꣨�߶ȣ�ֱ�Ӹ��ơ�
		  *
		  * @param cartographic �������꣬��λΪ���ȡ�
		  * @returns ��Ч�ĵѿ������� X��Y��Z ���꣬��λΪ�ס�
		  */
		glm::dvec3 project(const Cartographic& cartographic) override;

		/**
		 * @brief ���ѿ������� X��Y ���꣨��λ�ף�ת��Ϊ���������������� Cartographic��
		 * Z ���꣨�߶ȣ�ֱ�Ӹ��ơ�
		 *
		 * @param cartesian �ѿ������꣬���� Z �����Ǹ߶ȣ���λ�ף���
		 * @returns ��Ч�ĵ������ꡣ
		 */
		Cartographic unproject(const glm::dvec3& cartesian)override;
	};
}