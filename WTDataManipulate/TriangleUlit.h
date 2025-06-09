#pragma once
#include <glm/glm.hpp>

namespace WT {
	class TriangleUlit
	{
	public:
		//返回已经按照一定顺序组织的三角形面积的两倍 如果返回值为正数 则三角形为ccw
		static double triAreaDouble(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c);

		static bool triCCW(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c);

		//判断点x在边（org--dest)的右侧
		static bool rightOf(const glm::dvec2& x, const glm::dvec2& org, const glm::dvec2& dest);

		//判断点x在边（org--dest)的左侧
		static bool leftOf(const glm::dvec2& x, const glm::dvec2& org, const glm::dvec2& dest);
	
		//判断点d是否在由a、b、c三个点组成的圆内
		static bool inCircle(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c, const glm::dvec2& d);
	};

	class Line {
	private:
		double a, b, c;
	public:
		Line(const glm::dvec2& p, const glm::dvec2& q) {
			const glm::dvec2 t = q - p;
			double l = t.length();

			a = t.y / l;
			b = -t.x / l;
			c = -(a * p.x + b * p.y);
		}
		inline double eval(const glm::dvec2& p)const noexcept {
			return (a * p.x + b * p.y + c);
		}
	};

	class Plane {
	public:
		double a, b, c;
		Plane() = default;
		Plane(const glm::dvec3& p, const glm::dvec3& q, const glm::dvec3& r) { init(p, q, r); }
		inline init(const glm::dvec3& p, const glm::dvec3& q, const glm::dvec3& r) noexcept;
		double eval(double x, double y) const noexcept {
			return a * x + b * y + c;
		}
		double eval(int x, int y)const noexcept {
			return a * x + b * y + c;
		}
	};
};