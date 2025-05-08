#pragma once
#include<glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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
		//bool equals(Rectangle& rect)const;
		//bool equalsEpsilon(Rectangle& rect,double epsilon=0.00000001)const;
		//
		//static Rectangle fromDegrees(double west, double south, double east, double north);
		//static Rectangle fromRadians(double west, double south, double east, double north);
		//static Rectangle fromCartographicArray(double west, double south, double east, double north);
		//static Rectangle fromCartesianArray(double west, double south, double east, double north);

		//定义地球上最大的矩形
		static Rectangle MAX_VALUE;
	public:
		double west; double south; double east; double north;
	};
};