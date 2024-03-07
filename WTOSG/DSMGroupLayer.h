#ifndef DSMGROUP_H
#define DSMGROUP_H
#include "WTDefines.h"
#include "ProgressInfo.h"
#include "WTLayer.h"
#include <optional>
#include <osg/MatrixTransform>
#include <osg/Matrix>

WTNAMESPACESTART
//DSM对象
class DSMGroupLayer : public osg::MatrixTransform
{
public:
	DSMGroupLayer(std::string folderPath, std::string outFolderPath, std::string xmlFileName = "metadata.xml", std::string dataFolder = "Data");
	osg::Vec3d getCenterWGS84();
	osg::Vec3d getCenterOriginSRS();
	osg::Vec3d getWGS84OfVertex(osg::Vec3d oneVertex);
	osg::Matrix getMatrix();
	//修改可见性
	void switchVisibility(std::optional<bool> visibility);
	//缩放到图层
	void zoomToLayer(osgGA::CameraManipulator* cameraManipulator);
	//获取可见性
	bool getVisibility();
private:
	bool readDSMMetaDataXML();
	bool getAllTilesInFolder();
	void transAllOSGBInFolder(
		std::string osgbFilesFolder, std::string osgbOutFilesFolder,
		osg::Vec3d transVec = osg::Vec3d(0, 0, 0), osg::Vec3d roateVec = osg::Vec3d(0, 0, 0), osg::Vec3d scaleVec = osg::Vec3d(1, 1, 1)
		, VertexCalcFunction beforeCall = NULL, VertexCalcFunction afterCall = NULL
	);
private:
	osg::ref_ptr<WTLayer> dsmLayer;
	std::string folderPath, outFolderPath, xmlFileName, dataFolder;
	osgEarth::GeoPoint wgs84Center;
	osgEarth::GeoPoint originCenter;
	const osgEarth::SpatialReference* wgs84SRS = osgEarth::SpatialReference::get("epsg:4326");
	const osgEarth::SpatialReference* originSRS;
	VertexCalcFunction beforeCall, afterCall;

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
