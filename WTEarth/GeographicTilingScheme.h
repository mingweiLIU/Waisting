#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "TilingSchema.h"
#include "Rectangle.h"
#include "Ellipsoid.h"
#include "GeographicProjection.h"
#include "Cartographic.h"

namespace WT{
	class GeographicTilingScheme final :public TilingScheme {
	private:
	public:
		GeographicTilingScheme(Ellipsoid& ellipsoid = Ellipsoid::WGS84, Rectangle rectangle = Rectangle::MAX_VALUE
			, int numberOfLevelZeroTilesX = 2, int numberOfLevelZeroTilesY = 1);

		//���Ի���Ϊ��λ�ľ��η�Χת��Ϊ��Ϊ��λ
		Rectangle rectangleToNativeRectangle(Rectangle rect) override;

		//����Ƭת��Ϊ��Ϊ��λ�ķ�Χ
		Rectangle tileXYToNativeRectangle(int tileX, int tileY, int level) override;

		//����Ƭת��Ϊ����Ϊ��λ�ķ�Χ
		Rectangle tileXYToRectangle(int tileX, int tileY, int level) override;

		//�������λ�����ĸ���Ƭ��
		bool positionToTileXY(double radX, double radY, int level, int& tileX, int& tileY) override;
	};
}