#include "WebMercatorTilingSheme.h"
namespace WT {
	WebMercatorTilingSheme::WebMercatorTilingSheme(int numberOfZeroTilesX /*= 1*/, int numberOfZeroTilesY /*= 1*/
		, Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/, Rectangle& rect /*= Rectangle::MAX_VALUE*/)
	{
		this->numberOfLevelZeroTilesX = numberOfLevelZeroTilesX;
		this->numberOfLevelZeroTilesY = numberOfLevelZeroTilesY;
		mProjection = std::make_shared<WebMercatorProjection>(ellipsoid);
	}

	//将以弧度为单位的矩形范围转换为度为单位
	void WebMercatorTilingSheme::rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) {
		// 使用内部的 WebMercatorProjection 实例来执行投影。
		// 首先投影矩形的西南角。height 设为 0，因为矩形转换只关心 X/Y。
		const glm::dvec3 southwest_projected = mProjection->project(Rectangle::southwest(rectangle));
		// 接着投影矩形的东北角。
		const glm::dvec3 northeast_projected = _projection.project(Rectangle::northeast(rectangle));

		// 如果未提供结果对象，则创建新的 Rectangle 实例并返回。
		if (result == nullptr) {
			return Rectangle(southwest_projected.x,
				southwest_projected.y,
				northeast_projected.x,
				northeast_projected.y);
		}

		// 否则，将结果填充到提供的实例中。
		result->west = southwest_projected.x;
		result->south = southwest_projected.y;
		result->east = northeast_projected.x;
		result->north = northeast_projected.y;
		return *result; // 返回对修改后实例的引用
	}
};