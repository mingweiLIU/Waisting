#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQuick/QQuickView>
#include <QIcon>
#include "WTOSGViewer.h"

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/qt/qml/Waisting/icon/app.png"));//qrc在qml文件中写 在c++中不要qrc
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	qmlRegisterType<WTOSGViewer>("Test", 1, 0, "WTOSGViewer");

	QQmlApplicationEngine engine;
	QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
		&app, []() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection);
	engine.loadFromModule("Waisting", "Main");

	return app.exec();
}
