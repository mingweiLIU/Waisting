#pragma once
#include <QQuickFramebufferObject>
#include <osgViewer/Viewer>
class WTOSGViewer;

class WTOSGRenderer : public QQuickFramebufferObject::Renderer
{
public:
	WTOSGRenderer(WTOSGViewer* oneWTOSGViewer);
	~WTOSGRenderer();

protected:
	virtual void render() override;
	virtual QOpenGLFramebufferObject* createFramebufferObject(const QSize& size) override;
	virtual void synchronize(QQuickFramebufferObject*) override;
private:
	WTOSGViewer* mWTOSGViewer;
	osg::ref_ptr<osgViewer::Viewer> mOSGViewer;
};
