#ifndef WTOSGVIEWER_H
#define WTOSGVIEWER_H

//QT 
#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <QTimer>

#include <osg/OperationThread>

class WTOSGViewer : public QQuickFramebufferObject
{
	Q_OBJECT

	Q_PROPERTY(bool hoverEnabled READ hoverEnabled WRITE setHoverEnabled NOTIFY hoverEnabledChanged)
	Q_PROPERTY(QOpenGLFramebufferObject* defaultFbo READ defaultFbo NOTIFY defaultFboChanged)

public:
    WTOSGViewer(QQuickItem* parent = nullptr);

	Q_INVOKABLE void addOperation(osg::Operation* operation);

	//这个和PROPERTY对应的 而且set和get配对 get直接不用get这几个字
	QOpenGLFramebufferObject* defaultFbo()const {
		return m_defaultFbo;
	}
	void setDefaultFbo(QOpenGLFramebufferObject* newDefaultFbo);

	bool hoverEnabled() { return m_hoverEnabled; }
	void setHoverEnabled(bool newHoverEnabled);

signals:
	void framerateChanged();
	void hoverEnabledChanged();
	void defaultFboChanged(QOpenGLFramebufferObject* defaultFbo);

	//模型文件加载对外消息
	//void loadFileEvent(QString filePath);
	void loadedFile(std::string name, std::string ui, std::string parentUID);


public slots:
	//加载模型文件响应消息
	void slot_loadFile(QString filePath,QString loadType);
protected:
	virtual QQuickFramebufferObject::Renderer* createRenderer() const override; 
	virtual void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void hoverMoveEvent(QHoverEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual void keyPressEvent(QKeyEvent* event) override;
	virtual void keyReleaseEvent(QKeyEvent* event) override;

private:
	void setKeyboardModifiers(QInputEvent* event);
	void setMouseEventData(QMouseEvent* event);
	void setHoverEventData(QHoverEvent* event);

	int getOsgKey(QKeyEvent* event);


private:
    QTimer m_updateTimer;
	QOpenGLFramebufferObject* m_defaultFbo = nullptr;
	int m_framerate = 60;	
	bool m_hoverEnabled = false;

	int m_keyMask;
	int m_mouseX, m_mouseY;
	int m_mouseBtn;   //Button numbering is 1 for left mouse button, 2 for middle, 3 for right.
};
Q_DECLARE_METATYPE(osg::Operation*) //这个需要和定义的调用函数中的参数一致 且在初始化中 qRegisterMetaType<osg::Operation*>();
#endif // WTOSGVIEWER_H
