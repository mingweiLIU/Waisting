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
		//获取level时X方向的瓦片数
		virtual int getNumberOfXTilesAtLevel(int level) {
			return numberOfLevelZeroTilesX << level;
		}

		//获取level时Y方向的瓦片数
		virtual int getNumberOfYTilesAtLevel(int level) {
			return numberOfLevelZeroTilesY << level;
		}

		/**
		* @brief 获取瓦片方案所基于的椭球体。
		* @returns 对 Ellipsoid 对象的常量引用。
		*/
		const Ellipsoid& getEllipsoid() const {
			return ellipsoid;
		}

		/**
		 * @brief 获取瓦片方案覆盖的地理范围。
		 * @returns 对 Rectangle 对象的常量引用。
		 */
		const Rectangle& getRectangle() const {
			return rect;
		}

		//将以弧度为单位的矩形范围转换为度为单位
		virtual void rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) = 0;

		//将瓦片转换为度为单位的范围
		virtual void tileXYToNativeRectangle(int tileX, int tileY, int level, Rectangle& nativeRect) = 0;

		//将瓦片转换为弧度为单位的范围
		virtual void tileXYToRectangle(int tileX, int tileY, int level, Rectangle& rect) = 0;

		//计算地理位置在哪个瓦片上
		virtual void positionToTileXY(float radX, float radY, int level, int& tileX, int& tileY) = 0;
	};
};