#include "WTOSGViewer.h"
#include <QQuickWindow>
#include <QQuickFramebufferObject>
#include <osgGA/EventQueue>
#include <osg/OperationThread>
#include "SceneManager.h"
#include "WTOSGRenderer.h"

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

WTOSGViewer::WTOSGViewer(QQuickItem* parent /*= nullptr*/)
{
	setMirrorVertically(true);
	setTextureFollowsItemSize(true);
	setAcceptedMouseButtons(Qt::AllButtons);

	qRegisterMetaType<osg::Operation*>();

	m_updateTimer.setInterval(1000 / m_framerate);
	connect(&m_updateTimer, &QTimer::timeout, this, [this]() {update(); });
	m_updateTimer.start();
}

void WTOSGViewer::addOperation(osg::Operation* operation)
{
	SceneManager::getInstance().addOperation(operation);
}

void WTOSGViewer::setDefaultFbo(QOpenGLFramebufferObject* newDefaultFbo)
{
	if (m_defaultFbo == newDefaultFbo) return;
	m_defaultFbo = newDefaultFbo;
	emit defaultFboChanged(m_defaultFbo);
}

void WTOSGViewer::setHoverEnabled(bool newHoverEnabled)
{
	if (m_hoverEnabled == newHoverEnabled) return;
	m_hoverEnabled = newHoverEnabled;
	setAcceptHoverEvents(m_hoverEnabled);
	emit hoverEnabledChanged();
}

QQuickFramebufferObject::Renderer* WTOSGViewer::createRenderer() const
{
	auto viewer = const_cast<WTOSGViewer*>(this);
	auto renderer = new WTOSGRenderer(viewer);
	return renderer;
}

void WTOSGViewer::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
	if (newGeometry.width() > 0 && newGeometry.height() > 0)
	{
		QQuickWindow* ptrWindow = window();
		qreal retinaScale = 1.0f;
		if (ptrWindow != NULL)
		{
			retinaScale = ptrWindow->devicePixelRatio();
		}

		GeometryChangeOperation* geomChangeOperation =
			new GeometryChangeOperation(newGeometry.x(), newGeometry.y(), newGeometry.width() * retinaScale, newGeometry.height() * retinaScale);
		SceneManager::getInstance().addOperation(geomChangeOperation);
		SceneManager::getInstance().getEventQueue()->windowResize(newGeometry.x(), newGeometry.y(), newGeometry.width() * retinaScale, newGeometry.height() * retinaScale);
	}
	QQuickFramebufferObject::geometryChange(newGeometry, oldGeometry);
	update();
}

void WTOSGViewer::mousePressEvent(QMouseEvent* event)
{
	setKeyboardModifiers(event);
	setMouseEventData(event);
	SceneManager::getInstance().getEventQueue()->getCurrentEventState()->setModKeyMask(m_keyMask);
	SceneManager::getInstance().getEventQueue()->mouseButtonPress(m_mouseX, m_mouseY, m_mouseBtn);
	update();
}

void WTOSGViewer::mouseDoubleClickEvent(QMouseEvent* event)
{
	setKeyboardModifiers(event);
	setMouseEventData(event);
	SceneManager::getInstance().getEventQueue()->getCurrentEventState()->setModKeyMask(m_keyMask);
	SceneManager::getInstance().getEventQueue()->mouseDoubleButtonPress(m_mouseX, m_mouseY, m_mouseBtn);
	update();
}

void WTOSGViewer::mouseMoveEvent(QMouseEvent* event)
{
	setMouseEventData(event);
	SceneManager::getInstance().getEventQueue()->mouseMotion(m_mouseX, m_mouseY);
}

void WTOSGViewer::hoverMoveEvent(QHoverEvent* event)
{
	QQuickItem::hoverMoveEvent(event);

	setHoverEventData(event);
	SceneManager::getInstance().getEventQueue()->mouseMotion(m_mouseX, m_mouseY);
}

void WTOSGViewer::mouseReleaseEvent(QMouseEvent* event)
{
	setKeyboardModifiers(event);
	setMouseEventData(event);
	SceneManager::getInstance().getEventQueue()->getCurrentEventState()->setModKeyMask(m_keyMask);
	SceneManager::getInstance().getEventQueue()->mouseButtonRelease(m_mouseX, m_mouseY, m_mouseBtn);
	update();
}

void WTOSGViewer::wheelEvent(QWheelEvent* event)
{
	setKeyboardModifiers(event);
	SceneManager::getInstance().getEventQueue()->mouseScroll(event->angleDelta().y() < 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN);

	QQuickItem::wheelEvent(event);
	update();
}

void WTOSGViewer::keyPressEvent(QKeyEvent* event)
{
	qDebug() << "keyPressEvent";
	int nKey = getOsgKey(event);
	SceneManager::getInstance().getEventQueue()->keyPress((osgGA::GUIEventAdapter::KeySymbol)nKey);
	QQuickFramebufferObject::keyPressEvent(event);
}

void WTOSGViewer::keyReleaseEvent(QKeyEvent* event)
{
	qDebug() << "keyReleaseEvent";
	int nKey = getOsgKey(event);
	SceneManager::getInstance().getEventQueue()->keyRelease((osgGA::GUIEventAdapter::KeySymbol)nKey);
}

void WTOSGViewer::setKeyboardModifiers(QInputEvent* event)
{
	int modkey = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
	m_keyMask = 0;
	if (modkey & Qt::ShiftModifier)
	{
		m_keyMask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
	}
	if (modkey & Qt::ControlModifier)
	{
		m_keyMask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
	}
	if (modkey & Qt::AltModifier)
	{
		m_keyMask |= osgGA::GUIEventAdapter::MODKEY_ALT;
	}
}

void WTOSGViewer::setMouseEventData(QMouseEvent* event)
{
	const qreal retinaScale = window()->devicePixelRatio();
	m_mouseX = event->x() * retinaScale;
	m_mouseY = event->y() * retinaScale;
	switch (event->button())
	{
	case Qt::LeftButton:
		m_mouseBtn = 1;
		break;
	case Qt::MiddleButton:
		m_mouseBtn = 2;
		break;
	case Qt::RightButton:
		m_mouseBtn = 3;
		break;
	default:
		m_mouseBtn = 0;
		break;
	}
}

void WTOSGViewer::setHoverEventData(QHoverEvent* event)
{
	const qreal retinaScale = window()->devicePixelRatio();
	m_mouseX = event->pos().x() * retinaScale;
	m_mouseY = event->pos().y() * retinaScale;
	m_mouseBtn = 0;
}

int WTOSGViewer::getOsgKey(QKeyEvent* event)
{
	int nKey = event->key();
	QString sTxt = event->text();
	if (!sTxt.isEmpty())
	{
		switch (nKey)
		{
		case Qt::Key_Home:
			return osgGA::GUIEventAdapter::KEY_Home;
		case Qt::Key_Left:
			return osgGA::GUIEventAdapter::KEY_Left;
		case Qt::Key_Up:
			return osgGA::GUIEventAdapter::KEY_Up;
		case Qt::Key_Right:
			return osgGA::GUIEventAdapter::KEY_Right;
		case Qt::Key_Down:
			return osgGA::GUIEventAdapter::KEY_Down;
		case Qt::Key_End:
			return osgGA::GUIEventAdapter::KEY_End;
		case Qt::Key_D:
			return osgGA::GUIEventAdapter::KEY_D;
		case Qt::Key_Shift:
			return osgGA::GUIEventAdapter::KEY_Shift_L;
		default:
			return nKey;
		}
	}

	return nKey;
}
