#pragma once
#include <GeographicLib/Geocentric.hpp>
#include <GeographicLib/LocalCartesian.hpp>

class GeodeticRelativeCoordinates
{
public:
	/*****************************************************************************
	* @brief : ���ڼ�������������������λ�õ��࣬����������⣺
	*			1.fromRelativeXYZToWGS84 �������xyzλ�ú󣬼�����Բο��㣨A_wgs84����xyzλ�ô���ľ�γ��
	*			2.fromWGS84ToRelativeXYZ ������γ������B_wgs84������õ�������A_wgs84����������xyz
	* @author : lmw
	* @date : 2020/9/1 9:20
	*****************************************************************************/
	GeodeticRelativeCoordinates(double lat, double lon, double h=0);

	/*****************************************************************************
	* @brief : ���ݸ���������ڲο���A_wgs84��XYZ�������õ�ľ�γ������
	* @author : lmw
	* @date : 2020/9/1 9:30
	*****************************************************************************/
	void fromRelativeXYZToWGS84(double x, double y, double z, double &lat, double &lon, double &h);

	/*****************************************************************************
	* @brief : ������γ�ȵ�B_wgs84������õ�����ڲο����XYZλ��
	* @author : lmw
	* @date : 2020/9/1 9:33
	*****************************************************************************/
	void fromWGS84ToRelativeXYZ(double lat, double lon, double h, double &x, double &y, double &z);


private:
	GeographicLib::LocalCartesian proj;
};