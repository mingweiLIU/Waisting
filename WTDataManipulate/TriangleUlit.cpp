#include "TriangleUlit.h"
#include "QuadEdge.h"
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

	bool TriangleUlit::rightOf(const glm::dvec2& x, QuadEdgePtr edge)
	{
		return TriangleUlit::rightOf(x, edge->Org(),edge->Dest());
	}

	bool TriangleUlit::leftOf(const glm::dvec2& x, const glm::dvec2& org, const glm::dvec2& dest)
	{
		return TriangleUlit::triCCW(x, org,dest);
	}

	bool TriangleUlit::leftOf(const glm::dvec2& x, QuadEdgePtr edge)
	{
		return TriangleUlit::leftOf(x, edge->Org(), edge->Dest());
	}

	bool TriangleUlit::inCircle(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c, const glm::dvec2& d)
	{
		return (a[0] * a[0] + a[1] * a[1]) * TriangleUlit::triAreaDouble(b, c, d) -
			(b[0] * b[0] + b[1] * b[1]) * TriangleUlit::triAreaDouble(a, c, d) +
			(c[0] * c[0] + c[1] * c[1]) * TriangleUlit::triAreaDouble(a, b, d) -
			(d[0] * d[0] + d[1] * d[1]) * TriangleUlit::triAreaDouble(a, b, c) >
			1e-6;
	}


};