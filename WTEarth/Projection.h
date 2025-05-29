#pragma once
#include "Ellipsoid.h"
namespace WT {
	class Projection
	{
	public:
		Projection(const Ellipsoid& ellipsoid = Ellipsoid::WGS84) :mEllipsoid(ellipsoid) {
			mSemimajorAxis = ellipsoid.getMaximumRadius();
			mOneOverSemimajorAxis = 1.0 / mSemimajorAxis;
		}

		const Ellipsoid& getEllipsoid()const { return mEllipsoid; };

		/**
		 * @brief ������������꣨���ȣ�ת��Ϊ��Ч�ĵѿ��� X��Y��Z ���ꡣ
		 * Z ���꣨�߶ȣ�ֱ�Ӹ��ơ�
		 *
		 * @param cartographic �������꣬��λΪ���ȡ�
		 * @returns ��Ч�ĵѿ������� X��Y��Z ���꣬��λΪ�ס�
		 */
		virtual glm::dvec3 project(const Cartographic& cartographic)=0;

		/**
		 * @brief ���ѿ������� X��Y ���꣨��λ�ף�ת��Ϊ���������������� Cartographic��
		 * Z ���꣨�߶ȣ�ֱ�Ӹ��ơ�
		 *
		 * @param cartesian �ѿ������꣬���� Z �����Ǹ߶ȣ���λ�ף���
		 * @returns ��Ч�ĵ������ꡣ
		 */
		virtual Cartographic unproject(const glm::dvec3& cartesian)=0;

	protected:
		const Ellipsoid& mEllipsoid;
		double mSemimajorAxis;
		double mOneOverSemimajorAxis;
	};
};