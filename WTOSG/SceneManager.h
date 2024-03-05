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
	void addLayer(WT::WTLayer* layer, WT::WTLayer* parentLayer = nullptr);
	//读取文件并加载
	osg::ref_ptr<osg::Node> addNode(std::string filePath, WT::WTLayer* parentNode = nullptr);

	osg::ref_ptr<osgGA::EventQueue> getEventQueue();
	//根据名字获取图层
	WT::WTLayer* getLayer(std::string findInfo, FINDLAYERTYPE findType= FINDLAYERTYPE::NAME);
	//切换图层显隐
	void switchLayerVisibility(std::string layerInfo,std::optional<bool> visibility, FINDLAYERTYPE findType = FINDLAYERTYPE::NAME);
	//飞到某个图层
	void zoomToLayer(std::string layerInfo, FINDLAYERTYPE findType = FINDLAYERTYPE::NAME);

	osg::ref_ptr<osg::Group> getRoot();
private:
	void initOSG();
	void setupEarth();
	void setupEventHandler();
	void commonSettings();
private:
	osgViewer::Viewer* viewer;
	osgEarth::Map* map;
	osg::ref_ptr<osg::Group> root;
	osg::ref_ptr<osg::OperationQueue> operationQueue;
	osg::ref_ptr<osgGA::EventQueue> eventQueue;
	osg::ref_ptr<osgEarth::MapNode> mapNode;
	osg::ref_ptr<osgEarth::EarthManipulator>earthManipulator;

public slots:
	//图层显隐切换事件
	void switchLayerVisibilityByUIDSlot(std::string UID);
	//飞到某个图层事件
	void zoomToLayerByUIDSlot(std::string UID);
signals:
	//节点加载消息 node和group都使用它
	void nodeLoaded(std::string name, std::string uid, std::string parentUID,bool visible=true);
	//节点删除消息
	void nodeDeleted(std::string name, std::string uid);
	//节点显隐切换消息
	void nodeSwitchVisible(std::string name, std::string uid);
};
