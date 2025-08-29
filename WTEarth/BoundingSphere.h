#pragma once
#include <optional>
#include<string>
#include<glm/glm.hpp>
#include "Ellipsoid.h"
#include "AABBox.h"

namespace WT {
	class BoundingSphere
	{
	public:
		/**
		 * @brief 默认构造函数。
		 * 创建一个中心位于原点，半径为 0 的包围球。
		 */
		BoundingSphere() : center(0.0), radius(0.0) {}
		//外部指定中心和半径
		BoundingSphere(glm::dvec3& center, double radius) :center(center), radius(radius) {};
		/**
		 * @brief 从一组点中计算一个包围球。
		 * 此方法使用优化的算法来找到一个紧密贴合的包围球。
		 *
		 * @param positions 包含包围球将要包围的点的列表。
		 */
		BoundingSphere(const std::vector<glm::dvec3>& points);
		/**
		 * @brief 从一个轴对齐包围盒 (AABB) 创建一个包围球。
		 * @param aabb 用于创建包围球的轴对齐包围盒。
		 */
		BoundingSphere(const AABB& aabb);

		/**
		 * @brief 将一个包围球和当前包围球合并
		 *
		 * @param 要被包含在包围球中的球体
		 */
		void merge(const BoundingSphere& boundingSphere);

		//扩展包围球
		void expand(const glm::dvec3& point);

	public:
		glm::dvec3 center;
		double radius;
	};
};