#ifndef NODEHANDLEVISITOR_H
#define NODEHANDLEVISITOR_H
#include "WTOSGDefines.h"
#include <osg/NodeVisitor>
#include <osg/Geode>
#include <osg/PagedLOD>

WTNAMESPACESTART
class NodeVertexHandler : public osg::NodeVisitor
{ 
public:
	NodeVertexHandler(osg::Vec3d transVec = osg::Vec3d(0, 0, 0), osg::Vec3d roateVec = osg::Vec3d(0, 0, 0), osg::Vec3d scaleVec = osg::Vec3d(1, 1, 1));

	NodeVertexHandler(
		float xOffet = 0, float yOffset = 0, float zOffset = 0
		, float xAngle = 0, float yAngle = 0, float zAngle = 0
		, float xScale = 1, float yScale = 1, float zScale = 1);

	virtual void apply(osg::Geode& geode);
	virtual void apply(osg::PagedLOD& pLod);

	//这里设置顶点变换前处理函数和顶点变换后函数
	void  setVertexComputeFunction(VertexCalcFunction beforeCall = NULL, VertexCalcFunction afterCall = NULL);
private:
	osg::Vec3d translateVec, roateAngleVec, scaleVec;
	osg::Matrix transMat;
	VertexCalcFunction beforeCall;
	VertexCalcFunction afterCall;
};
WTNAMESPACEEND
#endif // NODEHANDLEVISITOR_H
