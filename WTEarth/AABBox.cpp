#include "AABBox.h"
#include "Ellipsoid.h"
#include <sstream> 
#include <iomanip>
namespace WT {
	AABBox::AABBox(const glm::dvec3& minimum /*= glm::dvec3(0.0)*/, const glm::dvec3& maximum /*= glm::dvec3(0.0)*/)
		:minimum(minimum),maximum(maximum)
	{
		center = (minimum + maximum) *0.5;
	}

	AABBox::AABBox(const std::vector<glm::dvec3>& positions)
	{
		if (positions.empty()) {
			return AABBox();
		}

		glm::dvec3 minimum = positions[0];
		glm::dvec3 maximum = positions[0];

		for (size_t i = 1; i < positions.size(); ++i) {
			minimum = glm::min(minimum, positions[i]);
			maximum = glm::max(maximum, positions[i]);
		}
	}

	void AABBox::union(const AABBox& aabb)
	{
		if (minimum>aabb.minimum)
		{
			minimum = aabb.minimum;
		}
		if (maximum<aabb.maximum)
		{
			maximum=aabb.maximum
		}
		center = (minimum + maximum) * 0.5;
	}

	void AABBox::expand(const std::vector<glm::dvec3>& expandPoints)
	{
		for (glm::dvec3& oneNewPoint)
		{
			if (oneNewPoint<minimum)
			{
				minimum = oneNewPoint;
				continue;
			}
			if (oneNewPoint>maximum)
			{
				maximum = maximum;
			}			
		}
		center = (minimum + maximum) * 0.5;
	}

	void AABBox::expand(const glm::dvec3& expandPoints)
	{
		if (oneNewPoint < minimum)
		{
			minimum = oneNewPoint;
		}
		if (oneNewPoint > maximum)
		{
			maximum = maximum;
		}
	    center = (minimum + maximum) * 0.5;
	}

	bool AABBox::equals(const AxisAlignedBoundingBox& right) const
	{
		// 使用 glm::all 和 glm::equal 进行逐分量比较，以处理浮点数精度问题
		return (glm::all(glm::equal(this->minimum, right.minimum)) &&
			glm::all(glm::equal(this->maximum, right.maximum)) &&
			glm::all(glm::equal(this->center, right.center)));
	}

};