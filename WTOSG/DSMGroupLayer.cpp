#include "DSMGroupLayer.h"
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#include <osgEarth/XmlUtils>
#include <osgEarth/Viewpoint>
#include <osgEarth/Registry>
#include <osgEarth/EarthManipulator>
#include "NodeHandleVisitor.h"
#include "GeodeticRelativeCoordinates.h"

WTNAMESPACESTART
DSMGroup::DSMGroup(std::string folderPath, std::shared_ptr<ProgressInfo> progreeInfo, std::string xmlFileName /*= "metadata.xml"*/, std::string dataFolder /*= "Data"*/)
	:folderPath(folderPath)
	, xmlFileName(xmlFileName)
	, dataFolder(dataFolder)
	,progreeInfo(progreeInfo)
{
	bool tempOK = false;
	if (tempOK = readDSMMetaDataXML();tempOK) {
		isOK=tempOK&&getAllTilesInFolder();
		progreeInfo->showProgress(1, 1, "", "数据加载完成...");
	}
}

bool DSMGroup::readDSMMetaDataXML()
{
	std::string xmlfilePath = osgDB::concatPaths(folderPath, xmlFileName);
	setlocale(LC_ALL, "");
	osg::ref_ptr<osgEarth::XmlDocument> metaDataXML = osgEarth::XmlDocument::load(xmlfilePath);
	if (!metaDataXML.valid()) {
		return false;
	}

	osgEarth::Config conf = metaDataXML->getConfig();
	std::string srsStr = conf.child("modelmetadata").child("srs").value();
	std::string srsOriginStr = conf.child("modelmetadata").child("srsorigin").value();

	//解析SRSOrigin
	int fristColPos = srsOriginStr.find(',');
	int lastColPos = srsOriginStr.find_last_of(',');
	std::string xString = srsOriginStr.substr(0, fristColPos);
	std::string yString = srsOriginStr.substr(fristColPos + 1, lastColPos - fristColPos - 1);
	std::string zString = srsOriginStr.substr(lastColPos + 1);

	double xOrigin = std::stod(xString);
	double yOrigin = std::stod(yString);
	double zOrigin = std::stod(zString);

	//处理SRS问题，分三种，第一ENU，第二EPSG 第三wkt
	unsigned int indexFlag = srsStr.find("ENU");
	if (0 == indexFlag)
	{
		//数据坐标系为站心坐标系
		srsStr = srsStr.substr(4);
		indexFlag = srsStr.find(",");
		std::string yString = srsStr.substr(0, indexFlag);
		std::string xString = srsStr.substr(indexFlag + 1);

		double y = std::stod(yString);
		double x = std::stod(xString);

		//这里ENU的默认SRSOrigin中只有z可用 且相对坐标
		originCenter = osgEarth::GeoPoint(osgEarth::SpatialReference::get("epsg:4326"), x, y, zOrigin, osgEarth::ALTMODE_RELATIVE);
		originSRS = osgEarth::SpatialReference::get("epsg:4326");
	}
	else
	{
		//如何判断wkt字符串？
		std::transform(srsStr.begin(), srsStr.end(), srsStr.begin(), ::tolower);
		originSRS = osgEarth::SpatialReference::get(srsStr);
		originCenter = osgEarth::GeoPoint(originSRS, xOrigin, yOrigin, zOrigin, osgEarth::ALTMODE_RELATIVE);
	}

	wgs84Center = originCenter.transform(wgs84SRS);
	return true;
}

osgEarth::GeoPoint DSMGroup::getCenterWGS84()
{
	return this->wgs84Center;
}

osg::Vec3d DSMGroup::getCenterOriginSRS()
{
	return this->originCenter.vec3d();
}

osg::Vec3d DSMGroup::getWGS84OfVertex(osg::Vec3d oneVertex)
{
	if (this->originSRS->isGeocentric())
	{
		//地心坐标系
		return originCenter.vec3d() + oneVertex;
	}
	else if (this->originSRS->isProjected())
	{
		//投影坐标
		osg::Vec3d oneVertexProjectCoord = (originCenter.vec3d() + oneVertex);
		osgEarth::GeoPoint oneVertexProjectCoordPoint = osgEarth::GeoPoint(originSRS, oneVertexProjectCoord, osgEarth::ALTMODE_RELATIVE);
		osgEarth::GeoPoint wgs84Vertex = oneVertexProjectCoordPoint.transform(wgs84SRS);
		return wgs84Vertex.vec3d();
	}
	else if (this->originSRS->isGeographic())
	{
		//经纬度坐标
		double lon, lat, h;
		GeodeticRelativeCoordinates geoRelativeCooder(originCenter.x(), originCenter.y(), originCenter.z());
		geoRelativeCooder.fromRelativeXYZToWGS84(oneVertex.x(), oneVertex.y(), oneVertex.z(), lat, lon, h);
		return osg::Vec3d(lon, lat, h);
	}
}

