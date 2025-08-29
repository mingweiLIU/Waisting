#pragma once
#include <optional>
#include<string>
#include<glm/glm.hpp>
#include <glm/gtx/vector_ops.hpp>
#include "Ellipsoid.h"

namespace WT {
	/**
	 * @brief 表示一个轴对齐包围盒。
	 * 轴对齐包围盒 (AABB) 是一个与坐标轴对齐的矩形盒子。
	 * 它由其最小和最大角点定义。
	 */
	class AABBox
	{
	public:
		/**
		 * 定义包围盒的最小点。
		 */
		glm::dvec3 minimum;

		/**
		 * 定义包围盒的最大点。
		 */
		glm::dvec3 maximum;

		/**
		 * 包围盒的中心点。
		 */
		glm::dvec3 center;

	public:
		//默认生成一个空的AABB
		AABBox() :minimum(0.0), maximum(0.0), center(0.0) {};
		/**
		 * @brief 使用沿 x、y 和 z 轴的最小和最大点创建一个 AxisAlignedBoundingBox 实例。
		 * @param minimum 沿 x、y 和 z 轴的最小点。默认为 `glm::dvec3(0.0)`。
		 * @param maximum 沿 x、y 和 z 轴的最大点。默认为 `glm::dvec3(0.0)`。
		 */
		AABBox(const glm::dvec3& minimum = glm::dvec3(0.0), const glm::dvec3& maximum = glm::dvec3(0.0));

		/**
		 * @brief 计算一个 AxisAlignedBoundingBox 实例。
		 * 该盒子是通过找到沿 x、y 和 z 轴相距最远的点来确定的。
		 * @param positions 盒子的点列表。
		 * @return 一个新的 AxisAlignedBoundingBox 实例。
		 */
		AABBox(const std::vector<glm::dvec3>& positions);

		//将另外的一个AABB和当前的融合到一起
		void merge(const AABBox& aabb);


		//用另外一个点来扩展AABB
		void expand(const glm::dvec3& expandPoints);

		//用另外一批点来扩展AABB
		void expand(const std::vector<glm::dvec3>& expandPoints);

		/**
		* @brief 将此 AxisAlignedBoundingBox 与提供的 AxisAlignedBoundingBox 进行逐分量比较，
		* 如果它们相等，则返回 `true`，否则返回 `false`。
		* @param right 右侧的 AxisAlignedBoundingBox。
		* @return 如果它们相等，则返回 `true`，否则返回 `false`。
		*/
		bool equals(const AxisAlignedBoundingBox& right) const;

	private:
		glm::dvec3 mini
	};
};