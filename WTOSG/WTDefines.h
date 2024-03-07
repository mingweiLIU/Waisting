#ifndef WTDEFINES_H
#define WTDEFINES_H
#include <functional>
#include <osg/Vec3d>

#define WTNAMESPACESTART namespace WT{
#define WTNAMESPACEEND }



typedef std::function<osg::Vec3d(osg::Vec3d)> VertexCalcFunction;


#endif // WTDEFINES_H