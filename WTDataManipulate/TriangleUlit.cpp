#include "TriangleUlit.h"

namespace WT {

	double TriangleUlit::triAreaDouble(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c)
	{
		return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
	}

	bool TriangleUlit::triCCW(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c)
	{
		return TriangleUlit::triAreaDouble(a, b, c) > 0;
	}

	bool TriangleUlit::rightOf(const glm::dvec2& x, const glm::dvec2& org, const glm::dvec2& dest)
	{
		return TriangleUlit::triCCW(x, dest, org);
	}

	bool TriangleUlit::leftOf(const glm::dvec2& x, const glm::dvec2& org, const glm::dvec2& dest)
	{
		return TriangleUlit::triCCW(x, org,dest);
	}

	bool TriangleUlit::inCircle(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c, const glm::dvec2& d)
	{
		return (a[0] * a[0] + a[1] * a[1]) * TriangleUlit::triAreaDouble(b, c, d) -
			(b[0] * b[0] + b[1] * b[1]) * TriangleUlit::triAreaDouble(a, c, d) +
			(c[0] * c[0] + c[1] * c[1]) * TriangleUlit::triAreaDouble(a, b, d) -
			(d[0] * d[0] + d[1] * d[1]) * TriangleUlit::triAreaDouble(a, b, c) >
			1e-6;
	}

	Plane::init(const glm::dvec3& p, const glm::dvec3& q, const glm::dvec3& r) noexcept
	{
		// We explicitly declare these (rather than putting them in a
		// Vector) so that they can be allocated into registers.
		const double ux = q.x - p.x;
		const double uy = q.y - p.y;
		const double uz = q.z - p.z;

		const double vx = r.x - p.x;
		const double vy = r.y - p.y;
		const double vz = r.z - p.z;

		const double den = ux * vy - uy * vx;

		const double _a = (uz * vy - uy * vz) / den;
		const double _b = (ux * vz - uz * vx) / den;

		a = _a;
		b = _b;
		c = p.z - _a * p.x - _b * p.y;
	}

};