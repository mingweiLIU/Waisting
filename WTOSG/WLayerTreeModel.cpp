#include "WLayerTreeModel.h"

WTNAMESPACESTART

TreeItem::TreeItem(const QList<QVariant>& data, TreeItem* parent/*=NULL*/)
{
	this->m_parent = parent;
	this->m_itemData = data;
}

TreeItem::~TreeItem()
{
	this->m_itemData.clear();
	this->m_parent = NULL;
}

void TreeItem::appendChild(TreeItem* child)
{
	child->setParent(this);
	this->m_children.append(child);
}

void TreeItem::deleteAllChild()
{
	for (int index=0;index<m_children.size();++index)
	{
		m_children[index]->deleteAllChild();
	}
	qDeleteAll(m_children);
	m_children.clear();
}

TreeItem* TreeItem::child(int row)
{
	return this->m_children[row];
}

int TreeItem::childCount() const
{
	return m_children.size();
}

int TreeItem::columnCount() const
{
	return m_itemData.size();
}

QVariant TreeItem::data(int column) const
{
	return m_itemData[column];
}

void TreeItem::setParent(TreeItem* parent)
{
	m_parent = parent;
}

TreeItem* TreeItem::panret()
{
	return m_parent;
}

int TreeItem::row() const
{
	if (!m_parent) return 0;
	return m_parent->m_children.indexOf(const_cast<TreeItem*>(this));
}

WLayerTreeModel::WLayerTreeModel(QObject* parent/*=0*/)
{
	this->root = new TreeItem(QVariantList());

	TreeItem* oneBook = new TreeItem(QVariantList() << "第一本书" << "None");
	oneBook->appendChild(new TreeItem(QVariantList()<<"asfjl"<<"akfka"));
	oneBook->appendChild(new TreeItem(QVariantList() << "afka" << "None"));
	oneBook->child(0)->appendChild(new TreeItem(QVariantList() << "第一章" << "saved"));
	oneBook->child(0)->appendChild(new TreeItem(QVariantList() << "第二章" << "saved"));
	oneBook->child(0)->appendChild(new TreeItem(QVariantList() << "第三章" << "saving"));
	oneBook->appendChild(new TreeItem(QVariantList() << "kkk" << "None"));
	oneBook->child(1)->appendChild(new TreeItem(QVariantList() << "第一章" << "saved"));
	oneBook->child(1)->appendChild(new TreeItem(QVariantList() << "第三章" << "no save"));

	TreeItem* twoBook = new TreeItem(QVariantList() << "第二本书" << "None");
	twoBook->appendChild(new TreeItem(QVariantList() << "kajfka" << "None"));
	twoBook->child(0)->appendChild(new TreeItem(QVariantList() << "第一章" << "saved"));
	twoBook->child(0)->appendChild(new TreeItem(QVariantList() << "第二章" << "saving"));
	twoBook->appendChild(new TreeItem(QVariantList() << "jakfjd" << "None"));
	twoBook->child(1)->appendChild(new TreeItem(QVariantList() << "第一章" << "saved"));

	root->appendChild(oneBook);
	root->appendChild(twoBook);
}

WLayerTreeModel::~WLayerTreeModel()
{
	root->deleteAllChild();
	root = NULL;
}

// 在parent节点下，第row行，第column列位置上创建索引
QModelIndex WLayerTreeModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
	if (!hasIndex(row,column,parent))
	{
		return QModelIndex();
	}

	TreeItem* treeItem = itemFromIndex(parent);
	TreeItem* child = treeItem->child(row);
	if (child)
	{
		return createIndex(row, column, child);
	}
	else
	{
		return QModelIndex();
	}
}

TreeItem* WLayerTreeModel::itemFromIndex(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		TreeItem* item = static_cast<TreeItem*> (parent.internalPointer());
		return item;
	}
	return root;
}

QModelIndex WLayerTreeModel::parent(const QModelIndex& child) const
{
	if (!child.isValid())
	{
		return QModelIndex();
	}

	TreeItem* childItem = static_cast<TreeItem*>(child.internalPointer());
	TreeItem* parentItem = childItem->panret();
	if (parentItem==root)
	{
		return QModelIndex();
	}
	return createIndex(parentItem->row(), 0, parentItem);
}

QVariant WLayerTreeModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
	if (!index.isValid()) return QVariant();

	TreeItem* item = itemFromIndex(index);
	switch (role)
	{
	case NAME:
		return item->data(0);
	case STATE:
		return item->data(1);
	default:
		break;
	}
}

int WLayerTreeModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	if (parent.column() > 0) return 0;
	TreeItem* item = itemFromIndex(parent);
	return item->childCount();
}

int WLayerTreeModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	if (parent.isValid()) 
		return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
	else
		return root->columnCount();
}

QHash<int, QByteArray> WLayerTreeModel::roleNames() const
{
	 QHash<int, QByteArray> names(QAbstractItemModel::roleNames());
	 names[NAME] = "name";
	 names[STATE] = "state";
	 return names;
}

WTNAMESPACEEND