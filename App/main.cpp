#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuick/QQuickView>
#include <QIcon>
#include <osgEarth/Common>
#include<qsurfaceformat.h>
#include "WTOSGViewer.h"
#include "WLayerTreeModel.h"
#include "SceneManager.h"


int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);

	QSurfaceFormat format = QSurfaceFormat::defaultFormat();
	format.setVersion(3, 2);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setOption(QSurfaceFormat::DebugContext);
	format.setDepthBufferSize(24);
	format.setSamples(8);
	format.setStencilBufferSize(8);
	format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	QSurfaceFormat::setDefaultFormat(format);

	osgEarth::initialize();

	app.setWindowIcon(QIcon(":/qt/qml/Waisting/icon/app.png"));//qrc��qml�ļ���д ��c++�в�Ҫqrc
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	qmlRegisterType<WTOSGViewer>("WTOSG", 1, 0, "WTOSGViewer");
	QQmlApplicationEngine engine;
	auto* wLayerTree=engine.singletonInstance<WT::WLayerTreeModel*>("WTOSG", "WLayerTreeModel");
	auto& te = SceneManager::getInstance();
	QObject::connect(&te, &SceneManager::nodeLoaded, wLayerTree, &WT::WLayerTreeModel::addNode);
	QObject::connect(wLayerTree, &WT::WLayerTreeModel::checkStateChangedTreeNode, &te, &SceneManager::switchLayerVisibilityByUIDSlot); 
	QObject::connect(wLayerTree, &WT::WLayerTreeModel::zoomToTreeNode, &te, &SceneManager::zoomToLayerByUIDSlot);

	QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
		&app, []() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection); 

	//QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
	//	&app, [&engine]() { 
	//		QList<QObject*>rootOjects = engine.rootObjects();
	//		QQuickItem* viewerObject = rootOjects.first()->findChild<QQuickItem*>("leftMenu");
	//		//绑定Viewer和Tree
	//		//QObject::connect(viewerObject, &WTOSGViewer::loadedFile, wLayerTree, &WT::WLayerTreeModel::addNode);
	//	},
	//	Qt::QueuedConnection); 
    engine.loadFromModule("Waisting", "Main");

	QObject* root = nullptr;

	

	return app.exec();
}
