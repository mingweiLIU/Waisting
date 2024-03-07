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
    explicit WTLayer() {};
    explicit WTLayer(osg::ref_ptr<osg::Node> node,std::string name="",bool visible =true,WTLAYERTYPE layerType=WTLAYERTYPE::NODE);

    std::string getUID();

    //�޸Ŀɼ���
    void switchVisibility(std::optional<bool> visibility);
    //���ŵ�ͼ��
    void zoomToLayer(osgGA::CameraManipulator* cameraManipulator);
    //��ȡ�ɼ���
    bool getVisibility();
private:
    std::string uid;
    WTLAYERTYPE layerType = WTLAYERTYPE::NODE; 
};
WTNAMESPACEEND
#endif // WTLAYER_H
