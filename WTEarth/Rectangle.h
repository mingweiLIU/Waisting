#pragma once
#include<glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include "Cartographic.h"
#include "Ellipsoid.h"

namespace WT {
	class Rectangle {
	public:
		Rectangle(double west = 0.0, double south = 0.0, double east = 0.0, double north = 0.0)
			:west(west)
			, south(south)
			, east(east)
			, north(north) {}

		double computeWidth();
		double computeHeight();
		/**
		* @brief �Ƚ��ṩ���������Σ����������ȫ��ȣ��򷵻� `true`��
		*
		* @param right �ڶ������Ρ�
		* @returns ����� `right` ��ȫ��ȣ��򷵻� `true`�����򷵻� `false`��
		*/
		bool equals(const Rectangle& right); 

		/**
		 * @brief �������Ƚ��������Σ������������Ƿ�ͨ���˾��Ի�����ݲ���ԡ�
		 *
		 * @param right �ڶ������Ρ�
		 * @param absoluteEpsilon ��������Բ��Եľ����ݲĬ��Ϊ 0��
		 * @returns ����� `right` ���ṩ���ݲΧ�ڣ��򷵻� `true`�����򷵻� `false`��
		 */
		bool equalsEpsilon(const Rectangle& right, double absoluteEpsilon = 0.0);

		/**
		* @brief ���㵱ǰ���ε����Ͻǡ�
		*
		* @returns �򷵻�һ���µ� Cartographic ʵ����
		*/
		Cartographic getSouthwest() const {
			return Cartographic(this->west, this->south, 0.0);
		}

		/**
		 * @brief ���㵱ǰ���ε������ǡ�
		 *
		 * @returns ����һ���µ� Cartographic ʵ����
		 */
		Cartographic getNorthwest() const {
			return Cartographic(this->west, this->north, 0.0);
		}

		/**
		 * @brief ���㵱ǰ���εĶ����ǡ�
		 *
		 *  �򷵻�һ���µ� Cartographic ʵ����
		 */
		Cartographic getNortheast() const {
			return Cartographic(this->east, this->north, 0.0);
		}

		/**
		 * @brief ���㵱ǰ���εĶ��Ͻǡ�
		 *
		 * ����һ���µ� Cartographic ʵ����
		 */
		Cartographic getSoutheast() const {
			return Cartographic(this->east, this->south, 0.0);
		}

		/**
		 * @brief ���㵱ǰ���ε����ĵ㡣
		 *
		 * @returns ����һ���µ� Cartographic ʵ����
		 */
		Cartographic getCenter() const;

		/**
		* @brief ���㵱ǰ��������һ�����εĽ�����
		*
		* �ú����ٶ����ε������ǻ��ȱ�ʾ�ľ�γ�ȣ���������ȷ�Ľ�����ͬʱ���ǵ���
		* 1. ��ͬ�ĽǶȿ����ö��ֵ��ʾ�����磬360�Ⱥ�0����ͬһ�����ߣ���
		* 2. �����ڶ��������ߣ��������ߣ�antimeridian�����Ļ������ԡ�
		*
		* @param otherRectangle ��һ�����Σ����ڲ��ҽ�����
		* @returns ����std::optional<Rectangle>
		*/
		std::optional<Rectangle> intersection(const Rectangle& otherRectangle) const;

		/**
		* @brief �����������εļ򵥽�����
		*
		* �� {@link Rectangle::intersection} ��ͬ���˺��������Խ��Ƕ��������һ�µķ�Χ��
		* Ҳ�����ǿ�Խ�������ߵ��������ˣ������������겻ֻ�Ǿ�γ��
		* (���磬ͶӰ����) �ľ��Ρ�
		*
		* @param otherRectangle ��һ�����Σ����ڲ��ҽ�����
		* @returns std::optional<Rectangle>��
		*/
		std::optional<Rectangle> simpleIntersection(const Rectangle& otherRectangle) const;

