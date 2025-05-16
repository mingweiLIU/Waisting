#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuick/QQuickView>
#include <QIcon>
#include<qsurfaceformat.h>
#include <QtQuickControls2/QQuickStyle>  // 引入 Style 模块

int main(int argc, char *argv[])
{
	// 方法1：禁用DPI缩放（强制1:1像素）
	QGuiApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
	QGuiApplication app(argc, argv);
	// 设置全局 Style（推荐 Fusion 或 Material）
	QQuickStyle::setStyle("Fusion");  // 或 "Material", "Basic"
	app.setWindowIcon(QIcon(":/qt/qml/Waisting/icon/app.png"));//
	QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

	QQmlApplicationEngine engine;

	QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
		&app, []() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection); 
    engine.loadFromModule("Waisting", "Main");

	return app.exec();
}
