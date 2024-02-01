#ifndef WLAYERTREEMODEL_H
#define WLAYERTREEMODEL_H
#include <QAbstractItemModel>
#include <QVariantList>
#include <QQmlEngine>
#include <osg/Node>
#include "WTDefines.h"
#pragma execution_character_set("utf-8")
WTNAMESPACESTART
//class TreeItem
//{
//public:
//	explicit TreeItem(const QList<QVariant>&data,TreeItem* parent=NULL);
//	~TreeItem();
//	void appendChild(TreeItem* child);
//	void deleteAllChild();
//
//	TreeItem* child(int row);
//	int childCount() const;
//	int columnCount()const;
//	QVariant data(int column)const;
//	void setParent(TreeItem* parent);
//	TreeItem* panret();
//	int row()const;
//
//public:
//	Qt::CheckState getCheckState() {
//		return checkState;
//	}
//
//private:
//	TreeItem* m_parent;
//	QList<TreeItem*> m_children;
//	QList<QVariant> m_itemData;
//	Qt::CheckState checkState = Qt::Unchecked;
//};



class TreeItemNode
{
public:
	explicit TreeItemNode(std::string name, TreeItemNode* parent = NULL);
	//获取父节点
	TreeItemNode* getParent() { return parent; }
	//获取子数
	size_t childCount() { return children.size(); }
	//获取第N个字节点
	TreeItemNode* getNthChild(size_t N);
	//添加子节点
	//因为是弱智能指针的原因 需要再调用setParent
	void addChild(TreeItemNode* oneChild); 
	//设置父节点
	void setParent(TreeItemNode* parent);

	//勾选本节点
	void setChecked();
	//取消勾选本节点
	void setUnChecked();

	//设置折叠
	void setFolded() { isFold = true; }
	//设置展开
	void setUnFolded() { isFold = false; }
	bool getFoldState() { return isFold; }

	//获取当前节点的状态
	Qt::CheckState getCheckState();

	int nthOf(TreeItemNode* oneChild);

	//计算当前节点的子节点是全勾选、全非勾选或者半勾选状态
	Qt::CheckState checkAndUpdateCheckState();
public:
	//节点名字
	std::string name;
private:
	//设置选中 为了保证效率 这里只修改自己和子节点的状态 需要再专门调用父节点更新的函数
	void setCheckState(bool state);
	void setCheckState(Qt::CheckState state);

	//当设置完当前节点的变化后 迭代更新父节点的状态
	void updateParenCheckState();



private:
	//该节点是否被勾选 注意 勾选后应该把所有子节点勾选完成 如果不勾选则所有子节点不勾选 如果为部分勾选则勾选部分子节点
	Qt::CheckState checkState = Qt::Unchecked;
	//改节点是否折叠 false标识展开 true标识折叠
	bool isFold = false;//

	//该节点的子节点
	std::vector<TreeItemNode*>children;
	//该节点的父节点
	TreeItemNode* parent;
};


class WLayerTreeModel:public QAbstractItemModel
{
	Q_OBJECT
	QML_ELEMENT
	QML_SINGLETON
	enum ItemRoles {
		NAME = Qt::UserRole + 1,//节点名字
		CHECKSTATE = Qt::UserRole + 2,//节点勾选状态
		FOLDSTATE= Qt::UserRole + 3//节点折叠状态
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

public slots:
	Q_INVOKABLE Qt::CheckState itemChecked(int row, int column, const QModelIndex& parent = QModelIndex())const;

private:
	TreeItemNode* itemFromIndex(const QModelIndex& index) const;
	TreeItemNode* root=nullptr;
};
WTNAMESPACEEND
#endif // WLAYERTREEMODEL_H
