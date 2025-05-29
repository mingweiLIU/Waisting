#pragma once
#include<memory>
#include <glm/glm.hpp>
#include "Rectangle.h"
#include "Ellipsoid.h"
#include "Projection.h"

namespace WT {

	class TilingScheme {
	protected:
		int numberOfLevelZeroTilesX;
		int numberOfLevelZeroTilesY;
		Ellipsoid mEllipsoid;
		Rectangle mRect;
		std::shared_ptr<Projection> mProjection;
	public:
		//获取level时X方向的瓦片数
		virtual int getNumberOfXTilesAtLevel(int level) {
			return numberOfLevelZeroTilesX << level;
		}

		//获取level时Y方向的瓦片数
		virtual int getNumberOfYTilesAtLevel(int level) {
			return numberOfLevelZeroTilesY << level;
		}

		/**
		* @brief 获取瓦片方案所基于的椭球体。
		* @returns 对 Ellipsoid 对象的常量引用。
		*/
		const Ellipsoid& getEllipsoid() const {
			return mEllipsoid;
		}

		/**
		 * @brief 获取瓦片方案覆盖的地理范围。
		 * @returns 对 Rectangle 对象的常量引用。
		 */
		const Rectangle& getRectangle() const {
			return mRect;
		}

		//将以弧度为单位的矩形范围转换为m为单位
		virtual Rectangle rectangleToNativeRectangle(Rectangle rect) = 0;

		//将瓦片转换为m为单位的范围
		virtual Rectangle tileXYToNativeRectangle(int tileX, int tileY, int level) = 0;

		//将瓦片转换为弧度为单位的范围
		virtual Rectangle tileXYToRectangle(int tileX, int tileY, int level) = 0;

		//计算地理位置在哪个瓦片上
		virtual bool positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) = 0;

		/////////////////////////下面为切片计算用的函数/////////////////////////////////////////
		// 给定Rectanle计算其瓦片范围
		virtual Rectangle rectangle2TileXYRange(Rectangle& rectToCalc,int level) {
			Cartographic minXYPos = rectToCalc.getSouthwest();
			Cartographic maxXYPos = rectToCalc.getNortheast();

			//把坐标转为弧度
			minXYPos.longitude = glm::radians(minXYPos.longitude);
			minXYPos.latitude = glm::radians(minXYPos.latitude);
			maxXYPos.longitude = glm::radians(maxXYPos.longitude);
			maxXYPos.latitude = glm::radians(maxXYPos.latitude);

			//
			int minTileX, minTileY, maxTileX, maxTileY;
			positionToTileXY(minXYPos.longitude, minXYPos.latitude, level, minTileX, minTileY);
			positionToTileXY(maxXYPos.longitude, maxXYPos.latitude, level, maxTileX, maxTileY);
			return Rectangle(minTileX, minTileY, maxTileX, maxTileY);
		}

		//给定瓦片坐标 转为其范围（单位为度）
		virtual Rectangle tileXYToDegreeRectangle(int tileX, int tileY, int level) {
			Rectangle radRect = tileXYToRectangle(tileX, tileY, level);
			radRect.east = glm::degrees(radRect.east);
			radRect.west = glm::degrees(radRect.west);
			radRect.south = glm::degrees(radRect.south);
			radRect.north = glm::degrees(radRect.north);
			return radRect;
		}

		//给定经纬度（单位度）计算瓦片坐标
		virtual bool positionDegreeToTileXY(double degreeX, double degreeY, int level, int& tileX, int& tileY) {
			return positionToTileXY(glm::radians(degreeX), glm::radians(degreeY), level, tileX, tileY);
		}

		//给定层级计算像素数
		virtual double mapSize(int level, int tileSize) {
			return  std::ceil(tileSize * (double)(numberOfLevelZeroTilesX << level));
		}

		//计算给定纬度（单位度）下地面分辨率
		virtual double groundResolution(double latitude, double level, int tileSize) {
			return cos(latitude * glm::pi<double>() / 180) * glm::two_pi<double>() * mEllipsoid.getMaximumRadius() / mapSize(level, tileSize);
		}

		//计算给定分辨率影像的合适切片层级
		virtual int getPoperLevel(double groundResolution, int tileSize) {
			const double EARTH_CIRCUMFERENCE = 40075016.686;  // 地球赤道周长（米）

			// OSM 在 zoom=0 时的分辨率（米/像素）
			const double RESOLUTION_0 = EARTH_CIRCUMFERENCE / (numberOfLevelZeroTilesX * tileSize);

			// 计算最佳 zoom（理论值可能是小数）
			double zoom = std::log2(RESOLUTION_0 / groundResolution);

			// 取整（向下取整，避免分辨率过高）
			int bestZoom = static_cast<int>(std::floor(zoom));

			return bestZoom;
		}
	};
};