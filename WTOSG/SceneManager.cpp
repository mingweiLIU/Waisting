#include "SceneManager.h"
#include<stdlib.h>
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
#include <osgEarth/ModelLayer>
#include <osgEarth/ViewFitter>
#include <osgEarth/GeoTransform>
#include <osgEarth/Registry>
#include<osgEarth/ShaderGenerator>
#include <gdal_priv.h>
#include <cpl_conv.h>
#include <optional>

#include "WTUlits/NanoID/nanoid.h"
#include "OperationTools.h"
#include "DSMGroupLayer.h"

class FindLayer : public osg::NodeVisitor
{
public:
	explicit FindLayer(std::optional<int> layerUID=std::nullopt, std::optional<int> mapUID=std::nullopt)
		:osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
		, mapUID(mapUID)
		, layerUID(layerUID){}

	virtual void apply(osg::Group& node)override {
		if (layerUID == std::nullopt && mapUID == std::nullopt) return;
		osgEarth::Map* oneMap = dynamic_cast<osgEarth::Map*>(&node);
		if (mapUID==std::nullopt){
			//那么layerUID一定存在
			if (!oneMap){
				traverse(node);
			}
			else{
				map = oneMap;
				layer = oneMap->getLayerByUID(layerUID.value());
				if (!layer){
					traverse(node);
				}
			}
		}
		else {
			if (!oneMap) {
				traverse(node); return;
			}
			if (oneMap->getUID() != mapUID) {
				traverse(node); return;
			}
			map = oneMap;

			if (layerUID==std::nullopt) return; 
			else {
				layer = oneMap->getLayerByUID(layerUID.value());
			}
		}

	}
	osgEarth::Layer* getLayer() const { return layer; }
	osgEarth::Map* getMap() const { return map; }
private:
	std::optional<int> mapUID, layerUID;
	osgEarth::Layer* layer=nullptr;
	osgEarth::Map* map = nullptr;
};

SceneManager::SceneManager()
{
	commonSettings();
	initOSG();
	//setupEarth();
	//
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

//void SceneManager::addLayer(WT::WTLayer* layer, WT::WTLayer* parentLayer /*= nullptr*/)
//{
//	auto parentGroupNode = (parentLayer == nullptr) ? root : parentLayer;
//	
//	auto op = new AddChildOperation(layer, parentGroupNode);
//	addOperation(op);
//
//	std::string parentUID= (parentLayer == nullptr) ? "" : parentLayer->getUID();
//	emit(nodeLoaded(layer->getName(), layer->getUID(), parentUID, layer->getValue(0)));
//}

void SceneManager::addLayer(osgEarth::VisibleLayer* layer, osgEarth::Map* map /*= nullptr*/)
{
	osgEarth::Map* parentMap = (map == nullptr) ? rootMap.get() : map;
	auto op = new AddChildOperation<osgEarth::VisibleLayer, osgEarth::Map>(layer, parentMap);
	addOperation(op);

	std::string parentName = parentMap->getName();
	emit(nodeLoaded(layer->getName(), layer->getUID(), parentMap->getUID(), layer->getVisible()));
}

//osg::ref_ptr<osg::Node> SceneManager::addNode(std::string filePath, WT::WTLayer* parentNode /*= nullptr*/)
//{
//	osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filePath);
//	if (node)
//	{
//		std::string name = osgDB::getSimpleFileName(filePath);
//		osg::ref_ptr<WT::WTLayer> oneLayer = new WT::WTLayer(node, name);
//		oneLayer->setValue(0, false);
//		this->addLayer(oneLayer.get(), parentNode);
//	}
//	return node;
//}

osg::ref_ptr<osg::Node> SceneManager::addNode(std::string filePath, osgEarth::VisibleLayer::Options modelOptions, osg::ref_ptr<osgEarth::Map> map /*= nullptr*/)
{
	osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filePath);
	if (node)
	{
		std::string name = osgDB::getSimpleFileName(filePath);
		if (modelOptions.name() == "") modelOptions.set_name(name);
		osg::ref_ptr<osgEarth::ModelLayer>oneModelLayer = new osgEarth::ModelLayer(modelOptions);
		oneModelLayer->setNode(node);
		this->addLayer(oneModelLayer, map);
	}
	return node;
}

