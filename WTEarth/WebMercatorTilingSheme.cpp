#include "WebMercatorTilingSheme.h"
namespace WT {
	WebMercatorTilingSheme::WebMercatorTilingSheme(int numberOfZeroTilesX /*= 1*/, int numberOfZeroTilesY /*= 1*/
		, Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/, Rectangle& rect /*= Rectangle::MAX_VALUE*/)
	{
		this->numberOfLevelZeroTilesX = numberOfZeroTilesX;
        this->numberOfLevelZeroTilesY = numberOfZeroTilesY;
		mProjection = std::make_shared<WebMercatorProjection>(ellipsoid);
        southwest_projected = mProjection->project(rect.getSouthwest());
        // ����ͶӰ���εĶ�����
        northeast_projected = mProjection->project(rect.getNortheast());
        mEllipsoid = ellipsoid;
        mRect = rect;
	}

	//���Ի���Ϊ��λ�ľ��η�Χת��Ϊ��Ϊ��λ
	Rectangle WebMercatorTilingSheme::rectangleToNativeRectangle(Rectangle rect) {
		// ʹ���ڲ��� WebMercatorProjection ʵ����ִ��ͶӰ��
		// ����ͶӰ���ε����Ͻǡ�height ��Ϊ 0����Ϊ����ת��ֻ���� X/Y��
		const glm::dvec3 southwest_projected = mProjection->project(rect.getSouthwest());
		// ����ͶӰ���εĶ����ǡ�
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
		double minLatitude = atan(0.5 * (exp(n) - exp(-n)));   // ��˫�����м���

		double maxLongitude = (tileX+1) / (double)(numberOfLevelZeroTilesX << level) * glm::two_pi<double>() - glm::pi<double>();

		n = glm::pi<double>() - glm::two_pi<double>() * tileY / (double)(numberOfLevelZeroTilesY << level);
		double maxLatitude = atan(0.5 * (exp(n) - exp(-n)));   // ��˫�����м���

		return Rectangle(minLongitude, minLatitude, maxLongitude, maxLatitude);
	}


	bool WebMercatorTilingSheme::positionToTileXY(double radX, double radY, int level, int& tileX, int& tileY) {
		tileX=(int)(floor((radX + glm::pi<double>()) / glm::two_pi<double>() * (numberOfLevelZeroTilesX << level)));
		tileY=(int)(floor((1.0 - asinh(tan(radY)) / glm::pi<double>()) / 2.0 * (numberOfLevelZeroTilesY << level)));
        return true; // �ɹ����㲢����
	}
};