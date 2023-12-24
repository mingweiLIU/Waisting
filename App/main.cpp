#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuick/QQuickView>
#include <QIcon>
#include "WTOSGViewer.h"
#include "WLayerTreeModel.h"

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/qt/qml/Waisting/icon/app.png"));//qrc��qml�ļ���д ��c++�в�Ҫqrc
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	qmlRegisterType<WTOSGViewer>("Test", 1, 0, "WTOSGViewer");
	QQmlApplicationEngine engine;
	auto* wLayerTree=engine.singletonInstance<WT::WLayerTreeModel*>("WTOSG", "WLayerTreeModel");

	QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
		&app, []() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection);
    engine.loadFromModule("Waisting", "Main");

	QObject* root = nullptr;
	QList<QObject*>rootOjects = engine.rootObjects();

	return app.exec();
}
