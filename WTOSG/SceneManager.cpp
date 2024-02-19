#include "SceneManager.h"
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgDB/FileNameUtils>
#include <osg/Switch>

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
	initOSG();
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
	emit(nodeLoaded(layer->getName(), layer->getUID(), parentUID));
}

osg::ref_ptr<osg::Node> SceneManager::addNode(std::string filePath, WT::WTLayer* parentNode /*= nullptr*/)
{
	osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filePath);
	if (node)
	{
		std::string name = osgDB::getSimpleFileName(filePath);
		osg::ref_ptr<WT::WTLayer> oneLayer = new WT::WTLayer(node, name);
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

osg::ref_ptr<osg::Group> SceneManager::getRoot()
{
	return root;
}

void SceneManager::switchLayerVisibilitySlot(std::string UID) {
	switchLayerVisibility(UID, std::nullopt, FINDLAYERTYPE::UID);
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
	osg::ref_ptr<osg::Node> node = osgDB::readNodeFile("D:\\Code\\OSG\\OSGHandler\\TestData\\Data\\Tile_+011_+014\\Tile_+011_+014.osgb");
	if (node)
	{
		root->addChild(node);
	}
	viewer->setSceneData(root.get());

	std::string  parentUID;
	root->getUserValue("uid", parentUID);
	emit(nodeLoaded(root->getName(), parentUID, ""));
}