osg::ref_ptr<osg::Node> SceneManager::addDSMGroup(std::string filePath, osg::ref_ptr<osgEarth::Map> map /*= nullptr*/)
{
	osg::ref_ptr<WT::DSMGroup> dsmGroup = new WT::DSMGroup(filePath);
	if (dsmGroup->isOK)
	{
		osg::ref_ptr<osgEarth::GeoTransform> xform = new osgEarth::GeoTransform;
		xform->addChild(dsmGroup);
		xform->setPosition(dsmGroup->getCenterWGS84());
		std::string name = osgDB::getSimpleFileName(filePath);
		osgEarth::ModelLayer* oneModelLayer = new osgEarth::ModelLayer();
		oneModelLayer->setLocation(dsmGroup->getCenterWGS84());
		oneModelLayer->setName(name);
		oneModelLayer->setNode(xform.get());

		osgEarth::Registry::shaderGenerator().run(dsmGroup.get());
		//mapNode->addChild(xform.get());
		this->addLayer(oneModelLayer,rootMap.get());
	}
	return dsmGroup;
}

osg::ref_ptr<osgGA::EventQueue> SceneManager::getEventQueue()
{
	return std::move(eventQueue);
}

//根据map和layer的uid获取图层
osgEarth::Layer* SceneManager::getLayer(int layerUID,int mapUID)
{
	if (mapNode == nullptr) return nullptr;
	return rootMap->getLayerByUID(layerUID);
}

osgEarth::Layer* SceneManager::getLayer(int layerUID)
{
	if (mapNode == nullptr) return nullptr;

	FindLayer findNodeVisitor(layerUID);
	mapNode->accept(findNodeVisitor);
	return findNodeVisitor.getLayer();
}

osgEarth::Map* SceneManager::getMap(int mapUID)
{
	if (mapNode == nullptr) return nullptr;

	FindLayer findNodeVisitor(mapUID, std::nullopt);
	mapNode->accept(findNodeVisitor);
	return findNodeVisitor.getMap();
}

void SceneManager::switchLayerVisibility(int mapUID, int layerUID, std::optional<bool> visibility) {
	osgEarth::Layer* oneLayer = getLayer(layerUID, mapUID);
	osgEarth::VisibleLayer* oneVisibleLayer = dynamic_cast<osgEarth::VisibleLayer*>(oneLayer);
	if (!oneVisibleLayer) return;

	if (visibility==std::nullopt){
		oneVisibleLayer->setVisible(!oneVisibleLayer->getVisible());
	}
	else{
		oneVisibleLayer->setVisible(visibility.value());
	}
}

void SceneManager::zoomToLayer(int mapUID, int layerUID)
{
	osgEarth::Layer* oneLayer = getLayer(layerUID, mapUID);

	//这里根据不同类型得数据进行处理
	std::string typeName=oneLayer->getTypeName();
	if (typeid(osgEarth::ModelLayer).name()== typeName)
	{
		osgEarth::ModelLayer* oneModelLayer = dynamic_cast<osgEarth::ModelLayer*>(oneLayer);
		if (!oneModelLayer) return;

		osg::Group* modelGroup = dynamic_cast<osg::Group*>(oneModelLayer->getNode());
		osgEarth::GeoTransform* modelGeoTransform = dynamic_cast<osgEarth::GeoTransform*>(modelGroup->getChild(0));
		WT::DSMGroup* dsm= dynamic_cast<WT::DSMGroup*>(modelGeoTransform->getChild(0));
		if (!dsm) return;
		dsm->zoomToLayer(earthManipulator);
	}
	//osgEarth::ImageLayer* oneImageLayer = dynamic_cast<osgEarth::ImageLayer*>(oneLayer);
	//if (oneVisibleLayer)
	//{
	//	oneVisibleLayer
	//}

	//osgEarth::VisibleLayer* oneVisibleLayer = dynamic_cast<osgEarth::VisibleLayer*>(oneLayer);
	//if (!oneVisibleLayer) return;

	//osgEarth::ModelLayer* oneModelLayer = dynamic_cast<osgEarth::ModelLayer*>(oneLayer);
	//if (!oneModelLayer) return;
	//oneModelLayer->get

	//

	//oneVisibleLayer->getExtent();
	//osgEarth::Viewpoint vp;
	//osgEarth::ViewFitter fitter(mapNode->getMapSRS(),viewer->getCamera());
	//if (fitter.createViewpoint(oneVisibleLayer->getExtent(),vp))
	//{
	//	earthManipulator->setViewpoint(vp, 0.5);
	//}
}


osg::ref_ptr<osgEarth::MapNode> SceneManager::getMapNode()
{
	return mapNode;
}

