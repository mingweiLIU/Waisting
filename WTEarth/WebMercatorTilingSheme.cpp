#include "WebMercatorTilingSheme.h"
namespace WT {
	WebMercatorTilingSheme::WebMercatorTilingSheme(int numberOfZeroTilesX /*= 1*/, int numberOfZeroTilesY /*= 1*/
		, Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/, Rectangle& rect /*= Rectangle::MAX_VALUE*/)
	{
		this->numberOfLevelZeroTilesX = numberOfLevelZeroTilesX;
		this->numberOfLevelZeroTilesY = numberOfLevelZeroTilesY;
		mProjection = std::make_shared<WebMercatorProjection>(ellipsoid);
	}

	//���Ի���Ϊ��λ�ľ��η�Χת��Ϊ��Ϊ��λ
	void WebMercatorTilingSheme::rectangleToNativeRectangle(Rectangle rect, Rectangle& nativeRect) {
		// ʹ���ڲ��� WebMercatorProjection ʵ����ִ��ͶӰ��
		// ����ͶӰ���ε����Ͻǡ�height ��Ϊ 0����Ϊ����ת��ֻ���� X/Y��
		const glm::dvec3 southwest_projected = mProjection->project(Rectangle::southwest(rectangle));
		// ����ͶӰ���εĶ����ǡ�
		const glm::dvec3 northeast_projected = _projection.project(Rectangle::northeast(rectangle));

		// ���δ�ṩ��������򴴽��µ� Rectangle ʵ�������ء�
		if (result == nullptr) {
			return Rectangle(southwest_projected.x,
				southwest_projected.y,
				northeast_projected.x,
				northeast_projected.y);
		}

		// ���򣬽������䵽�ṩ��ʵ���С�
		result->west = southwest_projected.x;
		result->south = southwest_projected.y;
		result->east = northeast_projected.x;
		result->north = northeast_projected.y;
		return *result; // ���ض��޸ĺ�ʵ��������
	}
};