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
		 * @brief ���Ե����ȱ�ʾ�ľ��Σ�ת��Ϊ����Ƭ������ͶӰ����ϵ�µľ��Ρ�
		 * ����ġ�ԭ������ϵ��ָ���� Web ī����ͶӰ�����Ƶѿ�������ϵ��
		 *
		 * @param rectangle ��ת���ĵ�����Σ���γ���Ի��ȱ�ʾ����
		 * @returns �����´�����Rectangle����
		 */
		Rectangle rectangleToNativeRectangle(Rectangle rect) override;

		//����Ƭת��Ϊ��Ϊ��λ�ķ�Χ
		Rectangle tileXYToNativeRectangle(int tileX, int tileY, int level) override;

		//����Ƭת��Ϊ����Ϊ��λ�ķ�Χ
		Rectangle tileXYToRectangle(int tileX, int tileY, int level) override;

		//�������λ�����ĸ���Ƭ��
		 /**
		 * @brief ���������������λ�õ���Ƭ�� (x, y) ���ꡣ
		 *
		 * @param radX λ�õľ��ȣ����ȣ���
		 * @param radY λ�õ�γ�ȣ����ȣ���
		 * @param level ��Ƭ��ϸ�ڼ���0 �����ϸ�ļ���
		 * @param tileX ������������ڴ洢������Ĵ��� X ���ꡣ
		 * @param tileY ������������ڴ洢������Ĵ��� Y ���ꡣ
		 */
		 bool positionToTileXY(double radX, double radY, int level, int& tileX, int& tileY) override;

	private:
		glm::dvec3 southwest_projected ;
		glm::dvec3 northeast_projected ;
	};
}