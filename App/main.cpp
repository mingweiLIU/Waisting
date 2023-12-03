#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQuick/QQuickView>
#include "quickosgviewer.h"
#include "squircle.h"
#include "WTOSGViewer.h"

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	qmlRegisterType<QuickOSGViewer>("Test", 1, 0, "QuickOSGViewer");
	qmlRegisterType<WTOSGViewer>("Test", 1, 0, "WTOSGViewer");

	QQmlApplicationEngine engine;
	QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
		&app, []() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection);
	engine.loadFromModule("Waisting", "Main");

	return app.exec();
}
