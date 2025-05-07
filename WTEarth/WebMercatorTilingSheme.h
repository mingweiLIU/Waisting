#pragma once
#include "TilingSchema.h"
#include "Rectangle.h"
namespace WT {
	class WebMercatorTilingSheme :public TilingScheme {
	public:
		WebMercatorTilingSheme(int numberOfZeroTilesX = 1, int numberOfZeroTilesY = 1) :
			numberOfLevelZeroTilesX(numberOfLevelZeroTilesX),
			numberOfLevelZeroTilesY(numberOfLevelZeroTilesY) {}


	};
}