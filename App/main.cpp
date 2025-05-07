#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuick/QQuickView>
#include <QIcon>
#include<qsurfaceformat.h>
int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);

	// QSurfaceFormat format = QSurfaceFormat::defaultFormat();
	// format.setVersion(3, 2);
	// format.setProfile(QSurfaceFormat::CoreProfile);
	// format.setRenderableType(QSurfaceFormat::OpenGL);
	// format.setOption(QSurfaceFormat::DebugContext);
	// format.setDepthBufferSize(24);
	// format.setSamples(8);
	// format.setStencilBufferSize(8);
	// format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	// QSurfaceFormat::setDefaultFormat(format);

	// osgEarth::initialize();

	app.setWindowIcon(QIcon(":/qt/qml/Waisting/icon/app.png"));//
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	QQmlApplicationEngine engine;

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

	//QObject* root = nullptr;

	

	return app.exec();
}
