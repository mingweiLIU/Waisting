#include "WebMercatorTilingSheme.h"
namespace WT {
	WebMercatorTilingSheme::WebMercatorTilingSheme(int numberOfZeroTilesX /*= 1*/, int numberOfZeroTilesY /*= 1*/
		, Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/, Rectangle& rect /*= Rectangle::MAX_VALUE*/)
	{
		this->numberOfLevelZeroTilesX = numberOfZeroTilesX;
        this->numberOfLevelZeroTilesY = numberOfZeroTilesY;
		mProjection = std::make_shared<WebMercatorProjection>(ellipsoid);
        southwest_projected = mProjection->project(rect.getSouthwest());
        // 接着投影矩形的东北角
        northeast_projected = mProjection->project(rect.getNortheast());
        mEllipsoid = ellipsoid;
        mRect = rect;
	}

	//将以弧度为单位的矩形范围转换为度为单位
	Rectangle WebMercatorTilingSheme::rectangleToNativeRectangle(Rectangle rect) {
		// 使用内部的 WebMercatorProjection 实例来执行投影。
		// 首先投影矩形的西南角。height 设为 0，因为矩形转换只关心 X/Y。
		const glm::dvec3 southwest_projected = mProjection->project(rect.getSouthwest());
		// 接着投影矩形的东北角。
		const glm::dvec3 northeast_projected = mProjection->project(rect.getNortheast());

		return Rectangle(southwest_projected.x,southwest_projected.y,northeast_projected.x,northeast_projected.y);	
	}

	Rectangle WebMercatorTilingSheme::tileXYToNativeRectangle(int tileX, int tileY, int level) {
		int xTiles = getNumberOfXTilesAtLevel(level);
		int yTiles = getNumberOfYTilesAtLevel(level);

		float xTileWidth = (northeast_projected.x - southwest_projected.x) / xTiles;
		int west = southwest_projected.x + tileX * xTileWidth;
		int east = southwest_projected.x + (tileX + 1) * xTileWidth;

		float yTileHeight = (northeast_projected.y - southwest_projected.y) / yTiles;
		int north = northeast_projected.y - tileY * yTileHeight;
		int south = northeast_projected.y - (tileY + 1) * yTileHeight;
		return Rectangle(west, south, east, north);
	}


	Rectangle WebMercatorTilingSheme::tileXYToRectangle(int tileX, int tileY, int level) {
		double minLongitude= tileX / (double)(numberOfLevelZeroTilesX << level)  * glm::two_pi<double>() - glm::pi<double>();

		double n = glm::pi<double>() - glm::two_pi<double>() * (tileY+1) / (double)(numberOfLevelZeroTilesY << level);
		double minLatitude = atan(0.5 * (exp(n) - exp(-n)));   // 反双曲正切计算

		double maxLongitude = (tileX+1) / (double)(numberOfLevelZeroTilesX << level) * glm::two_pi<double>() - glm::pi<double>();

		n = glm::pi<double>() - glm::two_pi<double>() * tileY / (double)(numberOfLevelZeroTilesY << level);
		double maxLatitude = atan(0.5 * (exp(n) - exp(-n)));   // 反双曲正切计算

		return Rectangle(minLongitude, minLatitude, maxLongitude, maxLatitude);
	}


	bool WebMercatorTilingSheme::positionToTileXY(double radX, double radY, int level, int& tileX, int& tileY) {
		tileX=(int)(floor((radX + glm::pi<double>()) / glm::two_pi<double>() * (numberOfLevelZeroTilesX << level)));
		tileY=(int)(floor((1.0 - asinh(tan(radY)) / glm::pi<double>()) / 2.0 * (numberOfLevelZeroTilesY << level)));
        return true; // 成功计算并返回
	}
};