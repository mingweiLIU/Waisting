#pragma once
#include <osg/OperationThread>
#include <osg/Group>
#include <osg/Node>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>

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

template<typename T_Child,typename T_Parent>
class AddChildOperation : public osg::Operation
{
public:
	explicit AddChildOperation(T_Child* childNode, T_Parent* parentNode)
		: osg::Operation("add child node operation", false)
		, m_childNode(childNode), m_parentNode(parentNode)
	{}
	virtual void operator()(osg::Object* caller) override
	{		
		if (std::is_same<T_Parent,osgEarth::Map>::value)
		{
			auto temp = dynamic_cast<osgEarth::Map*>(m_parentNode);
			auto childTemp = dynamic_cast<osgEarth::ModelLayer*>(m_childNode);
			temp->addLayer(childTemp);
		}
		else
		{
			auto temp = dynamic_cast<osg::Group*>(m_parentNode);
			auto childTemp = dynamic_cast<osg::Node*>(m_childNode);
			temp->addChild(childTemp);
		}
	}
private:
	T_Child* m_childNode;
	T_Parent* m_parentNode;
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