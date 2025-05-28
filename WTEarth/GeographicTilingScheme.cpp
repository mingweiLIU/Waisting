#include "GeographicTilingScheme.h"

namespace WT{

	void GeographicTilingScheme::rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect)
	{
		nativeRect.west = glm::degrees(rect.west);
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

		float xTileWidth = (this->mRectangle.east - this->mRectangle.west)/xTiles;
		float west = tileX * xTileWidth + this->mRectangle.west;
		float east = west + xTileWidth;

		float yTileHeight= (this->mRectangle.north - this->mRectangle.south) / yTiles;
		float north = this->mRectangle.north - tileY * yTileHeight;
		float south = north - yTileHeight;

		rect = Rectangle{ west,south,east,north };
	}

	void GeographicTilingScheme::positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY)
	{
		if (!(mRectangle.west <= radX && radX <= mRectangle.east && mRectangle.south <= radY && radY<=mRectangle.north )) return;

		int xTiles = this->getNumberOfXTilesAtLevel(level);
		int yTiles = this->getNumberOfYTilesAtLevel(level);

		float xTileWidth = (this->mRectangle.east - this->mRectangle.west) / xTiles;
		float yTileHeight = (this->mRectangle.north - this->mRectangle.south) / yTiles;

		tileX = ((radX - this->mRectangle.west) / xTileWidth) ;
		if (tileX >= xTiles) tileX = xTiles - 1;

		tileY = ((this->mRectangle.north - radY) / yTileHeight);
		if (tileY >= yTiles) tileY = yTiles - 1;
	}
}