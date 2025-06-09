#pragma once
#include <glm/glm.hpp>

namespace WT {
	class TriangleUlit
	{
	public:
		//�����Ѿ�����һ��˳����֯����������������� �������ֵΪ���� ��������Ϊccw
		static double triAreaDouble(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c);

		static bool triCCW(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c);

		//�жϵ�x�ڱߣ�org--dest)���Ҳ�
		static bool rightOf(const glm::dvec2& x, const glm::dvec2& org, const glm::dvec2& dest);

		//�жϵ�x�ڱߣ�org--dest)�����
		static bool leftOf(const glm::dvec2& x, const glm::dvec2& org, const glm::dvec2& dest);
	
		//�жϵ�d�Ƿ�����a��b��c��������ɵ�Բ��
		static bool inCircle(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c, const glm::dvec2& d);
	};
};