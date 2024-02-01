#include "WLayerTreeModel.h"
#include <QFile>
WTNAMESPACESTART

//TreeItem::TreeItem(const QList<QVariant>& data, TreeItem* parent/*=NULL*/)
//{
//	this->m_parent = parent;
//	this->m_itemData = data;
//}
//
//TreeItem::~TreeItem()
//{
//	this->m_itemData.clear();
//	this->m_parent = NULL;
//}
//
//void TreeItem::appendChild(TreeItem* child)
//{
//	child->setParent(this);
//	this->m_children.append(child);
//}
//
//void TreeItem::deleteAllChild()
//{
//	for (int index=0;index<m_children.size();++index)
//	{
//		m_children[index]->deleteAllChild();
//	}
//	qDeleteAll(m_children);
//	m_children.clear();
//}
//
//TreeItem* TreeItem::child(int row)
//{
//	return this->m_children[row];
//}
//
//int TreeItem::childCount() const
//{
//	return m_children.size();
//}
//
//int TreeItem::columnCount() const
//{
//	return m_itemData.size();
//}
//
//QVariant TreeItem::data(int column) const
//{
//	return m_itemData[column];
//}
//
//void TreeItem::setParent(TreeItem* parent)
//{
//	m_parent = parent;
//}
//
//TreeItem* TreeItem::panret()
//{
//	return m_parent;
//}
//
//int TreeItem::row() const
//{
//	if (!m_parent) return 0;
//	return m_parent->m_children.indexOf(const_cast<TreeItem*>(this));
//}

WLayerTreeModel::WLayerTreeModel(QObject* parent/*=0*/)
{
	//this->root = new TreeItem(QVariantList());

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
	//QFile file("D:\\Code\\QML\\TestTree\\default.txt");
	//file.open(QIODevice::ReadOnly);
	//const QString data = file.readAll();
	//file.close();
	//root = new TreeItem({ tr("Title"), tr("Summary") });
	//setupModelData(data.split('\n'), root);
	root = new TreeItemNode("begin");
	TreeItemNode* child1 = new TreeItemNode("child1");
	TreeItemNode* child2 = new TreeItemNode("child2");
	TreeItemNode* child11 = new TreeItemNode("child11");
	TreeItemNode* child111 = new TreeItemNode("child111");
	TreeItemNode* child112 = new TreeItemNode("child112");
	TreeItemNode* child1121 = new TreeItemNode("child1121");
	TreeItemNode* child11211 = new TreeItemNode("child11211");
	TreeItemNode* child112111 = new TreeItemNode("child112111");
	TreeItemNode* child112112 = new TreeItemNode("child112112");
	TreeItemNode* child1121121 = new TreeItemNode("child1121121");
	TreeItemNode* child12 = new TreeItemNode("child12");
	TreeItemNode* child3 = new TreeItemNode("child3");
	TreeItemNode* child31 = new TreeItemNode("child31");

	root->addChild(child1);
	root->addChild(child2);
	root->addChild(child3);

	child1->addChild(child11);
	child1->addChild(child12);
	child11->addChild(child111);
	child11->addChild(child112);
	child112->addChild(child1121);
	child1121->addChild(child11211);
	child11211->addChild(child112111);
	child11211->addChild(child112112);
	child112112->addChild(child1121121);
	child3->addChild(child31);
	child12->setChecked();
}

WLayerTreeModel::~WLayerTreeModel()
{
	//root->deleteAllChild();
	//root = NULL;
}

// 在parent节点下，第row行，第column列位置上创建索引
QModelIndex WLayerTreeModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
	if (!hasIndex(row,column,parent))
	{
		return QModelIndex();
	}

	TreeItemNode* treeItem = itemFromIndex(parent);
	TreeItemNode* child = treeItem->getNthChild(row);
	if (child)
	{
		return createIndex(row, column, child);
	}
	else
	{
		return QModelIndex();
	}
}

TreeItemNode* WLayerTreeModel::itemFromIndex(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		TreeItemNode* item = static_cast<TreeItemNode*> (parent.internalPointer());
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

	TreeItemNode* childItem = static_cast<TreeItemNode*>(child.internalPointer());
	TreeItemNode* parentItem = childItem->getParent();

	if (parentItem == root)
	{
		return QModelIndex();
	}
	return createIndex(parentItem->nthOf(childItem), 0, parentItem);	
}

