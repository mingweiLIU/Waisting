#include "WLayerTreeModel.h"
#include <QFile>
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

	//TreeItem* oneBook = new TreeItem(QVariantList() << "第一本书" << "None");
	//oneBook->appendChild(new TreeItem(QVariantList()<<"asfjl"<<"akfka"));
	//oneBook->appendChild(new TreeItem(QVariantList() << "afka" << "None"));
	//oneBook->child(0)->appendChild(new TreeItem(QVariantList() << "第一章" << "saved"));
	//oneBook->child(0)->appendChild(new TreeItem(QVariantList() << "第二章" << "saved"));
	//oneBook->child(0)->appendChild(new TreeItem(QVariantList() << "第三章" << "saving"));
	//oneBook->appendChild(new TreeItem(QVariantList() << "kkk" << "None"));
	//oneBook->child(1)->appendChild(new TreeItem(QVariantList() << "第一章" << "saved"));
	//oneBook->child(1)->appendChild(new TreeItem(QVariantList() << "第三章" << "no save"));

	//TreeItem* twoBook = new TreeItem(QVariantList() << "第二本书" << "None");
	//twoBook->appendChild(new TreeItem(QVariantList() << "kajfka" << "None"));
	//twoBook->child(0)->appendChild(new TreeItem(QVariantList() << "第一章" << "saved"));
	//twoBook->child(0)->appendChild(new TreeItem(QVariantList() << "第二章" << "saving"));
	//twoBook->appendChild(new TreeItem(QVariantList() << "jakfjd" << "None"));
	//twoBook->child(1)->appendChild(new TreeItem(QVariantList() << "第一章" << "saved"));

	//root->appendChild(oneBook);
	//root->appendChild(twoBook);
	QFile file("D:\\Code\\QML\\TestTree\\default.txt");
	file.open(QIODevice::ReadOnly);
	const QString data = file.readAll();
	file.close();
	root = new TreeItem({ tr("Title"), tr("Summary") });
	setupModelData(data.split('\n'), root);
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

	TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
	switch (role)
	{
	case NAME: 
		return item->data(0);
	case STATE:
		return item->data(1);
	default:
		return item->data(index.column());
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
	return 1;
	//if (parent.isValid()) 
	//	return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
	//else
	//	return root->columnCount();
}

QHash<int, QByteArray> WLayerTreeModel::roleNames() const
{
	 QHash<int, QByteArray> names(QAbstractItemModel::roleNames());
	 names[NAME] = "name";
	 names[STATE] = "state";
	 return names;
}
void WLayerTreeModel::setupModelData(const QStringList& lines, TreeItem* parent)
{
	QList<TreeItem*> parents;
	QList<int> indentations;
	parents << parent;
	indentations << 0;

	int number = 0;

	while (number < lines.count()) {
		int position = 0;
		while (position < lines[number].length()) {
			if (lines[number].at(position) != ' ')
				break;
			position++;
		}

		const QString lineData = lines[number].mid(position).trimmed();

		if (!lineData.isEmpty()) {
			// Read the column data from the rest of the line.
			const QStringList columnStrings =
				lineData.split(QLatin1Char('\t'), Qt::SkipEmptyParts);
			QList<QVariant> columnData;
			columnData.reserve(columnStrings.count());
			for (const QString& columnString : columnStrings)
				columnData << columnString;

			if (position > indentations.last()) {
				// The last child of the current parent is now the new parent
				// unless the current parent has no children.

				if (parents.last()->childCount() > 0) {
					parents << parents.last()->child(parents.last()->childCount() - 1);
					indentations << position;
				}
			}
			else {
				while (position < indentations.last() && parents.count() > 0) {
					parents.pop_back();
					indentations.pop_back();
				}
			}

			// Append a new item to the current parent's list of children.
			parents.last()->appendChild(new TreeItem(columnData, parents.last()));
		}
		++number;
	}
}
WTNAMESPACEEND