void DSMGroup::transAllOSGBInFolder(std::string osgbFilesFolder, std::string osgbOutFilesFolder, osg::Vec3d transVec /*= osg::Vec3d(0, 0, 0)*/, osg::Vec3d roateVec /*= osg::Vec3d(0, 0, 0)*/, osg::Vec3d scaleVec /*= osg::Vec3d(1, 1, 1) */, VertexCalcFunction beforeCall /*= NULL*/, VertexCalcFunction afterCall /*= NULL */)
{
	std::vector<std::string> osgbContents = osgDB::getDirectoryContents(osgbFilesFolder);
	for (std::string& oneFileName : osgbContents)
	{
		if (oneFileName == "." || oneFileName == "..") continue;

		std::string filePath = osgDB::concatPaths(osgbFilesFolder, oneFileName);
		if (osgDB::fileType(filePath) != osgDB::REGULAR_FILE || osgDB::getLowerCaseFileExtension(filePath) != "osgb") continue;

		osg::ref_ptr<osgDB::Options> options = new osgDB::Options;
		options->setOptionString("OutputTextureFiles");
		osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filePath, options);
		if (node)
		{
			NodeVertexHandler nvh(transVec, roateVec, scaleVec);
			nvh.setVertexComputeFunction(beforeCall, afterCall);
			node->accept(nvh);

			osg::ref_ptr<osgDB::ReaderWriter::Options> options = new osgDB::ReaderWriter::Options;
			options->setOptionString("WriteImageHint=IncludeFile");			// ����ѹ��

			osgDB::writeNodeFile(*node, osgDB::concatPaths(osgbOutFilesFolder, oneFileName), options);
		}
	}
}

bool DSMGroup::getAllTilesInFolder()
{
	std::string dataFolderPath = osgDB::concatPaths(folderPath, dataFolder);
	osgDB::DirectoryContents allFileNameInFolder = osgDB::getDirectoryContents(dataFolderPath);

	osg::ref_ptr<osgDB::Options> vertexCorrectWhileReadOption = osgEarth::Registry::instance()->cloneOrCreateOptions();
	vertexCorrectWhileReadOption->setReadFileCallback(new ReadFileProgressCallback(progreeInfo, allFileNameInFolder.size() - 2));

	for (osgDB::DirectoryContents::iterator fileName = allFileNameInFolder.begin(); fileName != allFileNameInFolder.end(); ++fileName)
	{
		if (fileName->compare(".") == 0 || fileName->compare("..") == 0)
			continue;
		std::string filePath = osgDB::concatPaths(dataFolderPath, *fileName);
		if (osgDB::fileType(filePath) == osgDB::DIRECTORY) {

			osg::ref_ptr<osg::Node> oneTile = osgDB::readNodeFile(osgDB::concatPaths(filePath, *fileName + ".osgb"), vertexCorrectWhileReadOption);
			if (oneTile)
			{
				oneTile->setName(osgDB::concatPaths(filePath, *fileName + ".osgb"));
				this->addChild(oneTile);
			}
		}
	}
	return true;
}

osg::Matrix DSMGroup::getMatrix()
{
	//变换位置
	//这里要把原始的参考点转为wgs84的
	osg::Matrix posMatrix;
	wgs84Center.createLocalToWorld(posMatrix);
	return posMatrix;
}

void DSMGroup::zoomToLayer(osgGA::CameraManipulator* cameraManipulator)
{
	osgEarth::EarthManipulator* em = dynamic_cast<osgEarth::EarthManipulator*>(cameraManipulator);
	if (em) {
		GeodeticRelativeCoordinates geodeticProj(wgs84Center.y(), wgs84Center.x());
		osg::Vec3 boundingCenter = this->getBound().center();
		double lat1 = 0.0, lon1 = 0.0, h1 = 0.0;
		geodeticProj.fromRelativeXYZToWGS84(boundingCenter.x(), boundingCenter.y(), boundingCenter.z(), lat1, lon1, h1);

		osgEarth::Viewpoint vp;
		vp.focalPoint() = osgEarth::GeoPoint(osgEarth::SpatialReference::get("epsg:4326"), lon1, lat1, 0);
		vp.heading()->set(0.0, osgEarth::Units::DEGREES);
		vp.pitch()->set(-89.0, osgEarth::Units::DEGREES);
		vp.range()->set(this->getBound().radius() * 2.0, osgEarth::Units::METERS);
		vp.positionOffset()->set(0, 0, 0);

		em->setViewpoint(vp, 2);
	}
	else {
		double radius = this->getBound().radius();
		double viewDistance = radius * 2.5;
		osg::Vec3d viewDirection(0.0, -1.0, 1.5);
		osg::Vec3d center = this->getBound().center();
		osg::Vec3d eye = center + viewDirection * viewDistance;
		osg::Matrixd m = osg::Matrixd::lookAt(eye, center, osg::Vec3d(0, 0, 1));
		cameraManipulator->setByMatrix(osg::Matrixd::inverse(m));
	}
}

osgDB::ReaderWriter::ReadResult DSMGroup::ReadFileProgressCallback::readNode(const std::string& filename, const osgDB::Options* options)
{
	handledNubmer++;
	osgDB::ReaderWriter::ReadResult result = osgDB::Registry::instance()->readNodeImplementation(filename, options);
	progreeInfo->showProgress(totalNuber, handledNubmer, filename, "模型读取");
	return result;
}

WTNAMESPACEEND