#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "TilingSchema.h"
#include "Rectangle.h"

namespace WT{
	class GeographicTilingScheme final :public TilingScheme {
	private:
		Rectangle mRectangle;
	public:
		GeographicTilingScheme(Rectangle rectangle=Rectangle::MAX_VALUE,int numberOfLevelZeroTilesX=2, int numberOfLevelZeroTilesY = 1)
			:mRectangle(rectangle){
			this->numberOfLevelZeroTilesX = numberOfLevelZeroTilesX;
			this->numberOfLevelZeroTilesY = numberOfLevelZeroTilesY;
		}

		//���Ի���Ϊ��λ�ľ��η�Χת��Ϊ��Ϊ��λ
		void rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) override;

		//����Ƭת��Ϊ��Ϊ��λ�ķ�Χ
		void tileXYToNativeRectangle(int tileX, int tileY, int level, Rectangle& nativeRect) override;

		//����Ƭת��Ϊ����Ϊ��λ�ķ�Χ
		void tileXYToRectangle(int tileX, int tileY, int level, Rectangle& rect) override;

		//�������λ�����ĸ���Ƭ��
		void positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) override;
	};
}