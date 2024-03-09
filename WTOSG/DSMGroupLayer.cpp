#include "DSMGroupLayer.h"
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#include <osgEarth/XmlUtils>
#include <osgEarth/Registry>
#include "NodeHandleVisitor.h"
#include "GeodeticRelativeCoordinates.h"

WTNAMESPACESTART
DSMGroupLayer::DSMGroupLayer(std::string folderPath, std::string outFolderPath, std::shared_ptr<ProgressInfo> progreeInfo, std::string xmlFileName /*= "metadata.xml"*/, std::string dataFolder /*= "Data"*/)
	:folderPath(folderPath)
	, outFolderPath(outFolderPath)
	, xmlFileName(xmlFileName)
	, dataFolder(dataFolder)
	, dsmLayer(new WTLayer())
	,progreeInfo(progreeInfo)
{
	if (readDSMMetaDataXML()) {
		getAllTilesInFolder();

		//osg::ComputeBoundsVisitor boundVisitor;
		//dsmGroupSwitch->accept(boundVisitor);
		//osg::BoundingBox boundingBox = boundVisitor.getBoundingBox();
		//float zMin = boundingBox.zMin();

		////originCenter.z() = -zMin;

		osg::Matrix posMatrix;
		wgs84Center.createLocalToWorld(posMatrix);
		this->setMatrix(posMatrix);
	}
	progreeInfo->showProgress(1, 1, "", "数据加载完成...");
}

bool DSMGroupLayer::readDSMMetaDataXML()
{
	std::string xmlfilePath = osgDB::concatPaths(folderPath, xmlFileName);
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

osg::Vec3d DSMGroupLayer::getCenterWGS84()
{
	return this->wgs84Center.vec3d();
}

osg::Vec3d DSMGroupLayer::getCenterOriginSRS()
{
	return this->originCenter.vec3d();
}

osg::Vec3d DSMGroupLayer::getWGS84OfVertex(osg::Vec3d oneVertex)
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

void DSMGroupLayer::transAllOSGBInFolder(std::string osgbFilesFolder, std::string osgbOutFilesFolder, osg::Vec3d transVec /*= osg::Vec3d(0, 0, 0)*/, osg::Vec3d roateVec /*= osg::Vec3d(0, 0, 0)*/, osg::Vec3d scaleVec /*= osg::Vec3d(1, 1, 1) */, VertexCalcFunction beforeCall /*= NULL*/, VertexCalcFunction afterCall /*= NULL */)
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

bool DSMGroupLayer::getAllTilesInFolder()
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
				dsmLayer->addChild(oneTile);
			}
		}
	}
	osgEarth::Registry::shaderGenerator().run(dsmLayer);
	return true;
}

osg::Matrix DSMGroupLayer::getMatrix()
{
	//变换位置
	//这里要把原始的参考点转为wgs84的
	osg::Matrix posMatrix;
	wgs84Center.createLocalToWorld(posMatrix);
	return posMatrix;
}

void DSMGroupLayer::switchVisibility(std::optional<bool> visibility)
{
	dsmLayer->switchVisibility(visibility);
}

void DSMGroupLayer::zoomToLayer(osgGA::CameraManipulator* cameraManipulator)
{
	dsmLayer->zoomToLayer(cameraManipulator);
}

bool DSMGroupLayer::getVisibility()
{
	return dsmLayer->getVisibility();
}

osgDB::ReaderWriter::ReadResult DSMGroupLayer::ReadFileProgressCallback::readNode(const std::string& filename, const osgDB::Options* options)
{
	handledNubmer++;
	osgDB::ReaderWriter::ReadResult result = osgDB::Registry::instance()->readNodeImplementation(filename, options);
	progreeInfo->showProgress(totalNuber, handledNubmer, filename, "模型读取");
	return result;
}

WTNAMESPACEEND