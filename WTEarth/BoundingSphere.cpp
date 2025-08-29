#include "BoundingSphere.h"
#include "Ellipsoid.h"
#include <sstream> 
#include <iomanip>
namespace WT {

	BoundingSphere::BoundingSphere(const std::vector<glm::dvec3>& points)
	{
		// 如果点列表为空，返回一个中心在原点、半径为 0 的包围球。
		if (positions.empty()) {
			return BoundingSphere();
		}

		// 第一步：找到沿每个轴（x、y、z）具有最小和最大分量的点。
		// 这将确定一个初始的候选直径。
		glm::dvec3 xMin = positions[0], yMin = positions[0], zMin = positions[0];
		glm::dvec3 xMax = positions[0], yMax = positions[0], zMax = positions[0];

		for (size_t i = 1; i < positions.size(); ++i) {
			const auto& p = positions[i];
			if (p.x < xMin.x) xMin = p;
			if (p.x > xMax.x) xMax = p;
			if (p.y < yMin.y) yMin = p;
			if (p.y > yMax.y) yMax = p;
			if (p.z < zMin.z) zMin = p;
			if (p.z > zMax.z) zMax = p;
		}

		// 第二步：计算沿 x、y 和 z 轴的跨度（即最小和最大点之间的距离平方）。
		double xSpan = glm::length2(xMax - xMin);
		double ySpan = glm::length2(yMax - yMin);
		double zSpan = glm::length2(zMax - zMin);

		// 确定最大的跨度，其两端点将作为初始 Ritter 算法的直径。
		glm::dvec3 diameter1 = xMin;
		glm::dvec3 diameter2 = xMax;
		double maxSpan = xSpan;

		if (ySpan > maxSpan) {
			maxSpan = ySpan;
			diameter1 = yMin;
			diameter2 = yMax;
		}
		if (zSpan > maxSpan) {
			maxSpan = zSpan;
			diameter1 = zMin;
			diameter2 = zMax;
		}

		// 第三步：基于 Ritter's 算法计算初始包围球。
		glm::dvec3 ritterCenter = (diameter1 + diameter2) * 0.5;
		double ritterRadiusSquared = glm::length2(diameter2 - ritterCenter);
		double ritterRadius = std::sqrt(ritterRadiusSquared);

		// 第四步：计算 Naive（天真）方法的包围球。
		// Naive 方法的中心是最小和最大 x/y/z 分量组成的 AABB 的中点。
		glm::dvec3 minBoxPt(xMin.x, yMin.y, zMin.z);
		glm::dvec3 maxBoxPt(xMax.x, yMax.y, zMax.z);
		glm::dvec3 naiveCenter = (minBoxPt + maxBoxPt) * 0.5;
		double naiveRadius = 0.0;

		// 第五步：第二次遍历所有点，以调整 Ritter 包围球，并计算 Naive 半径。
		for (const auto& p : positions) {
			// 找到离 Naive 中心最远的点，以此计算其半径。
			double r = glm::length(p - naiveCenter);
			if (r > naiveRadius) {
				naiveRadius = r;
			}

			// 调整 Ritter 包围球，以包含所有在当前球体外部的点。
			double oldCenterToPointSquared = glm::length2(p - ritterCenter);
			if (oldCenterToPointSquared > ritterRadiusSquared) {
				double oldCenterToPoint = std::sqrt(oldCenterToPointSquared);
				// 计算新的半径
				ritterRadius = (ritterRadius + oldCenterToPoint) * 0.5;
				ritterRadiusSquared = ritterRadius * ritterRadius;
				// 计算新的中心
				double oldToNew = oldCenterToPoint - ritterRadius;
				ritterCenter = (ritterRadius * ritterCenter + oldToNew * p) / oldCenterToPoint;
			}
		}

		// 第六步：比较两个包围球的半径，选择半径较小的那个。
		if (ritterRadius < naiveRadius) {
			return BoundingSphere(ritterCenter, ritterRadius);
		}
		else {
			return BoundingSphere(naiveCenter, naiveRadius);
		}
	}

	BoundingSphere::BoundingSphere(const AABB& aabb)
	{
		// 包围球的中心就是 AABB 的中心。
		this->center = (aabb.minimum + aabb.maximum) * 0.5;

		// 计算从中心到 AABB 任一角落点的距离，即为包围球的半径。
		// 选择最大点或最小点都可以，因为它们到中心的距离是相同的。
		this->radius = glm::length(aabb.maximum - this->center);

	}

	void BoundingSphere::merge(const BoundingSphere& boundingSphere)
	{
		const glm::dvec3 rightCenter = boundingSphere.center;
		const double rightRadius = boundingSphere.radius;

		// 计算从左球中心到右球中心的向量
		const glm::dvec3 toRightCenter = rightCenter - center;
		// 计算两球中心之间的距离
		const double centerSeparation = glm::length(toRightCenter);

		if (radius >= centerSeparation + rightRadius) {
			return;
		}

		if (rightRadius >= centerSeparation + radius) {
			// 如果右球完全包含左球，则右球就是结果。
			center=rightCenter;
			radius = rightRadius;
			return;
		}

		// 如果两个球体相互重叠或分离，则存在两个切点，每个球体最远端各一个。
		// 计算两个切点之间距离的一半，这就是新包围球的半径。
		const double halfDistanceBetweenTangentPoints = (radius + centerSeparation + rightRadius) * 0.5;

		// 计算新包围球的中心点，它位于两个切点之间的中点。
		// 首先计算一个 t 值，它表示新中心在 `toRightCenter` 向量上的相对位置。
		const double t = (-radius + halfDistanceBetweenTangentPoints) / centerSeparation;
		// 然后，使用 t 值和 `toRightCenter` 向量，从左球中心计算新中心的位置。
		center = center + toRightCenter * t;
		radius= halfDistanceBetweenTangentPoints;
	}

	void BoundingSphere::expand(const glm::dvec3& point)
	{
		// 计算该点到球体中心的距离。
		const double r = glm::length(point - center);
		// 如果该点在当前球体外部，则更新半径以包含该点。
		if (r > radius) {
			radius = r;
		}
	}

};