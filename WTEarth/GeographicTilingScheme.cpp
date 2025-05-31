#include "GeographicTilingScheme.h"

namespace WT{
	GeographicTilingScheme::GeographicTilingScheme(Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/, Rectangle rectangle /*= Rectangle::MAX_VALUE*/
		, int numberOfLevelZeroTilesX /*= 2*/, int numberOfLevelZeroTilesY /*= 1*/) {
		this->numberOfLevelZeroTilesX = numberOfLevelZeroTilesX;
		this->numberOfLevelZeroTilesY = numberOfLevelZeroTilesY;
		this->mRect = rectangle;
		this->mEllipsoid = ellipsoid;
		this->mProjection = std::make_shared<GeographicProjection>();
	}

	Rectangle GeographicTilingScheme::rectangleToNativeRectangle(Rectangle rect)
	{
		Rectangle nativeRect;
		nativeRect.west = glm::degrees(rect.west);
		nativeRect.south = glm::degrees(rect.south);
		nativeRect.east = glm::degrees(rect.east);
		nativeRect.north = glm::degrees(rect.north);
		return nativeRect;
	}

	Rectangle GeographicTilingScheme::tileXYToNativeRectangle(int tileX, int tileY, int level)
	{
		Rectangle rect;
		rect=this->tileXYToRectangle(tileX, tileY, level);
		return this->rectangleToNativeRectangle(rect);
	}

	Rectangle GeographicTilingScheme::tileXYToRectangle(int tileX, int tileY, int level)
	{
		int xTiles = this->getNumberOfXTilesAtLevel(level);
		int yTiles = this->getNumberOfYTilesAtLevel(level);

		float xTileWidth = (this->mRect.east - this->mRect.west)/xTiles;
		float west = tileX * xTileWidth + this->mRect.west;
		float east = west + xTileWidth;

		float yTileHeight= (this->mRect.north - this->mRect.south) / yTiles;
		float north = this->mRect.north - tileY * yTileHeight;
		float south = north - yTileHeight;

		return Rectangle{ west,south,east,north };
	}

	bool GeographicTilingScheme::positionToTileXY(double radX, double radY, int level, int& tileX, int& tileY)
	{
		if (!(mRect.west <= radX && radX <= mRect.east && mRect.south <= radY && radY<=mRect.north )) return false;

		int xTiles = this->getNumberOfXTilesAtLevel(level);
		int yTiles = this->getNumberOfYTilesAtLevel(level);

		float xTileWidth = (this->mRect.east - this->mRect.west) / xTiles;
		float yTileHeight = (this->mRect.north - this->mRect.south) / yTiles;

		tileX = ((radX - this->mRect.west) / xTileWidth) ;
		if (tileX >= xTiles) tileX = xTiles - 1;

		tileY = ((this->mRect.north - radY) / yTileHeight);
		if (tileY >= yTiles) tileY = yTiles - 1;
		return true;
	}
}