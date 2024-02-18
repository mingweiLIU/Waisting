#pragma once
#include <QObject>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/Node>
#include <osg/OperationThread>
#include <osgGA/EventQueue>


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
	void addNode(osg::Node* childNode, osg::Group* parentNode=nullptr);
	osg::ref_ptr<osg::Node> addNode(std::string filePath, osg::Group* parentNode = nullptr);
	osg::ref_ptr<osgGA::EventQueue> getEventQueue();
	osg::Node* getNode(std::string name);
	osg::ref_ptr<osg::Group> getRoot();
private:
	void initOSG();
private:
	osg::ref_ptr<osgViewer::Viewer>viewer;
	osg::ref_ptr<osg::Group> root;
	osg::ref_ptr<osg::OperationQueue> operationQueue;
	osg::ref_ptr<osgGA::EventQueue> eventQueue;

public slots:

signals:
	//节点加载消息 node和group都使用它
	void nodeLoaded(std::string name, std::string uid, std::string parentUID);
	//节点删除消息
	void nodeDeleted(std::string name, std::string uid);
	//节点显隐切换消息
	void nodeSwitchVisible(std::string name, std::string uid);
};
