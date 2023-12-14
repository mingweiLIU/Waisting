#ifndef WLAYERMODEL_H
#define WLAYERMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>

//这是一个同步于osg中模型对象的数据模型，用于处理图层控制树的同步

class WLayerModel: public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit WLayerModel(QObject* parent = nullptr);

	Q_INVOKABLE virtual QModelIndex index(int row, int column,
		const QModelIndex& parent = QModelIndex()) const override;

	Q_INVOKABLE virtual QModelIndex parent(const QModelIndex& child) const override;

	Q_INVOKABLE virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;

	Q_INVOKABLE virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	Q_INVOKABLE virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
};

#endif // WLAYERMODEL_H
