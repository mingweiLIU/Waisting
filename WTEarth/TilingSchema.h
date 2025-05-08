#pragma once
#include <glm/glm.hpp>
#include "Rectangle.h"

namespace WT {

	class TilingScheme {
	protected:
		int numberOfLevelZeroTilesX;
		int numberOfLevelZeroTilesY;
	public:
		//获取level时X方向的瓦片数
		virtual int getNumberOfXTilesAtLevel(int level) = 0;

		//获取level时Y方向的瓦片数
		virtual int getNumberOfYTilesAtLevel(int level) = 0;

		//将以弧度为单位的矩形范围转换为度为单位
		virtual void rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) = 0;

		//将瓦片转换为度为单位的范围
		virtual void tileXYToNativeRectangle(int tileX, int tileY, int level, Rectangle& nativeRect) = 0;

		//将瓦片转换为弧度为单位的范围
		virtual void tileXYToRectangle(int tileX, int tileY, int level, Rectangle& rect) = 0;

		//计算地理位置在哪个瓦片上
		virtual void positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) = 0;
	};
};