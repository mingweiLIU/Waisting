#include "WTLayer.h"
#include "NanoID/nanoid.h"
WTNAMESPACESTART

WTLayer::WTLayer(osg::ref_ptr<osg::Node> node, std::string name /*= ""*/, bool visible /*= false*/, WTLAYERTYPE layerType /*= WTLAYERTYPE::NODE*/)
	:layerType(layerType)
{
	uid=nanoid::NanoID::generate();
	if ("" == name) {
		name = uid;
	}
	this->setName(name);
	node->setName("data_"+uid);
	this->addChild(node);
}

std::string  WTLayer::getUID() {
	return this->uid;
}

void WTLayer::switchVisibility(std::optional<bool> visibility) {
	visibility ? this->setValue(0, *visibility) : this->setValue(0, !this->getVisibility());
}

void WTLayer::zoomToLayer(osgGA::CameraManipulator* cameraManipulator)
{
	double radius = this->getBound().radius();
	double viewDistance = radius * 2.5;
	osg::Vec3d viewDirection(0.0, -1.0, 1.5);
	osg::Vec3d center = this->getBound().center();
	osg::Vec3d eye = center + viewDirection * viewDistance;
	osg::Matrixd m = osg::Matrixd::lookAt(eye, center, osg::Vec3d(0, 0, 1));
	cameraManipulator->setByMatrix(osg::Matrixd::inverse(m));
}

bool WTLayer::getVisibility() {
	return this->getValue(0);
}

WTNAMESPACEEND