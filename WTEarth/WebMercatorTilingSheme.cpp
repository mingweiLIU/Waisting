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
        // ���� Cartographic ���������ڲ�����
        Cartographic position(static_cast<double>(radX), static_cast<double>(radY), 0.0); // ����߶�Ϊ 0.0

        // ���λ���Ƿ�����Ƭ�����ľ��α߽���
        if (!this->mRect.contains(position)) {
            // ���λ�ó����߽磬�򷵻� false�����޸� tileX �� tileY
            return false;
        }

        // ��ȡָ�������� X �� Y �������Ƭ����
        const int xTiles = getNumberOfXTilesAtLevel(level);
        const int yTiles = getNumberOfYTilesAtLevel(level);

        // ��������ͶӰ��Χ�Ŀ�Ⱥ͸߶ȣ�����Ϊ��λ��
        int xTileWidth = (northeast_projected.x - southwest_projected.x) / xTiles;
        int yTileHeight = (northeast_projected.y - southwest_projected.y) / yTiles;

        // ������λ��ͶӰ�� Web ī������������
        const glm::dvec3 webMercatorPosition = mProjection->project(position);

        // ����ͶӰ���뷽�����߽�ͱ��߽�ľ���
        // distanceFromWest: �����߽絽��� X ����
        const double distanceFromWest = webMercatorPosition.x - southwest_projected.x;
        // distanceFromNorth: �ӱ��߽絽��� Y ���� (ע�� Y �᷽��ͨ�� Web ī���е� Y �����ϣ�����Ƭϵͳ�ӱ�����)
        const double distanceFromNorth = northeast_projected.y - webMercatorPosition.y;

        // ������Ƭ�� X ����
        // ʹ�� static_cast<int> ��������ȡ��
        int xTileCoordinate = static_cast<int>(distanceFromWest / xTileWidth);
        // �߽��飺ȷ�� X ���겻�ᳬ�������Ƭ����
        if (xTileCoordinate >= xTiles) {
            xTileCoordinate = xTiles - 1;
        }
        else if (xTileCoordinate < 0) { // ȷ�������Ǹ�������Ȼͨ���� contains ���󲻻ᷢ��
            xTileCoordinate = 0;
        }

        // ������Ƭ�� Y ����
        int yTileCoordinate = static_cast<int>(distanceFromNorth / yTileHeight);
        // �߽��飺ȷ�� Y ���겻�ᳬ�������Ƭ����
        if (yTileCoordinate >= yTiles) {
            yTileCoordinate = yTiles - 1;
        }
        else if (yTileCoordinate < 0) { // ȷ�������Ǹ���
            yTileCoordinate = 0;
        }

        // ͨ�����ò������ؽ��
        tileX = xTileCoordinate;
        tileY = yTileCoordinate;
        return true; // �ɹ����㲢����
	}
};