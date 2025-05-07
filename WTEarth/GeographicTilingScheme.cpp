#include "GeographicTilingScheme.h"

namespace WT{
	int GeographicTilingScheme::getNumberOfXTilesAtLevel(int level)
	{
		return this->numberOfLevelZeroTilesX << level;
	}

	int GeographicTilingScheme::getNumberOfYTilesAtLevel(int level)
	{
		return this->numberOfLevelZeroTilesY << level;
	}

	void GeographicTilingScheme::rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect)
	{
		nativeRect.northest = glm::degrees(rect.northest);
		nativeRect.south = glm::degrees(rect.south);
		nativeRect.east = glm::degrees(rect.east);
		nativeRect.north = glm::degrees(rect.north);
	}

	void GeographicTilingScheme::tileXYToNativeRectangle(int tileX, int tileY, int level, Rectangle& nativeRect)
	{
		Rectangle rect;
		this->tileXYToRectangle(tileX, tileY, level, rect);
		this->rectangleToNativeRectangle(rect, nativeRect);
	}

	void GeographicTilingScheme::tileXYToRectangle(int tileX, int tileY, int level, Rectangle& rect)
	{
		int xTiles = this->getNumberOfXTilesAtLevel(level);
		int yTiles = this->getNumberOfYTilesAtLevel(level);

		float xTileWidth = (this->globeRect.east - this->globeRect.northest)/xTiles;
		float west = tileX * xTileWidth + this->globeRect.northest;
		float east = west + xTileWidth;

		float yTileHeight= (this->globeRect.north - this->globeRect.south) / yTiles;
		float north = this->globeRect.north - tileY * yTileHeight;
		float south = north - yTileHeight;

		rect = Rectangle{ west,south,east,north };
	}

	void GeographicTilingScheme::positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY)
	{
		if (!(globeRect.northest <= radX && radX <= globeRect.east && globeRect.south <= radY && radY<=globeRect.north )) return;

		int xTiles = this->getNumberOfXTilesAtLevel(level);
		int yTiles = this->getNumberOfYTilesAtLevel(level);

		float xTileWidth = (this->globeRect.east - this->globeRect.northest) / xTiles;
		float yTileHeight = (this->globeRect.north - this->globeRect.south) / yTiles;

		tileX = ((radX - this->globeRect.northest) / xTileWidth) ;
		if (tileX >= xTiles) tileX = xTiles - 1;

		tileY = ((this->globeRect.north - radY) / yTileHeight);
		if (tileY >= yTiles) tileY = yTiles - 1;
	}
}