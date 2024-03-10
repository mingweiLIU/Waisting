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

//��������
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

	//���ͼ��
	void addLayer(osg::ref_ptr<osgEarth::VisibleLayer> layer, osg::ref_ptr<osgEarth::Map> map = nullptr);
	//��ȡ�ļ�������
	osg::ref_ptr<osg::Node> addNode(std::string filePath, osgEarth::VisibleLayer::Options modelOptions, osg::ref_ptr<osgEarth::Map> map = nullptr);
	//��ȡosgb�ļ�������
	osg::ref_ptr<osg::Node> addDSMGroup(std::string filePath, osg::ref_ptr<osgEarth::Map> map = nullptr);

	osg::ref_ptr<osgGA::EventQueue> getEventQueue();
	//����map��layer��uid��ȡͼ��
	osgEarth::Layer* getLayer(int layerUID, int mapUID);
	//����ͼ���UID��ȡͼ��
	osgEarth::Layer* getLayer(int layerUID);
	//���ݵ�ͼ��UID��ȡͼ��
	osgEarth::Map* getMap(int mapUID);

	//�л�ͼ������
	void switchLayerVisibility(int mapUID,int layerUID,std::optional<bool> visibility);
	//�ɵ�ĳ��ͼ��
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
	//ͼ�������л��¼�
	void slot_switchLayerVisibilityByUID(int layerUID,int mapUID);
	//�ɵ�ĳ��ͼ���¼�
	void slot_zoomToLayerByUID(int layerUID,int mapUID);
signals:
	//�ڵ������Ϣ node��group��ʹ����
	void nodeLoaded(std::string name, int uid, std::optional<int> parentUID,bool visible=true);
	//�ڵ�ɾ����Ϣ
	void nodeDeleted(std::string name, int uid);
	//�ڵ������л���Ϣ
	void nodeSwitchVisible(std::string name, int uid);
};
