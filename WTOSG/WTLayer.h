#ifndef WTLAYER_H
#define WTLAYER_H
#include<optional>
#include<osg/Switch>
#include <osg/Camera>
#include <osgGA/CameraManipulator>

#include "WTDefines.h"

WTNAMESPACESTART
class WTLayer:public osg::Switch
{
    enum class WTLAYERTYPE
    {
        NODE=0,
        GROUP
    };
public:
    explicit WTLayer(osg::ref_ptr<osg::Node> node,std::string name="",bool visible =true,WTLAYERTYPE layerType=WTLAYERTYPE::NODE);

    std::string getUID();

    //修改可见性
    void switchVisibility(std::optional<bool> visibility);
    //缩放到图层
    void zoomToLayer(osgGA::CameraManipulator* cameraManipulator);
private:
    std::string uid;
    bool visible = true;
    WTLAYERTYPE layerType = WTLAYERTYPE::NODE; 
};
WTNAMESPACEEND
#endif // WTLAYER_H
