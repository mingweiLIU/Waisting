#pragma once
#include <glm/glm.hpp>
#include "EntityPool.h"

namespace WT {
	class QuadEdge;
	using QuadEdgePtr = pool_ptr<QuadEdge>;
	class TriangleUlit
	{
	public:
		//�����Ѿ�����һ��˳����֯����������������� �������ֵΪ���� ��������Ϊccw
		static double triAreaDouble(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c);

		static bool triCCW(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c);

		//�жϵ�x�ڱߣ�org--dest)���Ҳ�
		static bool rightOf(const glm::dvec2& x, const glm::dvec2& org, const glm::dvec2& dest);
		static bool rightOf(const glm::dvec2& x, QuadEdgePtr edge);
		

		//�жϵ�x�ڱߣ�org--dest)�����
		static bool leftOf(const glm::dvec2& x, const glm::dvec2& org, const glm::dvec2& dest);
		static bool leftOf(const glm::dvec2& x, QuadEdgePtr edge);
	
		//�жϵ�d�Ƿ�����a��b��c��������ɵ�Բ��
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
		inline void init(const glm::dvec3& p, const glm::dvec3& q, const glm::dvec3& r) noexcept {
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
		double eval(double x, double y) const noexcept {
			return a * x + b * y + c;
		}
		double eval(int x, int y)const noexcept {
			return a * x + b * y + c;
		}
	};
};