
#include "GeodeticRelativeCoordinates.h"


GeodeticRelativeCoordinates::GeodeticRelativeCoordinates(double lat, double lon, double h /*=0*/)
{
	GeographicLib::Geocentric earth(GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f());
	proj = GeographicLib::LocalCartesian(lat, lon, h, earth);
}

void GeodeticRelativeCoordinates::fromRelativeXYZToWGS84(double x, double y, double z, double &lat, double &lon, double &h)
{
	proj.Reverse(x, y, z, lat, lon, h);
}


void GeodeticRelativeCoordinates::fromWGS84ToRelativeXYZ(double lat, double lon, double h, double &x, double &y, double &z)
{
	proj.Forward(lat, lon, h, x, y, z);
}

