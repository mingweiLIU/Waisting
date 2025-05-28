#pragma once
#include "TilingSchema.h"
#include "Rectangle.h"
#include "Ellipsoid.h"
namespace WT {
	class WebMercatorTilingSheme final :public TilingScheme {
	public:
		WebMercatorTilingSheme(int numberOfZeroTilesX = 1, int numberOfZeroTilesY = 1,Ellipsoid ellipsoid,);

		int getNumberOfXTilesAtLevel(int level) override;

		//��ȡlevelʱY�������Ƭ��
		int getNumberOfYTilesAtLevel(int level)override;

		//���Ի���Ϊ��λ�ľ��η�Χת��Ϊ��Ϊ��λ
		void rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) override;

		//����Ƭת��Ϊ��Ϊ��λ�ķ�Χ
		virtual void tileXYToNativeRectangle(int tileX, int tileY, int level, Rectangle& nativeRect) = 0;

		//����Ƭת��Ϊ����Ϊ��λ�ķ�Χ
		virtual void tileXYToRectangle(int tileX, int tileY, int level, Rectangle& rect) = 0;

		//�������λ�����ĸ���Ƭ��
		virtual void positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) = 0;
	private:
		Ellipsoid ellipsoid;
	};
}