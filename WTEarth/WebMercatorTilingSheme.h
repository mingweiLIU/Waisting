#pragma once
#include "TilingSchema.h"
#include "Rectangle.h"
#include "Ellipsoid.h"
#include "WebMercatorProjection.h"
namespace WT {
	class WebMercatorTilingSheme final :public TilingScheme {
	public:
		WebMercatorTilingSheme(int numberOfZeroTilesX = 1, int numberOfZeroTilesY = 1
			,Ellipsoid& ellipsoid= Ellipsoid::WGS84,Rectangle& rect=Rectangle::MAX_VALUE);

		//将以弧度为单位的矩形范围转换为度为单位
		void rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) override;

		//将瓦片转换为度为单位的范围
		virtual void tileXYToNativeRectangle(int tileX, int tileY, int level, Rectangle& nativeRect) = 0;

		//将瓦片转换为弧度为单位的范围
		virtual void tileXYToRectangle(int tileX, int tileY, int level, Rectangle& rect) = 0;

		//计算地理位置在哪个瓦片上
		virtual void positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) = 0;
	private:
		Ellipsoid mEllipsoid;
		Rectangle mRect;
		WebMercatorProjection mProjection;
	};
}