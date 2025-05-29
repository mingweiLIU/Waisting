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

		int xTileWidth = (northeast_projected.x - southwest_projected.x) / xTiles;
		int west = southwest_projected.x + tileX * xTileWidth;
		int east = southwest_projected.x + (tileX + 1) * xTileWidth;

		int yTileHeight = (northeast_projected.y - southwest_projected.y) / yTiles;
		int north = northeast_projected.y - tileY * yTileHeight;
		int south = northeast_projected.y - (tileY + 1) * yTileHeight;
		return Rectangle(west, south, east, north);
	}


	Rectangle WebMercatorTilingSheme::tileXYToRectangle(int tileX, int tileY, int level) {
		Rectangle nativeRect = tileXYToNativeRectangle(tileX, tileY, level);
		Cartographic soutwest= this->mProjection->unproject(glm::dvec3(nativeRect.west, nativeRect.south, 0));
		Cartographic northeast = this->mProjection->unproject(glm::dvec3(nativeRect.east, nativeRect.north, 0));

		return Rectangle(soutwest.longitude,soutwest.latitude,northeast.longitude,northeast.latitude);
	}


	bool WebMercatorTilingSheme::positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) {
        // 创建 Cartographic 对象用于内部计算
        Cartographic position(static_cast<double>(radX), static_cast<double>(radY), 0.0); // 假设高度为 0.0

        // 检查位置是否在瓦片方案的矩形边界内
        if (!this->mRect.contains(position)) {
            // 如果位置超出边界，则返回 false，不修改 tileX 和 tileY
            return false;
        }

        // 获取指定级别下 X 和 Y 方向的瓦片数量
        const int xTiles = getNumberOfXTilesAtLevel(level);
        const int yTiles = getNumberOfYTilesAtLevel(level);

        // 计算整个投影范围的宽度和高度（以米为单位）
        int xTileWidth = (northeast_projected.x - southwest_projected.x) / xTiles;
        int yTileHeight = (northeast_projected.y - southwest_projected.y) / yTiles;

        // 将地理位置投影到 Web 墨卡托米制坐标
        const glm::dvec3 webMercatorPosition = mProjection->project(position);

        // 计算投影点与方案西边界和北边界的距离
        // distanceFromWest: 从西边界到点的 X 距离
        const double distanceFromWest = webMercatorPosition.x - southwest_projected.x;
        // distanceFromNorth: 从北边界到点的 Y 距离 (注意 Y 轴方向，通常 Web 墨卡托的 Y 轴向上，但瓦片系统从北向下)
        const double distanceFromNorth = northeast_projected.y - webMercatorPosition.y;

        // 计算瓦片的 X 坐标
        // 使用 static_cast<int> 进行向下取整
        int xTileCoordinate = static_cast<int>(distanceFromWest / xTileWidth);
        // 边界检查：确保 X 坐标不会超出最大瓦片数量
        if (xTileCoordinate >= xTiles) {
            xTileCoordinate = xTiles - 1;
        }
        else if (xTileCoordinate < 0) { // 确保不会是负数，虽然通常在 contains 检查后不会发生
            xTileCoordinate = 0;
        }

        // 计算瓦片的 Y 坐标
        int yTileCoordinate = static_cast<int>(distanceFromNorth / yTileHeight);
        // 边界检查：确保 Y 坐标不会超出最大瓦片数量
        if (yTileCoordinate >= yTiles) {
            yTileCoordinate = yTiles - 1;
        }
        else if (yTileCoordinate < 0) { // 确保不会是负数
            yTileCoordinate = 0;
        }

        // 通过引用参数返回结果
        tileX = xTileCoordinate;
        tileY = yTileCoordinate;
        return true; // 成功计算并返回
	}
};