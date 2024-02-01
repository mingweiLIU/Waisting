#ifndef QIMAGE2QMLPROVIDER_H
#define QIMAGE2QMLPROVIDER_H

#include <QQuickImageProvider>

class QImage2QMLProvider :public QQuickImageProvider
{    
public:
    QImage2QMLProvider();

    void setQImage(QImage& image, QString id);

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    //QMap<QString,QImage> 

};

#endif // QIMAGE2QMLPROVIDER_H
