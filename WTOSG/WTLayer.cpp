#include "WTLayer.h"
#include "NanoID/nanoid.h"
WTNAMESPACESTART

WTLayer::WTLayer(osg::ref_ptr<osg::Node> node, std::string name /*= ""*/, bool visible /*= false*/, WTLAYERTYPE layerType /*= WTLAYERTYPE::NODE*/)
	:visible(visible)
	,layerType(layerType)
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
	if (visibility) {
		this->setValue(0, *visibility);
		this->visible = *visibility;
	}
	else {
		this->visible = !this->visible;
		this->setValue(0, this->visible);
	}
}

WTNAMESPACEEND