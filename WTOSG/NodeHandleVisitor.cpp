#include "NodeHandleVisitor.h"

WT::NodeVertexHandler::NodeVertexHandler(osg::Vec3d transVec /*= osg::Vec3d(0, 0, 0)*/, osg::Vec3d roateVec /*= osg::Vec3d(0, 0, 0)*/, osg::Vec3d scaleVec /*= osg::Vec3d(1, 1, 1)*/)
{
	this->translateVec = transVec;
	this->scaleVec = scaleVec;
	this->roateAngleVec = roateVec;
	float xAngle = roateVec[0], yAngle = roateVec[1], zAngle = roateVec[2];

	this->transMat = osg::Matrix::translate(this->translateVec)
		* osg::Matrix::scale(this->scaleVec)
		* osg::Matrix::rotate(
			osg::DegreesToRadians(xAngle), osg::X_AXIS,
			osg::DegreesToRadians(yAngle), osg::Y_AXIS,
			osg::DegreesToRadians(zAngle), osg::Z_AXIS
		);
}

WT::NodeVertexHandler::NodeVertexHandler(float xOffet /*= 0*/, float yOffset /*= 0*/, float zOffset /*= 0 */, float xAngle /*= 0*/, float yAngle /*= 0*/, float zAngle /*= 0 */, float xScale /*= 1*/, float yScale /*= 1*/, float zScale /*= 1*/)
{
	this->translateVec = osg::Vec3d(xOffet, yOffset, zOffset);
	this->scaleVec = osg::Vec3d(xScale, yScale, zScale);
	this->roateAngleVec = osg::Vec3d(xAngle, yAngle, zAngle);

	this->transMat = osg::Matrix::translate(this->translateVec) *
		osg::Matrix::scale(this->scaleVec) *
		osg::Matrix::rotate(
			osg::DegreesToRadians(xAngle), osg::X_AXIS,
			osg::DegreesToRadians(yAngle), osg::Y_AXIS,
			osg::DegreesToRadians(zAngle), osg::Z_AXIS
		);
}

void WT::NodeVertexHandler::apply(osg::Geode& geode)
{
	for (int i = 0, i_up = geode.getNumDrawables(); i < i_up; i++)
	{
		osg::Geometry* geom = geode.getDrawable(i)->asGeometry();
		osg::Vec3Array* coordArray = (osg::Vec3Array*)geom->getVertexArray();
		unsigned int numVertex = coordArray->size();

		for (auto it = coordArray->begin(); it != coordArray->end(); it++)
		{
			osg::Vec3d oriVertex = osg::Vec3d(*it);

			//转换前修改
			if (beforeCall)
			{
				oriVertex = beforeCall(oriVertex);
			}
			osg::Vec3d transformedVertex = this->transMat.preMult(oriVertex);

			//修改回归
			if (afterCall)
			{
				transformedVertex = afterCall(transformedVertex);
			}
			*it = transformedVertex;
		}
		geom->setVertexArray(coordArray);
	}
	traverse(geode);
}

void WT::NodeVertexHandler::apply(osg::PagedLOD& pLod)
{

	pLod.setDatabasePath("");
	traverse(pLod);
}

void WT::NodeVertexHandler::setVertexComputeFunction(VertexCalcFunction beforeCall /*= NULL*/, VertexCalcFunction afterCall /*= NULL*/)
{
	this->beforeCall = beforeCall;
	this->afterCall = afterCall;
}
