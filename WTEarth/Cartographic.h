#pragma once
#include "Ellipsoid.h"
namespace WT {
	class Cartographic
	{
	public:
		Cartographic(double longitude = 0, double latitude = 0, double height = 0)
			:longitude(longitude), latitude(latitude), height(height) {}

		/**
		* ���ɱ�ʾ�õ���������ַ�������ʽΪ"(longitude, latitude, height)"
		* @return ��ʽ���������ַ���
		*/
		std::string toString() const;

		/**
		 * @brief �ӻ���ֵ�����µĵ�������
		 *
		 * @param longitude_rad ���ȣ���λ�����ȣ�Ĭ��0.0��
		 * @param latitude_rad γ�ȣ���λ�����ȣ�Ĭ��0.0��
		 * @param height_m �߶ȣ���λ���ף�Ĭ��0.0��
		 *
		 * @note �˺��������е�λת����ֱ��ʹ������Ļ���ֵ
		 *
		 * @example
		 * // ��������λ�õĵ������꣨ʹ�û���ֵ��
		 * const double lon_rad = 2.0316;  // ~116.4��ת���Ļ���
		 * const double lat_rad = 0.6964;  // ~39.9��ת���Ļ���
		 * auto beijing = fromRadians(lon_rad, lat_rad, 50.0);
		 */
		static Cartographic fromRadians(double longtitude = 0, double latitude = 0, double height = 0);

		/**
		 * @brief �Ӷȷ����ʽ�����µĵ�������(����Ի��ȴ洢)
		 *
		 * @param longitude_deg ����(��)
		 * @param latitude_deg γ��(��)
		 * @param height_m �߶�(��)��Ĭ��Ϊ0.0
		 * @return Cartographic ����������
		 *
		 * @example
		 * // ��������λ�õĵ�������
		 * auto beijing = Cartographic::fromDegrees(116.4, 39.9, 50.0);
		 * // ����: 2.0316����(��116.4��)
		 * // γ��: 0.6964����(��39.9��)
		 * // �߶�: 50.0��
		 */
		static Cartographic fromDegrees(double longtitude, double latitude, double height = 0);

		/**
		 * �ӵѿ������괴���µĵ�������ʵ��������е�ֵ���Ի��ȱ�ʾ��
		 *
		 * @param cartesian Ҫת��Ϊ���������ʾ�ĵѿ���λ��
		 * @param ellipsoid λ�����ڵ������壬Ĭ��ΪWGS84������
		 * @param result ���ڴ洢����Ķ���
		 * @return ����ṩ��result�����򷵻��޸ĺ��result�����򷵻��µ�Cartographicʵ����
		 *         ����ѿ�������λ�������������򷵻�std::nullopt
		 */
		static std::optional<Cartographic> fromCartesian(const glm::dvec3& cartesian, const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

		/**
		 * �ӵ�������(Cartographic)�����µĵѿ�������(Cartesian3)ʵ����
		 * ��������е�ֵӦΪ�����ơ�
		 *
		 * @param cartographic Ҫת��Ϊ�ѿ�������������������
		 * @param ellipsoid �������ڵ������壬Ĭ��ΪWGS84������
		 * @param result �洢����Ķ���(��ѡ)
		 * @return ����ת����ĵѿ�������λ��
		 *
		 * @throws std::invalid_argument ���cartographic������Ч
		 */
		static glm::dvec3 toCartesian(const Cartographic& cartographic, const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

		/**
		 * �Ƚ�������������ĸ��������������ȫ��ͬ�򷵻�true�����򷵻�false
		 *
		 * @param left ��һ����������
		 * @param right �ڶ�����������
		 * @return �������������ȫ��ͬ����true�����򷵻�false
		 */
		static bool equals(const Cartographic& left, const Cartographic& right);


		/**
		 * �Ƚ�������������ĸ�������������ڸ�����epsilon��Χ������Ϊ���
		 *
		 * @param left ��һ����������
		 * @param right �ڶ�����������
		 * @param epsilon �Ƚ�ʱ�������Χ��Ĭ��Ϊ0
		 * @return �������������epsilon��Χ����ȷ���true�����򷵻�false
		 */
		static bool equalsEpsilon(const Cartographic& left, const Cartographic& right, double epsilon = 0.0);

	public:
		double longitude, latitude, height;
	}
};