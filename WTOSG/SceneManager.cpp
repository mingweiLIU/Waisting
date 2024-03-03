#include "SceneManager.h"
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osg/Switch>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/SphericalManipulator>

#include <osgEarth/Sky>
#include <osgEarth/XYZ>
#include <osgEarth/Profile>
#include <osgEarth/Map>
#include <osgEarth/XYZ>
#include <osgEarth/GLUtils>
#include <osgEarth/LogarithmicDepthBuffer>
#include <gdal_priv.h>
#include <cpl_conv.h>

#include "NanoID/nanoid.h"
#include "OperationTools.h"

class FindLayer : public osg::NodeVisitor
{
public:
	explicit FindLayer(const std::string& findInfo, FINDLAYERTYPE findType=FINDLAYERTYPE::UID)
		:osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
		, findInfo(findInfo)
		, findType(findType){}

	virtual void apply(osg::Group& node)override {
		WT::WTLayer* oneLayer = dynamic_cast<WT::WTLayer*>(&node);
		if (!oneLayer) {
			traverse(node);
			return;
		}

		switch (findType)
		{
		case FINDLAYERTYPE::UID:
			if (oneLayer->getUID() == findInfo)
				mLayer = oneLayer;
			else
				traverse(node);
			break;
		case FINDLAYERTYPE::NAME:
			if(oneLayer->getName() == findInfo)
				mLayer = oneLayer;
			else
				traverse(node);
			break;
		default:
			break;
		}
	}
	WT::WTLayer* getLayer() const { return mLayer; }
private:
	std::string findInfo;
	WT::WTLayer* mLayer = nullptr;
	FINDLAYERTYPE findType;
};

SceneManager::SceneManager()
{
	commonSettings();
	setupEarth();
	//initOSG();
}

SceneManager& SceneManager::getInstance()
{
	static SceneManager sceneManager;
	return sceneManager;
}

osg::ref_ptr<osgViewer::Viewer> SceneManager::getViewer()
{
	return viewer;
}

void SceneManager::addOperation(osg::Operation* operation)
{
	operationQueue->add(operation);
}

void SceneManager::addLayer(WT::WTLayer* layer, WT::WTLayer* parentLayer /*= nullptr*/)
{
	auto parentGroupNode = (parentLayer == nullptr) ? root : parentLayer;
	
	auto op = new AddChildOperation(layer, parentGroupNode);
	addOperation(op);

	std::string parentUID= (parentLayer == nullptr) ? "" : parentLayer->getUID();
	emit(nodeLoaded(layer->getName(), layer->getUID(), parentUID, layer->getValue(0)));
}

osg::ref_ptr<osg::Node> SceneManager::addNode(std::string filePath, WT::WTLayer* parentNode /*= nullptr*/)
{
	osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filePath);
	if (node)
	{
		std::string name = osgDB::getSimpleFileName(filePath);
		osg::ref_ptr<WT::WTLayer> oneLayer = new WT::WTLayer(node, name);
		oneLayer->setValue(0, false);
		this->addLayer(oneLayer.get(), parentNode);
	}
	return node;
}

osg::ref_ptr<osgGA::EventQueue> SceneManager::getEventQueue()
{
	return std::move(eventQueue);
}

WT::WTLayer* SceneManager::getLayer(std::string findInfo, FINDLAYERTYPE findType /*= FINDLAYERTYPE::NAME*/)
{
	if (root == nullptr) return nullptr;

	FindLayer findNodeVisitor(findInfo,findType);
	root->accept(findNodeVisitor);
	return findNodeVisitor.getLayer();
}

void SceneManager::switchLayerVisibility(std::string layerInfo, std::optional<bool> visibility, FINDLAYERTYPE findType /*= FINDLAYERTYPE::NAME*/) {
	WT::WTLayer* oneLayer = getLayer(layerInfo, findType);
	if (!oneLayer) return;
	oneLayer->switchVisibility(visibility);
}