		/**
		 * @brief ����һ���ܰ����������εĲ������Ρ�
		 *
		 * @param otherRectangle ��һ��Ҫ�����ڲ��������еľ��Ρ�
		 * @returns ����һ���µ� `Rectangle` ʵ����
		 */
		Rectangle computeUnion(const Rectangle& otherRectangle) const;

		/**
		* @brief ͨ����չ��ǰ����ֱ��������������������������µľ��Ρ�
		*
		* @param cartographic Ҫ�����ھ����еĵ�������㡣
		* @returns ����һ���µ� `Rectangle` ʵ����
		*/
		Rectangle expand(const Cartographic& cartographic) const;

		/**
		 * @brief ������������λ�ڻ��ھ����ڲ����򷵻� true�����򷵻� false��
		 *
		 * @param cartographic Ҫ���Եĵ�������㡣
		 * @returns ����ṩ�ĵ���������ھ����ڲ����򷵻� true�����򷵻� false��
		 */
		bool contains(const Cartographic& cartographic) const;

		/**
		 * @brief �Ե�ǰ���ν��в���������һϵ�еѿ�������㣬�����ڴ��ݸ�
		 * {@link BoundingSphere::fromPoints}��
		 * �����Ǳ�Ҫ�ģ��Դ������������Խ����ľ��Ρ�
		 *
		 * @param ellipsoid Ҫʹ�õ������塣Ĭ��Ϊ `Ellipsoid::WGS84`��
		 * @param surfaceHeight �������������Ϸ��ĸ߶ȡ�Ĭ��Ϊ 0.0��
		 * @returns ����һ���µ� `std::vector<glm::dvec3>` ʵ����
		 */
		std::vector<glm::dvec3> subsample(Ellipsoid& ellipsoid = Ellipsoid::WGS84,double surfaceHeight = 0.0) const;

		/**
		 * @brief ���� [0.0, 1.0] ��Χ�ڵĹ�һ�����������ε�һ��������
		 *
		 * @param westLerp �����ֵ���ӣ���Χ [0.0, 1.0]������С�ڻ���� eastLerp��
		 * @param southLerp �ϲ��ֵ���ӣ���Χ [0.0, 1.0]������С�ڻ���� northLerp��
		 * @param eastLerp �����ֵ���ӣ���Χ [0.0, 1.0]��������ڻ���� westLerp��
		 * @param northLerp �����ֵ���ӣ���Χ [0.0, 1.0]��������ڻ���� southLerp��
		 * @returns ����һ���µ� `Rectangle` ʵ����
		 */
		Rectangle subsection(
			double westLerp,
			double southLerp,
			double eastLerp,
			double northLerp
		) const;

	public:
		/**
		  * @brief �����Զ�����ʾ�ı߽羭γ�ȴ���һ�����Ρ�
		  *
		  * @param west_degrees ��������λ�ȣ���Χ [-180.0, 180.0]��Ĭ��Ϊ 0.0��
		  * @param south_degrees ��γ����λ�ȣ���Χ [-90.0, 90.0]��Ĭ��Ϊ 0.0��
		  * @param east_degrees ��������λ�ȣ���Χ [-180.0, 180.0]��Ĭ��Ϊ 0.0��
		  * @param north_degrees ��γ����λ�ȣ���Χ [-90.0, 90.0]��Ĭ��Ϊ 0.0��
		  * @returns ת����ľ��Ρ�����ṩ�� `result`���򷵻ض�������ã����򷵻��´����Ķ���
		  *
		  * @example
		  * // ����һ���� (0, 20) �� (10, 30) �ȵľ���
		  * Rectangle rect = Rectangle::fromDegrees(0.0, 20.0, 10.0, 30.0);
		  */
		static Rectangle fromDegrees(double west_degrees = 0.0,double south_degrees = 0.0,double east_degrees = 0.0,double north_degrees = 0.0);

