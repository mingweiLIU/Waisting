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
	void addLayer(WT::WTLayer* layer, WT::WTLayer* parentLayer = nullptr);
	//��ȡ�ļ�������
	osg::ref_ptr<osg::Node> addNode(std::string filePath, WT::WTLayer* parentNode = nullptr);

	osg::ref_ptr<osgGA::EventQueue> getEventQueue();
	//�������ֻ�ȡͼ��
	WT::WTLayer* getLayer(std::string findInfo, FINDLAYERTYPE findType= FINDLAYERTYPE::NAME);
	//�л�ͼ������
	void switchLayerVisibility(std::string layerInfo,std::optional<bool> visibility, FINDLAYERTYPE findType = FINDLAYERTYPE::NAME);
	//�ɵ�ĳ��ͼ��
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
	//ͼ�������л��¼�
	void switchLayerVisibilityByUIDSlot(std::string UID);
	//�ɵ�ĳ��ͼ���¼�
	void zoomToLayerByUIDSlot(std::string UID);
signals:
	//�ڵ������Ϣ node��group��ʹ����
	void nodeLoaded(std::string name, std::string uid, std::string parentUID,bool visible=true);
	//�ڵ�ɾ����Ϣ
	void nodeDeleted(std::string name, std::string uid);
	//�ڵ������л���Ϣ
	void nodeSwitchVisible(std::string name, std::string uid);
};