void SceneManager::zoomToLayer(std::string layerInfo, FINDLAYERTYPE findType /*= FINDLAYERTYPE::NAME*/)
{
	WT::WTLayer* oneLayer = getLayer(layerInfo, findType);
	if (!oneLayer) return;
	oneLayer->zoomToLayer(viewer->getCameraManipulator());
}

osg::ref_ptr<osg::Group> SceneManager::getRoot()
{
	return root;
}

void SceneManager::switchLayerVisibilityByUIDSlot(std::string UID) {
	switchLayerVisibility(UID, std::nullopt, FINDLAYERTYPE::UID);
}
void SceneManager::zoomToLayerByUIDSlot(std::string UID)
{
	zoomToLayer(UID, FINDLAYERTYPE::UID);
}


void SceneManager::initOSG()
{
	if (viewer) return;

	eventQueue = new osgGA::EventQueue;
	operationQueue = new osg::OperationQueue;
	viewer = new osgViewer::Viewer;

	auto manipulator = new osgGA::TrackballManipulator;
	manipulator->setAllowThrow(false);
	viewer->setCameraManipulator(manipulator);
	viewer->setUpViewerAsEmbeddedInWindow(0, 0, 300, 300);
	viewer->getCamera()->setClearColor(osg::Vec4(56 / 255.0, 56 / 255.0, 56 / 255.0,1.0));//现在用的偏亮点，后面这个偏黑些(30 / 255.0, 31 / 255.0, 34 / 255.0, 1.0));
	auto graphicsWindow = dynamic_cast<osgViewer::GraphicsWindow*>(viewer->getCamera()->getGraphicsContext());
	graphicsWindow->setEventQueue(eventQueue);
	viewer->setUpdateOperations(operationQueue);

	root = new osg::Group;
	root->setUserValue("uid", nanoid::NanoID::generate());

	//osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("D:\\Code\\OSG\\OSGHandler\\TestData\\Data\\Tile_+011_+014\\Tile_+011_+014.osgb");
	//if (node)
	//{
	//	root->addChild(node);
	//}
	viewer->setSceneData(root.get());

	std::string  parentUID;
	root->getUserValue("uid", parentUID);
	emit(nodeLoaded(root->getName(), parentUID, ""));
}

void SceneManager::setupEarth()
{
	viewer = new osgViewer::Viewer();
	osg::ref_ptr<osgEarth::Map> map = new osgEarth::Map();
	osg::ref_ptr<osgEarth::XYZImageLayer> googleImageLayer = new osgEarth::XYZImageLayer;
	googleImageLayer->setURL("http://webst0[1234].is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}");
	googleImageLayer->setProfile(osgEarth::Profile::create("spherical-mercator"));
	googleImageLayer->setMaxLevel(20);
	map->addLayer(googleImageLayer.get());
	osgEarth::MapNode* mapNode = new osgEarth::MapNode(map.get());

	viewer->setSceneData(mapNode);
	viewer->setCameraManipulator(new osgEarth::EarthManipulator());
	//viewer->run();


	//osg::ref_ptr<osgEarth::SkyNode> earthSky = osgEarth::SkyNode::create();
	//earthSky->attach(viewer);
	////earthSky->getSunLight()->setAmbient(osg::Vec4(0.5, 0.5, 0.5, 1.0));
	//earthSky->setDateTime(osgEarth::DateTime(2020, 8, 15, 4));//��������ʱ��8Сʱ
	//earthSky->setLighting(0);

	////����������MapNode �����ùȸ�Ӱ��
	//map = new osgEarth::Map;
	////osg::ref_ptr<XYZImageLayer> googleImageLayer = new XYZImageLayer;
	////googleImageLayer->setURL("http://mt1.google.cn/vt/lyrs=s&hl=zh-CN&x={x}&y={y}&z={z}");
	////googleImageLayer->setProfile(Profile::create("spherical-mercator"));
	////googleImageLayer->setMaxLevel(23);
	//osg::ref_ptr<osgEarth::XYZImageLayer> googleImageLayer = new osgEarth::XYZImageLayer;
	//googleImageLayer->setURL("http://webst0[1234].is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}");
	//googleImageLayer->setProfile(osgEarth::Profile::create("spherical-mercator"));
	//googleImageLayer->setMaxLevel(20);
	//map->addLayer(googleImageLayer);
	//mapNode = new osgEarth::MapNode(map);

	//earthSky->addChild(mapNode);

	//root->addChild(earthSky);

	//osgEarth::LogarithmicDepthBuffer buf;
	//buf.install(viewer->getCamera());
	//earthManipulator = new osgEarth::EarthManipulator();
	//viewer->setCameraManipulator(earthManipulator);
	//osg::ref_ptr<osgEarth::EarthManipulator::Settings> earthManipulatorSettings = earthManipulator->getSettings();
	////earthManipulator->getSettings()->setMinMaxPitch(-89, 89);

	//viewer->getCamera()->setSmallFeatureCullingPixelSize(-1.0f);
	//// default uniform values:
	//osgEarth::GLUtils::setGlobalDefaults(viewer->getCamera()->getOrCreateStateSet());
	//viewer->getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, false);

	//double equatorRadius = map->getSRS()->getEllipsoid().getRadiusEquator();//6378137.0
	//earthManipulator->setHomeViewpoint(osgEarth::Viewpoint("", 114, 26, 0, 0, -90, equatorRadius * 3.5)); 
}