void SceneManager::slot_switchLayerVisibilityByUID(int layerUID, int mapUID) {
	switchLayerVisibility(mapUID,layerUID, std::nullopt);
}
void SceneManager::slot_zoomToLayerByUID(int layerUID, int mapUID)
{
	zoomToLayer(mapUID,layerUID);
}


void SceneManager::initOSG()
{
	if (viewer|| setuped) return;

	eventQueue = new osgGA::EventQueue;
	operationQueue = new osg::OperationQueue;
	viewer = new osgViewer::Viewer;
	viewer->setUpViewerAsEmbeddedInWindow(0, 0, 300, 300);
	//viewer->getCamera()->setClearColor(osg::Vec4(56 / 255.0, 56 / 255.0, 56 / 255.0,1.0));//现在用的偏亮点，后面这个偏黑些(30 / 255.0, 31 / 255.0, 34 / 255.0, 1.0));
	auto graphicsWindow = dynamic_cast<osgViewer::GraphicsWindow*>(viewer->getCamera()->getGraphicsContext());
	graphicsWindow->setEventQueue(eventQueue);
	viewer->setUpdateOperations(operationQueue);
	setuped = true;
}

void SceneManager::setupEarth()
{
	osg::ref_ptr<osgEarth::SkyNode> earthSky = osgEarth::SkyNode::create();
	earthSky->attach(viewer);
	earthSky->setAtmosphereVisible(true);
	earthSky->getSunLight()->setAmbient(osg::Vec4(0.5, 0.5, 0.5, 1.0));
	earthSky->setDateTime(osgEarth::DateTime(2020, 8, 15, 4));//��������ʱ��8Сʱ
	earthSky->setLighting(1);

	//����������MapNode �����ùȸ�Ӱ��
	rootMap = new osgEarth::Map;
	osg::ref_ptr<osgEarth::XYZImageLayer> googleImageLayer = new osgEarth::XYZImageLayer;
	googleImageLayer->setURL("https://gac-geo.googlecnapps.cn/maps/vt?lyrs=s&x={x}&y={y}&z={z}");
	googleImageLayer->setProfile(osgEarth::Profile::create("spherical-mercator"));
	googleImageLayer->setMaxLevel(23);
	googleImageLayer->setName("googleMap");
	//osg::ref_ptr<osgEarth::XYZImageLayer> googleImageLayer = new osgEarth::XYZImageLayer;
	//googleImageLayer->setURL("http://webst0[1234].is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}");
	//googleImageLayer->setProfile(osgEarth::Profile::create("spherical-mercator"));
	//googleImageLayer->setMaxLevel(20);
	rootMap->addLayer(googleImageLayer.get());
	mapNode = new osgEarth::MapNode(rootMap.get());

	earthSky->addChild(mapNode.get());

	viewer->setSceneData(earthSky.get());

	osgEarth::LogarithmicDepthBuffer buf;
	buf.install(viewer->getCamera());
	earthManipulator = new osgEarth::EarthManipulator();
	viewer->setCameraManipulator(earthManipulator);
	osg::ref_ptr<osgEarth::EarthManipulator::Settings> earthManipulatorSettings = earthManipulator->getSettings();
	//earthManipulator->getSettings()->setMinMaxPitch(-89, 89);

	viewer->getCamera()->setSmallFeatureCullingPixelSize(-1.0f);
	// default uniform values:
	osgEarth::GLUtils::setGlobalDefaults(viewer->getCamera()->getOrCreateStateSet());
	viewer->getDatabasePager()->setUnrefImageDataAfterApplyPolicy(true, false);

	double equatorRadius = rootMap->getSRS()->getEllipsoid().getRadiusEquator();//6378137.0
	earthManipulator->setHomeViewpoint(osgEarth::Viewpoint("", 114, 26, 0, 0, -90, equatorRadius * 3.5)); 

	//初始化的时候就应该创建一个map的图层树和一个layer的图层树节点
	emit(nodeLoaded("tuceng", rootMap->getUID(),std::nullopt, true));
	emit(nodeLoaded(googleImageLayer->getName(), googleImageLayer->getUID(), rootMap->getUID(), googleImageLayer->getVisible()));
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
	std::string exeFolder=osgDB::getCurrentWorkingDirectory();
	CPLSetConfigOption("GDAL_DATA", osgDB::concatPaths(exeFolder, "gdal-data").c_str());
	std::string projPath = osgDB::concatPaths(exeFolder, "proj\\share");
	const char* proj_path[] = { projPath.c_str(), nullptr };
	OSRSetPROJSearchPaths(proj_path);
}
