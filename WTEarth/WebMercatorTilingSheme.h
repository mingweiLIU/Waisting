#pragma once
#include "TilingSchema.h"
#include "Rectangle.h"
#include "Ellipsoid.h"
#include "WebMercatorProjection.h"
#include <memory>
namespace WT {
	class WebMercatorTilingSheme final :public TilingScheme {
	public:
		WebMercatorTilingSheme(int numberOfZeroTilesX = 1, int numberOfZeroTilesY = 1
			,Ellipsoid& ellipsoid= Ellipsoid::WGS84,Rectangle& rect=Rectangle::MAX_VALUE);

		/**
		 * @brief ���Ե����ȱ�ʾ�ľ��Σ�ת��Ϊ����Ƭ������ԭ������ϵ�µľ��Ρ�
		 * ����ġ�ԭ������ϵ��ָ���� Web ī����ͶӰ�����Ƶѿ�������ϵ��
		 *
		 * @param rectangle ��ת���ĵ�����Σ���γ���Ի��ȱ�ʾ����
		 * @returns ת�����ԭ������ϵ�µľ��Ρ�����ṩ�� `result`���򷵻ض�������ã����򷵻��´����Ķ���
		 */
		void rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) override;

		//����Ƭת��Ϊ��Ϊ��λ�ķ�Χ
		virtual void tileXYToNativeRectangle(int tileX, int tileY, int level, Rectangle& nativeRect) = 0;

		//����Ƭת��Ϊ����Ϊ��λ�ķ�Χ
		virtual void tileXYToRectangle(int tileX, int tileY, int level, Rectangle& rect) = 0;

		//�������λ�����ĸ���Ƭ��
		virtual void positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) = 0;
	private:
		Ellipsoid mEllipsoid;
		Rectangle mRect;
		std::shared_ptr<WebMercatorProjection> mProjection;
	};
}