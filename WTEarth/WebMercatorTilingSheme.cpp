#include "WebMercatorTilingSheme.h"
namespace WT {
	WebMercatorTilingSheme::WebMercatorTilingSheme(Rectangle& rect, int numberOfZeroTilesX /*= 1*/, int numberOfZeroTilesY /*= 1*/
		, Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/);
	{
		this->numberOfLevelZeroTilesX = numberOfLevelZeroTilesX;
		this->numberOfLevelZeroTilesY = numberOfLevelZeroTilesY;
	}

	int WebMercatorTilingSheme::getNumberOfXTilesAtLevel(int level) {
		return numberOfLevelZeroTilesX << level;
	}

	int WebMercatorTilingSheme::getNumberOfYTilesAtLevel(int level) {
		return numberOfLevelZeroTilesY << level;
	}

	//���Ի���Ϊ��λ�ľ��η�Χת��Ϊ��Ϊ��λ
	void WebMercatorTilingSheme::rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) {

	}
};