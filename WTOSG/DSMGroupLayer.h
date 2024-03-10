#ifndef DSMGROUP_H
#define DSMGROUP_H
#include "WTDefines.h"
#include <optional>
#include <osg/Matrix>
#include <osgDB/Registry>
#include <osgEarth/SpatialReference>
#include <osgEarth/GeoData>

#include "ProgressInfo.h"
#include "WTLayer.h"

WTNAMESPACESTART
//DSM对象
class DSMGroup : public osg::Group
{
public:
	DSMGroup(std::string folderPath, std::shared_ptr<ProgressInfo> progreeInfo=std::make_shared<ProgressInfo>(), std::string xmlFileName = "metadata.xml", std::string dataFolder = "Data");
	osg::Vec3d getCenterWGS84();
	osg::Vec3d getCenterOriginSRS();
	osg::Vec3d getWGS84OfVertex(osg::Vec3d oneVertex);
	osg::Matrix getMatrix();
	//缩放到图层
	void zoomToLayer(osgGA::CameraManipulator* cameraManipulator);
private:
	bool readDSMMetaDataXML();
	bool getAllTilesInFolder();
	void transAllOSGBInFolder(
		std::string osgbFilesFolder, std::string osgbOutFilesFolder,
		osg::Vec3d transVec = osg::Vec3d(0, 0, 0), osg::Vec3d roateVec = osg::Vec3d(0, 0, 0), osg::Vec3d scaleVec = osg::Vec3d(1, 1, 1)
		, VertexCalcFunction beforeCall = NULL, VertexCalcFunction afterCall = NULL
	);
private:
	std::string folderPath, xmlFileName, dataFolder;
	osgEarth::GeoPoint wgs84Center;
	osgEarth::GeoPoint originCenter;
	const osgEarth::SpatialReference* wgs84SRS = osgEarth::SpatialReference::get("epsg:4326");
	const osgEarth::SpatialReference* originSRS;
	VertexCalcFunction beforeCall, afterCall;
	std::shared_ptr<ProgressInfo> progreeInfo;

public:

	class ReadFileProgressCallback :public osgDB::Registry::ReadFileCallback
	{
	public:
		ReadFileProgressCallback(std::shared_ptr<ProgressInfo> progreeInfo, int totalNuber)
			:progreeInfo(progreeInfo)
			, totalNuber(totalNuber) {}

		virtual osgDB::ReaderWriter::ReadResult readNode(const std::string& filename, const osgDB::Options* options);
	private:
		std::shared_ptr<ProgressInfo> progreeInfo;
		int totalNuber;
		int handledNubmer = 0;
	};
};
WTNAMESPACEEND
#endif // DSMGROUP_H
