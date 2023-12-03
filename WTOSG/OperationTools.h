#pragma once
#include <osg/OperationThread>
#include <osg/Group>
#include <osg/Node>

class CameraLookAtOperation : public osg::Operation {
public:
	explicit CameraLookAtOperation(osg::Vec3d eye, osg::Vec3d center, osg::Vec3d up)
		:osg::Operation("change camera lookat operation", false)
		, m_eye(eye), m_center(center), m_up(up) {}

	virtual void operator()(osg::Object* caller)override {
		auto viewer = dynamic_cast<osgViewer::Viewer*>(caller);
		try
		{
			osgGA::TrackballManipulator* m = reinterpret_cast<osgGA::TrackballManipulator*>(viewer->getCameraManipulator());
			m->setTransformation(m_eye, m_center, m_up);
		}
		catch (...)
		{
		}
	}
private:
	osg::Vec3d m_eye, m_center, m_up;
};


class AddChildOperation : public osg::Operation
{
public:
	explicit AddChildOperation(osg::Node* childNode, osg::Group* parentNode)
		: osg::Operation("add child node operation", false)
		, m_childNode(childNode), m_parentNode(parentNode)
	{}
	virtual void operator()(osg::Object* caller) override
	{
		m_parentNode->addChild(m_childNode.get());
	}
private:
	osg::ref_ptr<osg::Node> m_childNode;
	osg::ref_ptr<osg::Group> m_parentNode;
};

class GeometryChangeOperation : public osg::Operation
{
public:
	explicit GeometryChangeOperation(int x, int y, int width, int height)
		: osg::Operation("geometry change operation", false),
		m_x(x), m_y(y), m_width(width), m_height(height) {}

	virtual void operator()(osg::Object* caller) override
	{
		auto viewer = dynamic_cast<osgViewer::Viewer*>(caller);
		auto graphicsWindow = dynamic_cast<osgViewer::GraphicsWindow*>(viewer->getCamera()->getGraphicsContext());
		graphicsWindow->resized(m_x, m_y, m_width, m_height);
	}
	int m_x, m_y, m_width, m_height;
};