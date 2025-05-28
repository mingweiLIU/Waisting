#pragma once
#include <glm/glm.hpp>
#include "Rectangle.h"
#include "Ellipsoid.h"

namespace WT {

	class TilingScheme {
	protected:
		int numberOfLevelZeroTilesX;
		int numberOfLevelZeroTilesY;
		Ellipsoid ellipsoid;
		Rectangle rect;
	public:
		//��ȡlevelʱX�������Ƭ��
		virtual int getNumberOfXTilesAtLevel(int level) {
			return numberOfLevelZeroTilesX << level;
		}

		//��ȡlevelʱY�������Ƭ��
		virtual int getNumberOfYTilesAtLevel(int level) {
			return numberOfLevelZeroTilesY << level;
		}

		/**
		* @brief ��ȡ��Ƭ���������ڵ������塣
		* @returns �� Ellipsoid ����ĳ������á�
		*/
		const Ellipsoid& getEllipsoid() const {
			return ellipsoid;
		}

		/**
		 * @brief ��ȡ��Ƭ�������ǵĵ���Χ��
		 * @returns �� Rectangle ����ĳ������á�
		 */
		const Rectangle& getRectangle() const {
			return rect;
		}

		//���Ի���Ϊ��λ�ľ��η�Χת��Ϊ��Ϊ��λ
		virtual void rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) = 0;

		//����Ƭת��Ϊ��Ϊ��λ�ķ�Χ
		virtual void tileXYToNativeRectangle(int tileX, int tileY, int level, Rectangle& nativeRect) = 0;

		//����Ƭת��Ϊ����Ϊ��λ�ķ�Χ
		virtual void tileXYToRectangle(int tileX, int tileY, int level, Rectangle& rect) = 0;

		//�������λ�����ĸ���Ƭ��
		virtual void positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) = 0;
	};
};