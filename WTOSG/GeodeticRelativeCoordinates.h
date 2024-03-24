#pragma once
#include <GeographicLib/Geocentric.hpp>
#include <GeographicLib/LocalCartesian.hpp>

class GeodeticRelativeCoordinates
{
public:
	/*****************************************************************************
	* @brief : 用于计算球面上两个点的相对位置的类，解决两个问题：
	*			1.fromRelativeXYZToWGS84 给定相对xyz位置后，计算相对参考点（A_wgs84）的xyz位置处点的经纬度
	*			2.fromWGS84ToRelativeXYZ 给定经纬度坐标B_wgs84，计算该点的相对于A_wgs84点的相对坐标xyz
	* @author : lmw
	* @date : 2020/9/1 9:20
	*****************************************************************************/
	GeodeticRelativeCoordinates(double lat, double lon, double h=0);

	/*****************************************************************************
	* @brief : 根据给定的相对于参考点A_wgs84的XYZ坐标计算该点的经纬度坐标
	* @author : lmw
	* @date : 2020/9/1 9:30
	*****************************************************************************/
	void fromRelativeXYZToWGS84(double x, double y, double z, double &lat, double &lon, double &h);

	/*****************************************************************************
	* @brief : 给定经纬度点B_wgs84，计算该点相对于参考点的XYZ位置
	* @author : lmw
	* @date : 2020/9/1 9:33
	*****************************************************************************/
	void fromWGS84ToRelativeXYZ(double lat, double lon, double h, double &x, double &y, double &z);


private:
	GeographicLib::LocalCartesian proj;
};