QVariant WLayerTreeModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
	if (!index.isValid()) return QVariant();

	TreeItemNode* item = static_cast<TreeItemNode*>(index.internalPointer());
	switch (role)
	{
	case NAME: 
		return QString::fromLocal8Bit(item->name);
	case STATE:
		return item->getCheckState();
	default:
		return QString::fromLocal8Bit(item->name);
	}
}

int WLayerTreeModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	if (parent.column() > 0) return 0;
	TreeItemNode* item = itemFromIndex(parent);
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
	 names[STATE] = "checked";
	 //names[STATE] = "state";
	 return names;
}

Qt::CheckState WLayerTreeModel::itemChecked(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return Qt::Unchecked;
	}

	TreeItemNode* treeItem = itemFromIndex(parent);
	return treeItem->getCheckState();
}


void TreeItemNode::setCheckState(bool state)
{
	//先检查这个状态和当前的状态的情况
	if ((state && checkState == Qt::Checked) || (!state&&checkState ==Qt::Unchecked)) return;
	//首先所有的子节点需要修改
	for (auto& item : this->children)
	{
		item->setCheckState(state);
	}

	//在修改父节点状态前 先修改自己 以方便父节点遍历子节点
	checkState = state ? Qt::Checked : Qt::Unchecked;
}

void TreeItemNode::setCheckState(Qt::CheckState state)
{
	checkState = state;
}

Qt::CheckState TreeItemNode::checkAndUpdateCheckState()
{
	if (children.size()>0)
	{
		Qt::CheckState result= children[0]->getCheckState();
		//遍历当前情况
		for (size_t i = 1, i_up = children.size(); i < i_up; ++i)
		{
			TreeItemNode* tempItem = children[i];
			if (result != tempItem->getCheckState())
			{
				checkState = Qt::PartiallyChecked;
				return checkState;
			}
		}
		checkState = result;
		return checkState;		
	}	
}

Qt::CheckState TreeItemNode::getCheckState()
{
	return checkState;
}

void TreeItemNode::setParent(TreeItemNode* parent)
{
	this->parent=parent;
}

//因为是弱智能指针的原因 需要再调用setParent
void TreeItemNode::addChild(TreeItemNode* oneChild)
{
	children.push_back(oneChild);
	oneChild->setParent(this);
}

TreeItemNode* TreeItemNode::getNthChild(size_t N)
{
	if (N > children.size()) return NULL;
	return children[N];	
}

int TreeItemNode::nthOf(TreeItemNode* oneChild)
{
	auto it=std::find(children.begin(), children.end(), oneChild);
	if (it != children.end()) {
		return std::distance(children.begin(), it);
	}
	return -1;
}

TreeItemNode::TreeItemNode(std::string name, TreeItemNode* parent /*= NULL*/)
{
	this->name = name;
	this->parent = parent;
}

void TreeItemNode::updateParenCheckState()
{
	if (!parent) return;

	//如果当前节点是partial的 那么它的父节点一定是partrial的
	if (Qt::PartiallyChecked == checkState
		||(parent->getCheckState()==Qt::Checked&& checkState ==Qt::Unchecked)
		||(parent->getCheckState()==Qt::Unchecked && checkState ==Qt::Checked)) {
		parent->setCheckState(Qt::PartiallyChecked);
		parent->updateParenCheckState();
		return;
	}

	//这个不可能出现父节点和子节点同时不是非partial的情况
	//更新 //因为它是从子上出来的 肯定有至少一个子节点
	Qt::CheckState result = parent->children[0]->getCheckState();
	//遍历当前情况
	for (size_t i = 1, i_up = parent->children.size(); i < i_up; ++i)
	{
		TreeItemNode* tempItem = parent->children[i];
		if (result != tempItem->getCheckState())
		{
			parent->setCheckState(Qt::PartiallyChecked);
			//这个时候就该进行父的父节点了 迭代处理
			parent->updateParenCheckState();
			return;
		}
	}
	parent->setCheckState(result);
	//这个时候就该进行父的父节点了 迭代处理
	parent->updateParenCheckState();
}

void TreeItemNode::setChecked()
{
	setCheckState(true);
	updateParenCheckState();
}

void TreeItemNode::setUnChecked()
{
	setCheckState(false);
	updateParenCheckState();
}

WTNAMESPACEEND