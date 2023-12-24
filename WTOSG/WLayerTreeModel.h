#ifndef WLAYERTREEMODEL_H
#define WLAYERTREEMODEL_H
#include <QAbstractItemModel>
#include <QVariantList>
#include <QQmlEngine>
#include "WTDefines.h"
#pragma execution_character_set("utf-8")
WTNAMESPACESTART
class TreeItem
{
public:
	explicit TreeItem(const QList<QVariant>&data,TreeItem* parent=NULL);
	~TreeItem();
	void appendChild(TreeItem* child);
	void deleteAllChild();

	TreeItem* child(int row);
	int childCount() const;
	int columnCount()const;
	QVariant data(int column)const;
	void setParent(TreeItem* parent);
	TreeItem* panret();
	int row()const;

private:
	TreeItem* m_parent;
	QList<TreeItem*> m_children;
	QList<QVariant> m_itemData;
};

class WLayerTreeModel:public QAbstractItemModel
{
	Q_OBJECT
	QML_ELEMENT
	QML_SINGLETON
	enum ItemRoles {
		NAME = Qt::UserRole + 1,
		STATE
	};
public:
    explicit WLayerTreeModel(QObject* parent=0);
	~WLayerTreeModel();

	Q_INVOKABLE virtual QModelIndex index(int row, int column,const QModelIndex& parent = QModelIndex()) const override;
	Q_INVOKABLE virtual QModelIndex parent(const QModelIndex& child) const override;
    Q_INVOKABLE virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	Q_INVOKABLE virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	Q_INVOKABLE virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	Q_INVOKABLE QHash<int, QByteArray> roleNames()const override;

private:
	TreeItem* itemFromIndex(const QModelIndex& index) const;
	TreeItem* root;
	QStringList m_headers;
	void setupModelData(const QStringList& lines, TreeItem* parent);
};
WTNAMESPACEEND
#endif // WLAYERTREEMODEL_H
