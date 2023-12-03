#include "WTOSGRenderer.h"
#include <QOpenGLFramebufferObject>
#include "SceneManager.h"
#include "WTOSGViewer.h"


WTOSGRenderer::WTOSGRenderer(WTOSGViewer* oneWTOSGViewer)
	:mWTOSGViewer(oneWTOSGViewer)
{
	mOSGViewer = SceneManager::getInstance().getViewer();
}

WTOSGRenderer::~WTOSGRenderer()
{}

void WTOSGRenderer::render()
{
	mOSGViewer->frame();
}

QOpenGLFramebufferObject* WTOSGRenderer::createFramebufferObject(const QSize& size)
{
	QOpenGLFramebufferObjectFormat format;
	format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	format.setSamples(4);
	auto fbo = new QOpenGLFramebufferObject(size, format);
	mWTOSGViewer->setDefaultFbo(fbo);
	return fbo;
}

void WTOSGRenderer::synchronize(QQuickFramebufferObject* item)
{
	mWTOSGViewer = qobject_cast<WTOSGViewer*>(item);
}
