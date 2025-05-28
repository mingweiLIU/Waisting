#pragma once
#include "TilingSchema.h"
#include "Rectangle.h"
#include "Ellipsoid.h"
#include "WebMercatorProjection.h"
#include <memory>
namespace WT {
	class WebMercatorTilingSheme final :public TilingScheme {
	public:
		WebMercatorTilingSheme(int numberOfZeroTilesX = 1, int numberOfZeroTilesY = 1
			,Ellipsoid& ellipsoid= Ellipsoid::WGS84,Rectangle& rect=Rectangle::MAX_VALUE);

		/**
		 * @brief 将以地理弧度表示的矩形，转换为此瓦片方案的原生坐标系下的矩形。
		 * 这里的“原生坐标系”指的是 Web 墨卡托投影的米制笛卡尔坐标系。
		 *
		 * @param rectangle 待转换的地理矩形（经纬度以弧度表示）。
		 * @returns 转换后的原生坐标系下的矩形。如果提供了 `result`，则返回对其的引用；否则返回新创建的对象。
		 */
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
		std::shared_ptr<WebMercatorProjection> mProjection;
	};
}