		/**
		 * @brief �����Ի��ȱ�ʾ�ı߽羭γ�ȴ���һ�����Ρ�
		 *
		 * @param west_radians ��������λ���ȣ���Χ [-��, ��]��Ĭ��Ϊ 0.0��
		 * @param south_radians ��γ����λ���ȣ���Χ [-��/2, ��/2]��Ĭ��Ϊ 0.0��
		 * @param east_radians ��������λ���ȣ���Χ [-��, ��]��Ĭ��Ϊ 0.0��
		 * @param north_radians ��γ����λ���ȣ���Χ [-��/2, ��/2]��Ĭ��Ϊ 0.0��
		 * @returns ת����ľ��Ρ�����ṩ�� `result`���򷵻ض�������ã����򷵻��´����Ķ���
		 *
		 * @example
		 * // ����һ���� (0, ��/4) �� (��/8, 3��/4) ���ȵľ���
		 * Rectangle rect = Rectangle::fromRadians(0.0, std::numbers::pi/4.0, std::numbers::pi/8.0, 3.0*std::numbers::pi/4.0);
		 */
		static Rectangle fromRadians(double west_radians = 0.0,	double south_radians = 0.0,double east_radians = 0.0,double north_radians = 0.0 );
		
        /**
		 * @brief ����һ���ܰ�Χ����������������������λ�õ���С���ܾ��Ρ�
		 *
		 * �˺��������˾��ο��ܿ�Խ�������ڱ���ߣ����ȴ� PI ��Ϊ -PI���������
		 * ȷ���������С��ʵ�ʰ�Χ�С�
		 *
		 * @param cartographics ������������ʵ�����б�ͨ��Ϊ `std::vector<Cartographic>`����
		 * @returns ����һ���µ� `Rectangle` ʵ����
		 */
        static Rectangle fromCartographicArray(const std::vector<Cartographic>& cartographics);

        /**
		 * @brief ����һ���ܰ�Χ�����ѿ�����������������λ�õ���С���ܾ��Ρ�
		 *
		 * �˺������Ƚ����еѿ��������ת������������ϵ��Ȼ����ڵ���������а�Χ�м��㡣
		 * �������˾��ο��ܿ�Խ�������ڱ���ߣ����ȴ� PI ��Ϊ -PI���������
		 * ȷ���������С��ʵ�ʰ�Χ�С�
		 *
		 * @param cartesians �����ѿ�������ʵ�����б�ͨ��Ϊ `std::vector<glm::dvec3>`����
		 * @param ellipsoid �ѿ���������λ�ڵ������塣Ĭ��Ϊ `Ellipsoid::WGS84`��
		 * @returns ����һ���µ� `Rectangle` ʵ����
		 */
		static Rectangle fromCartesianArray(const std::vector<glm::dvec3>& cartesians, Ellipsoid& ellipsoid = Ellipsoid::WGS84);

		/**
		 * @brief �������Ƚ��������Σ������������Ƿ�ͨ���˾��Ի�����ݲ���ԡ�
		 *
		 * @param left ��һ�����Ρ�
		 * @param right �ڶ������Ρ�
		 * @param absoluteEpsilon ��������Բ��Եľ����ݲĬ��Ϊ 0��
		 * @returns ��� `left` �� `right` ���ṩ���ݲΧ�ڣ��򷵻� `true`�����򷵻� `false`��
		 */
		static bool equalsEpsilon(const Rectangle& left, const Rectangle& right, double absoluteEpsilon = 0.0);

		/**
		* @brief �Ƚ��ṩ���������Σ����������ȫ��ȣ��򷵻� `true`��
		*
		* @param left ��һ�����Ρ�
		* @param right �ڶ������Ρ�
		* @returns ��� `left` �� `right` ��ȫ��ȣ��򷵻� `true`�����򷵻� `false`��
		*/
		static bool equals(const Rectangle& left, const Rectangle& right);

		//������������ľ���
		static Rectangle MAX_VALUE;
	public:
		double west; double south; double east; double north;
	};
};