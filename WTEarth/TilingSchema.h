#pragma once
#include<memory>
#include <glm/glm.hpp>
#include "Rectangle.h"
#include "Ellipsoid.h"
#include "Projection.h"

namespace WT {

	class TilingScheme {
	protected:
		int numberOfLevelZeroTilesX;
		int numberOfLevelZeroTilesY;
		Ellipsoid mEllipsoid;
		Rectangle mRect;
		std::shared_ptr<Projection> mProjection;
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
			return mEllipsoid;
		}

		/**
		 * @brief ��ȡ��Ƭ�������ǵĵ���Χ��
		 * @returns �� Rectangle ����ĳ������á�
		 */
		const Rectangle& getRectangle() const {
			return mRect;
		}

		//���Ի���Ϊ��λ�ľ��η�Χת��ΪmΪ��λ
		virtual Rectangle rectangleToNativeRectangle(Rectangle rect) = 0;

		//����Ƭת��ΪmΪ��λ�ķ�Χ
		virtual Rectangle tileXYToNativeRectangle(int tileX, int tileY, int level) = 0;

		//����Ƭת��Ϊ����Ϊ��λ�ķ�Χ
		virtual Rectangle tileXYToRectangle(int tileX, int tileY, int level) = 0;

		//�������λ�����ĸ���Ƭ��
		virtual bool positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) = 0;

		/////////////////////////����Ϊ��Ƭ�����õĺ���/////////////////////////////////////////
		// ����Rectanle��������Ƭ��Χ
		virtual Rectangle rectangle2TileXYRange(Rectangle& rectToCalc,int level) {
			Cartographic minXYPos = rectToCalc.getSouthwest();
			Cartographic maxXYPos = rectToCalc.getNortheast();

			//������תΪ����
			minXYPos.longitude = glm::radians(minXYPos.longitude);
			minXYPos.latitude = glm::radians(minXYPos.latitude);
			maxXYPos.longitude = glm::radians(maxXYPos.longitude);
			maxXYPos.latitude = glm::radians(maxXYPos.latitude);

			//
			int minTileX, minTileY, maxTileX, maxTileY;
			positionToTileXY(minXYPos.longitude, minXYPos.latitude, level, minTileX, minTileY);
			positionToTileXY(maxXYPos.longitude, maxXYPos.latitude, level, maxTileX, maxTileY);
			return Rectangle(minTileX, minTileY, maxTileX, maxTileY);
		}

		//������Ƭ���� תΪ�䷶Χ����λΪ�ȣ�
		virtual Rectangle tileXYToDegreeRectangle(int tileX, int tileY, int level) {
			Rectangle radRect = tileXYToRectangle(tileX, tileY, level);
			radRect.east = glm::degrees(radRect.east);
			radRect.west = glm::degrees(radRect.west);
			radRect.south = glm::degrees(radRect.south);
			radRect.north = glm::degrees(radRect.north);
			return radRect;
		}

		//������γ�ȣ���λ�ȣ�������Ƭ����
		virtual bool positionDegreeToTileXY(double degreeX, double degreeY, int level, int& tileX, int& tileY) {
			return positionToTileXY(glm::radians(degreeX), glm::radians(degreeY), level, tileX, tileY);
		}

		//�����㼶����������
		virtual double mapSize(int level, int tileSize) {
			return  std::ceil(tileSize * (double)(numberOfLevelZeroTilesX << level));
		}

		//�������γ�ȣ���λ�ȣ��µ���ֱ���
		virtual double groundResolution(double latitude, double level, int tileSize) {
			return cos(latitude * glm::pi<double>() / 180) * glm::two_pi<double>() * mEllipsoid.getMaximumRadius() / mapSize(level, tileSize);
		}

		//��������ֱ���Ӱ��ĺ�����Ƭ�㼶
		virtual int getPoperLevel(double groundResolution, int tileSize) {
			const double EARTH_CIRCUMFERENCE = 40075016.686;  // �������ܳ����ף�

			// OSM �� zoom=0 ʱ�ķֱ��ʣ���/���أ�
			const double RESOLUTION_0 = EARTH_CIRCUMFERENCE / (numberOfLevelZeroTilesX * tileSize);

			// ������� zoom������ֵ������С����
			double zoom = std::log2(RESOLUTION_0 / groundResolution);

			// ȡ��������ȡ��������ֱ��ʹ��ߣ�
			int bestZoom = static_cast<int>(std::floor(zoom));

			return bestZoom;
		}
	};
};