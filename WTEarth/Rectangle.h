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
		* @brief 比较提供的两个矩形，如果它们完全相等，则返回 `true`。
		*
		* @param right 第二个矩形。
		* @returns 如果和 `right` 完全相等，则返回 `true`；否则返回 `false`。
		*/
		bool equals(const Rectangle& right); 

		/**
		 * @brief 按分量比较两个矩形，并返回它们是否通过了绝对或相对容差测试。
		 *
		 * @param right 第二个矩形。
		 * @param absoluteEpsilon 用于相等性测试的绝对容差。默认为 0。
		 * @returns 如果和 `right` 在提供的容差范围内，则返回 `true`；否则返回 `false`。
		 */
		bool equalsEpsilon(const Rectangle& right, double absoluteEpsilon = 0.0);

		/**
		* @brief 计算当前矩形的西南角。
		*
		* @returns 则返回一个新的 Cartographic 实例。
		*/
		Cartographic getSouthwest() const {
			return Cartographic(this->west, this->south, 0.0);
		}

		/**
		 * @brief 计算当前矩形的西北角。
		 *
		 * @returns 返回一个新的 Cartographic 实例。
		 */
		Cartographic getNorthwest() const {
			return Cartographic(this->west, this->north, 0.0);
		}

		/**
		 * @brief 计算当前矩形的东北角。
		 *
		 *  则返回一个新的 Cartographic 实例。
		 */
		Cartographic getNortheast() const {
			return Cartographic(this->east, this->north, 0.0);
		}

		/**
		 * @brief 计算当前矩形的东南角。
		 *
		 * 返回一个新的 Cartographic 实例。
		 */
		Cartographic getSoutheast() const {
			return Cartographic(this->east, this->south, 0.0);
		}

		/**
		 * @brief 计算当前矩形的中心点。
		 *
		 * @returns 返回一个新的 Cartographic 实例。
		 */
		Cartographic getCenter() const;

		/**
		* @brief 计算当前矩形与另一个矩形的交集。
		*
		* 该函数假定矩形的坐标是弧度表示的经纬度，并生成正确的交集，同时考虑到：
		* 1. 相同的角度可以用多个值表示（例如，360度和0度是同一个经线）。
		* 2. 经度在对向子午线（反子午线，antimeridian）处的环绕特性。
		*
		* @param otherRectangle 另一个矩形，用于查找交集。
		* @returns 返回std::optional<Rectangle>
		*/
		std::optional<Rectangle> intersection(const Rectangle& otherRectangle) const;

		/**
		* @brief 计算两个矩形的简单交集。
		*
		* 与 {@link Rectangle::intersection} 不同，此函数不尝试将角度坐标放入一致的范围，
		* 也不考虑跨越反子午线的情况。因此，它可用于坐标不只是经纬度
		* (例如，投影坐标) 的矩形。
		*
		* @param otherRectangle 另一个矩形，用于查找交集。
		* @returns std::optional<Rectangle>；
		*/
		std::optional<Rectangle> simpleIntersection(const Rectangle& otherRectangle) const;

		/**
		 * @brief 计算一个能包含两个矩形的并集矩形。
		 *
		 * @param otherRectangle 另一个要包含在并集矩形中的矩形。
		 * @returns 返回一个新的 `Rectangle` 实例。
		 */
		Rectangle computeUnion(const Rectangle& otherRectangle) const;

		/**
		* @brief 通过扩展当前矩形直到包含给定地理坐标点来计算新的矩形。
		*
		* @param cartographic 要包含在矩形中的地理坐标点。
		* @returns 返回一个新的 `Rectangle` 实例。
		*/
		Rectangle expand(const Cartographic& cartographic) const;

		/**
		 * @brief 如果地理坐标点位于或在矩形内部，则返回 true，否则返回 false。
		 *
		 * @param cartographic 要测试的地理坐标点。
		 * @returns 如果提供的地理坐标点在矩形内部，则返回 true，否则返回 false。
		 */
		bool contains(const Cartographic& cartographic) const;

		/**
		 * @brief 对当前矩形进行采样，生成一系列笛卡尔坐标点，适用于传递给
		 * {@link BoundingSphere::fromPoints}。
		 * 采样是必要的，以处理覆盖两极或跨越赤道的矩形。
		 *
		 * @param ellipsoid 要使用的椭球体。默认为 `Ellipsoid::WGS84`。
		 * @param surfaceHeight 矩形在椭球体上方的高度。默认为 0.0。
		 * @returns 返回一个新的 `std::vector<glm::dvec3>` 实例。
		 */
		std::vector<glm::dvec3> subsample(Ellipsoid& ellipsoid = Ellipsoid::WGS84,double surfaceHeight = 0.0) const;

		/**
		 * @brief 根据 [0.0, 1.0] 范围内的归一化坐标计算矩形的一个子区域。
		 *
		 * @param westLerp 西侧插值因子，范围 [0.0, 1.0]。必须小于或等于 eastLerp。
		 * @param southLerp 南侧插值因子，范围 [0.0, 1.0]。必须小于或等于 northLerp。
		 * @param eastLerp 东侧插值因子，范围 [0.0, 1.0]。必须大于或等于 westLerp。
		 * @param northLerp 北侧插值因子，范围 [0.0, 1.0]。必须大于或等于 southLerp。
		 * @returns 返回一个新的 `Rectangle` 实例。
		 */
		Rectangle subsection(
			double westLerp,
			double southLerp,
			double eastLerp,
			double northLerp
		) const;

	public:
		/**
		  * @brief 根据以度数表示的边界经纬度创建一个矩形。
		  *
		  * @param west_degrees 西经，单位度，范围 [-180.0, 180.0]。默认为 0.0。
		  * @param south_degrees 南纬，单位度，范围 [-90.0, 90.0]。默认为 0.0。
		  * @param east_degrees 东经，单位度，范围 [-180.0, 180.0]。默认为 0.0。
		  * @param north_degrees 北纬，单位度，范围 [-90.0, 90.0]。默认为 0.0。
		  * @returns 转换后的矩形。如果提供了 `result`，则返回对其的引用；否则返回新创建的对象。
		  *
		  * @example
		  * // 创建一个从 (0, 20) 到 (10, 30) 度的矩形
		  * Rectangle rect = Rectangle::fromDegrees(0.0, 20.0, 10.0, 30.0);
		  */
		static Rectangle fromDegrees(double west_degrees = 0.0,double south_degrees = 0.0,double east_degrees = 0.0,double north_degrees = 0.0);

		/**
		 * @brief 根据以弧度表示的边界经纬度创建一个矩形。
		 *
		 * @param west_radians 西经，单位弧度，范围 [-π, π]。默认为 0.0。
		 * @param south_radians 南纬，单位弧度，范围 [-π/2, π/2]。默认为 0.0。
		 * @param east_radians 东经，单位弧度，范围 [-π, π]。默认为 0.0。
		 * @param north_radians 北纬，单位弧度，范围 [-π/2, π/2]。默认为 0.0。
		 * @returns 转换后的矩形。如果提供了 `result`，则返回对其的引用；否则返回新创建的对象。
		 *
		 * @example
		 * // 创建一个从 (0, π/4) 到 (π/8, 3π/4) 弧度的矩形
		 * Rectangle rect = Rectangle::fromRadians(0.0, std::numbers::pi/4.0, std::numbers::pi/8.0, 3.0*std::numbers::pi/4.0);
		 */
		static Rectangle fromRadians(double west_radians = 0.0,	double south_radians = 0.0,double east_radians = 0.0,double north_radians = 0.0 );
		
        /**
		 * @brief 创建一个能包围给定地理坐标数组中所有位置的最小可能矩形。
		 *
		 * 此函数考虑了矩形可能跨越国际日期变更线（经度从 PI 变为 -PI）的情况，
		 * 确保计算出最小的实际包围盒。
		 *
		 * @param cartographics 包含地理坐标实例的列表（通常为 `std::vector<Cartographic>`）。
		 * @returns 返回一个新的 `Rectangle` 实例。
		 */
        static Rectangle fromCartographicArray(const std::vector<Cartographic>& cartographics);

        /**
		 * @brief 创建一个能包围给定笛卡尔坐标数组中所有位置的最小可能矩形。
		 *
		 * 此函数首先将所有笛卡尔坐标点转换到地理坐标系，然后基于地理坐标进行包围盒计算。
		 * 它考虑了矩形可能跨越国际日期变更线（经度从 PI 变为 -PI）的情况，
		 * 确保计算出最小的实际包围盒。
		 *
		 * @param cartesians 包含笛卡尔坐标实例的列表（通常为 `std::vector<glm::dvec3>`）。
		 * @param ellipsoid 笛卡尔坐标所位于的椭球体。默认为 `Ellipsoid::WGS84`。
		 * @returns 返回一个新的 `Rectangle` 实例。
		 */
		static Rectangle fromCartesianArray(const std::vector<glm::dvec3>& cartesians, Ellipsoid& ellipsoid = Ellipsoid::WGS84);

		/**
		 * @brief 按分量比较两个矩形，并返回它们是否通过了绝对或相对容差测试。
		 *
		 * @param left 第一个矩形。
		 * @param right 第二个矩形。
		 * @param absoluteEpsilon 用于相等性测试的绝对容差。默认为 0。
		 * @returns 如果 `left` 和 `right` 在提供的容差范围内，则返回 `true`；否则返回 `false`。
		 */
		static bool equalsEpsilon(const Rectangle& left, const Rectangle& right, double absoluteEpsilon = 0.0);

		/**
		* @brief 比较提供的两个矩形，如果它们完全相等，则返回 `true`。
		*
		* @param left 第一个矩形。
		* @param right 第二个矩形。
		* @returns 如果 `left` 和 `right` 完全相等，则返回 `true`；否则返回 `false`。
		*/
		static bool equals(const Rectangle& left, const Rectangle& right);

		//定义地球上最大的矩形
		static Rectangle MAX_VALUE;
	public:
		double west; double south; double east; double north;
	};
};