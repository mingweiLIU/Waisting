#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "TilingSchema.h"
#include "Rectangle.h"

namespace WT{
	class GeographicTilingScheme final :public TilingScheme {
	private:
		Rectangle mRectangle;
	public:
		GeographicTilingScheme(Rectangle rectangle=Rectangle::MAX_VALUE,int numberOfLevelZeroTilesX=2, int numberOfLevelZeroTilesY = 1)
			:mRectangle(rectangle){
			this->numberOfLevelZeroTilesX = numberOfLevelZeroTilesX;
			this->numberOfLevelZeroTilesY = numberOfLevelZeroTilesY;
		}

		//将以弧度为单位的矩形范围转换为度为单位
		void rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) override;

		//将瓦片转换为度为单位的范围
		void tileXYToNativeRectangle(int tileX, int tileY, int level, Rectangle& nativeRect) override;

		//将瓦片转换为弧度为单位的范围
		void tileXYToRectangle(int tileX, int tileY, int level, Rectangle& rect) override;

		//计算地理位置在哪个瓦片上
		void positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) override;
	};
}