#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQuick/QQuickView>
#include <QIcon>
#include "WTOSGViewer.h"

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/qt/qml/Waisting/icon/app.png"));//qrc��qml�ļ���д ��c++�в�Ҫqrc
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	qmlRegisterType<WTOSGViewer>("Test", 1, 0, "WTOSGViewer");

	QQmlApplicationEngine engine;
	QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
		&app, []() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection);
	engine.loadFromModule("Waisting", "Main");

	return app.exec();
}
