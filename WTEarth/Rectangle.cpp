#include "Rectangle.h"

namespace WT {
	Rectangle Rectangle::MAX_VALUE = Rectangle(-glm::pi<float>(), -glm::pi<float>() / 2, glm::pi<float>(), glm::pi<float>() / 2);

	double Rectangle::computeWidth() {
		if (east<west)
		{
			east += (glm::pi<float>() * 2.0);
		}
		return (east - west);
	}
	double Rectangle::computeHeight() {
		return (north - south);
	}
};