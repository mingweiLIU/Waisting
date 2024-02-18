#include "SceneManager.h"
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgDB/FileNameUtils>

#include "NanoID/nanoid.h"

#include "OperationTools.h"

class FindNameNode : public osg::NodeVisitor
{
public:
	explicit FindNameNode(const std::string& name)
		:osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
		, m_name(name) {}

	virtual void apply(osg::Node& node)override {
		if (node.getName() == m_name)
		{
			m_node = &node;
		}
		else
		{
			traverse(node);
		}
	}
	osg::Node* getNode() const { return m_node; }
private:
	std::string m_name;
	osg::Node* m_node = nullptr;
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

void SceneManager::addNode(osg::Node* childNode, osg::Group* parent /*= nullptr*/)
{
	auto parentGroupNode = (parent == nullptr) ? root.get()->asGroup() : parent->asGroup();
	if (parentGroupNode == nullptr) {
		return;
	}
	auto op = new AddChildOperation(childNode, parentGroupNode);
	addOperation(op);
}

osg::ref_ptr<osg::Node> SceneManager::addNode(std::string filePath, osg::Group* parentNode /*= nullptr*/)
{
	osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filePath);
	std::string nodeUID = nanoid::NanoID::generate();
	node->setUserValue("uid", nodeUID);
	node->setName(osgDB::getSimpleFileName(filePath));
	if (node)
	{
		this->addNode(node.get(), parentNode);
	}

	std::string  parentUID="";
	if (parentNode)
	{
		parentNode->getUserValue("uid", parentUID);
	}
	emit(nodeLoaded(node->getName(), nodeUID, parentUID));
	return node;
}

osg::ref_ptr<osgGA::EventQueue> SceneManager::getEventQueue()
{
	return std::move(eventQueue);
}

osg::Node* SceneManager::getNode(std::string name)
{
	if (root == nullptr) return nullptr;

	FindNameNode findNodeVisitor(name);
	root->accept(findNodeVisitor);
	auto nodeFound = findNodeVisitor.getNode();
	if (nodeFound == nullptr) {
		return nullptr;
	}
	return nodeFound;
}

osg::ref_ptr<osg::Group> SceneManager::getRoot()
{
	return root;
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
