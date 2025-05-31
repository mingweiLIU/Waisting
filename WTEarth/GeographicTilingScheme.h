#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "TilingSchema.h"
#include "Rectangle.h"
#include "Ellipsoid.h"
#include "GeographicProjection.h"
#include "Cartographic.h"

namespace WT{
	class GeographicTilingScheme final :public TilingScheme {
	private:
	public:
		GeographicTilingScheme(Ellipsoid& ellipsoid = Ellipsoid::WGS84, Rectangle rectangle = Rectangle::MAX_VALUE
			, int numberOfLevelZeroTilesX = 2, int numberOfLevelZeroTilesY = 1);

		//将以弧度为单位的矩形范围转换为度为单位
		Rectangle rectangleToNativeRectangle(Rectangle rect) override;

		//将瓦片转换为度为单位的范围
		Rectangle tileXYToNativeRectangle(int tileX, int tileY, int level) override;

		//将瓦片转换为弧度为单位的范围
		Rectangle tileXYToRectangle(int tileX, int tileY, int level) override;

		//计算地理位置在哪个瓦片上
		bool positionToTileXY(double radX, double radY, int level, int& tileX, int& tileY) override;
	};
}