void SceneManager::setupEventHandler()
{
	if (viewer)
	{
		osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keySwitchManipulator = new osgGA::KeySwitchMatrixManipulator;

		keySwitchManipulator->addMatrixManipulator('1', "Trackball", new osgGA::TrackballManipulator);
		keySwitchManipulator->addMatrixManipulator('2', "Flight", new osgGA::FlightManipulator);
		keySwitchManipulator->addMatrixManipulator('3', "Drive", new osgGA::DriveManipulator);
		keySwitchManipulator->addMatrixManipulator('4', "Terrain", new osgGA::TerrainManipulator);
		keySwitchManipulator->addMatrixManipulator('5', "Orbit", new osgGA::OrbitManipulator);
		keySwitchManipulator->addMatrixManipulator('6', "FristPerson", new osgGA::FirstPersonManipulator);
		keySwitchManipulator->addMatrixManipulator('7', "Spherical", new osgGA::SphericalManipulator);

		viewer->setCameraManipulator(keySwitchManipulator.get());

		// add the state manipulator
		viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));

		// add the thread model handler
		viewer->addEventHandler(new osgViewer::ThreadingHandler);
		//// add the window size toggle handler
		viewer->addEventHandler(new osgViewer::WindowSizeHandler);
		//// add the stats handler
		viewer->addEventHandler(new osgViewer::StatsHandler);
		//// add the record camera path handler
		viewer->addEventHandler(new osgViewer::RecordCameraPathHandler);
		// add the LOD Scale handler
		viewer->addEventHandler(new osgViewer::LODScaleHandler);
		// add the screen capture handler
		viewer->addEventHandler(new osgViewer::ScreenCaptureHandler);
		viewer->addEventHandler(new osgViewer::WindowSizeHandler());
	}
}

void SceneManager::commonSettings()
{
	osgEarth::initialize();
	std::string exeFolder=osgDB::getCurrentWorkingDirectory();
	CPLSetConfigOption("GDAL_DATA", osgDB::concatPaths(exeFolder, "gdal-data").c_str());
	CPLSetConfigOption("PROJ_LIB", osgDB::concatPaths(exeFolder, "proj9/share").c_str());
	std::string test = osgDB::concatPaths(exeFolder, "proj9\\share");
	const char* newprojs[] = { osgDB::concatPaths(exeFolder, "proj9\\share").c_str(),NULL };
	OSRSetPROJSearchPaths(newprojs);
}
