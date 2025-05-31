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
		 * @brief 将以地理弧度表示的矩形，转换为此瓦片方案的投影坐标系下的矩形。
		 * 这里的“原生坐标系”指的是 Web 墨卡托投影的米制笛卡尔坐标系。
		 *
		 * @param rectangle 待转换的地理矩形（经纬度以弧度表示）。
		 * @returns 返回新创建的Rectangle对象。
		 */
		Rectangle rectangleToNativeRectangle(Rectangle rect) override;

		//将瓦片转换为度为单位的范围
		Rectangle tileXYToNativeRectangle(int tileX, int tileY, int level) override;

		//将瓦片转换为弧度为单位的范围
		Rectangle tileXYToRectangle(int tileX, int tileY, int level) override;

		//计算地理位置在哪个瓦片上
		 /**
		 * @brief 计算包含给定地理位置的瓦片的 (x, y) 坐标。
		 *
		 * @param radX 位置的经度（弧度）。
		 * @param radY 位置的纬度（弧度）。
		 * @param level 瓦片的细节级别。0 是最不详细的级别。
		 * @param tileX 输出参数，用于存储计算出的磁贴 X 坐标。
		 * @param tileY 输出参数，用于存储计算出的磁贴 Y 坐标。
		 */
		 bool positionToTileXY(double radX, double radY, int level, int& tileX, int& tileY) override;

	private:
		glm::dvec3 southwest_projected ;
		glm::dvec3 northeast_projected ;
	};
}