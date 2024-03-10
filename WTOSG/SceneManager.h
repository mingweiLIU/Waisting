#pragma once
#include <QObject>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/Node>
#include <osg/OperationThread>
#include <osgGA/EventQueue>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarth/EarthManipulator>

#include"WTLayer.h"

//查找类型
enum class FINDLAYERTYPE
{
	UID = 0,
	NAME
};

class SceneManager:public QObject
{
	Q_OBJECT
protected:
	SceneManager();
public:
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	virtual ~SceneManager() {};

	static SceneManager& getInstance();
	osg::ref_ptr<osgViewer::Viewer> getViewer();
	void addOperation(osg::Operation* operation);

	//添加图层
	void addLayer(osg::ref_ptr<osgEarth::VisibleLayer> layer, osg::ref_ptr<osgEarth::Map> map = nullptr);
	//读取文件并加载
	osg::ref_ptr<osg::Node> addNode(std::string filePath, osgEarth::VisibleLayer::Options modelOptions, osg::ref_ptr<osgEarth::Map> map = nullptr);
	//读取osgb文件并加载
	osg::ref_ptr<osg::Node> addDSMGroup(std::string filePath, osg::ref_ptr<osgEarth::Map> map = nullptr);

	osg::ref_ptr<osgGA::EventQueue> getEventQueue();
	//根据map和layer的uid获取图层
	osgEarth::Layer* getLayer(int layerUID, int mapUID);
	//根据图层的UID获取图层
	osgEarth::Layer* getLayer(int layerUID);
	//根据地图的UID获取图层
	osgEarth::Map* getMap(int mapUID);

	//切换图层显隐
	void switchLayerVisibility(int mapUID,int layerUID,std::optional<bool> visibility);
	//飞到某个图层
	void zoomToLayer(int mapUID, int layerUID);

	osg::ref_ptr<osgEarth::MapNode> getMapNode();

	void setupEarth();
private:
	void initOSG();
	void setupEventHandler();
	void commonSettings();
private:
	bool setuped = false;
	osg::ref_ptr<osgViewer::Viewer> viewer;
	osg::ref_ptr<osgEarth::Map> rootMap;
	osg::ref_ptr<osg::OperationQueue> operationQueue;
	osg::ref_ptr<osgGA::EventQueue> eventQueue;
	osg::ref_ptr<osgEarth::MapNode> mapNode;
	osg::ref_ptr<osgEarth::EarthManipulator>earthManipulator;

public slots:
	//图层显隐切换事件
	void slot_switchLayerVisibilityByUID(int layerUID,int mapUID);
	//飞到某个图层事件
	void slot_zoomToLayerByUID(int layerUID,int mapUID);
signals:
	//节点加载消息 node和group都使用它
	void nodeLoaded(std::string name, int uid, std::optional<int> parentUID,bool visible=true);
	//节点删除消息
	void nodeDeleted(std::string name, int uid);
	//节点显隐切换消息
	void nodeSwitchVisible(std::string name, int uid);
};
