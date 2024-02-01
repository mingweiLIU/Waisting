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

	//TreeItem* oneBook = new TreeItem(QVariantList() << "��һ����" << "None");
	//oneBook->appendChild(new TreeItem(QVariantList()<<"asfjl"<<"akfka"));
	//oneBook->appendChild(new TreeItem(QVariantList() << "afka" << "None"));
	//oneBook->child(0)->appendChild(new TreeItem(QVariantList() << "��һ��" << "saved"));
	//oneBook->child(0)->appendChild(new TreeItem(QVariantList() << "�ڶ���" << "saved"));
	//oneBook->child(0)->appendChild(new TreeItem(QVariantList() << "������" << "saving"));
	//oneBook->appendChild(new TreeItem(QVariantList() << "kkk" << "None"));
	//oneBook->child(1)->appendChild(new TreeItem(QVariantList() << "��һ��" << "saved"));
	//oneBook->child(1)->appendChild(new TreeItem(QVariantList() << "������" << "no save"));

	//TreeItem* twoBook = new TreeItem(QVariantList() << "�ڶ�����" << "None");
	//twoBook->appendChild(new TreeItem(QVariantList() << "kajfka" << "None"));
	//twoBook->child(0)->appendChild(new TreeItem(QVariantList() << "��һ��" << "saved"));
	//twoBook->child(0)->appendChild(new TreeItem(QVariantList() << "�ڶ���" << "saving"));
	//twoBook->appendChild(new TreeItem(QVariantList() << "jakfjd" << "None"));
	//twoBook->child(1)->appendChild(new TreeItem(QVariantList() << "��һ��" << "saved"));

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
	child112111->setChecked();
}

WLayerTreeModel::~WLayerTreeModel()
{
	//root->deleteAllChild();
	//root = NULL;
}

// ��parent�ڵ��£���row�У���column��λ���ϴ�������
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
	case CHECKSTATE:
		return item->getCheckState();
	case FOLDSTATE:
		return item->getFoldState();
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
	////if (parent.isValid()) 
	////	return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
	////else
	////	return root->columnCount();
}

QHash<int, QByteArray> WLayerTreeModel::roleNames() const
{
	 QHash<int, QByteArray> names(QAbstractItemModel::roleNames());
	 names[NAME] = "name";
	 names[CHECKSTATE] = "checkState";
	 names[FOLDSTATE] = "foldState";
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
	//�ȼ�����״̬�͵�ǰ��״̬�����
	if ((state && checkState == Qt::Checked) || (!state&&checkState ==Qt::Unchecked)) return;
	//�������е��ӽڵ���Ҫ�޸�
	for (auto& item : this->children)
	{
		item->setCheckState(state);
	}

	//���޸ĸ��ڵ�״̬ǰ ���޸��Լ� �Է��㸸�ڵ�����ӽڵ�
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
		//������ǰ���
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

//��Ϊ��������ָ���ԭ�� ��Ҫ�ٵ���setParent
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

	//�����ǰ�ڵ���partial�� ��ô���ĸ��ڵ�һ����partrial��
	if (Qt::PartiallyChecked == checkState
		||(parent->getCheckState()==Qt::Checked&& checkState ==Qt::Unchecked)
		||(parent->getCheckState()==Qt::Unchecked && checkState ==Qt::Checked)) {
		parent->setCheckState(Qt::PartiallyChecked);
		parent->updateParenCheckState();
		return;
	}

	//��������ܳ��ָ��ڵ���ӽڵ�ͬʱ���Ƿ�partial�����
	//���� //��Ϊ���Ǵ����ϳ����� �϶�������һ���ӽڵ�
	Qt::CheckState result = parent->children[0]->getCheckState();
	//������ǰ���
	for (size_t i = 1, i_up = parent->children.size(); i < i_up; ++i)
	{
		TreeItemNode* tempItem = parent->children[i];
		if (result != tempItem->getCheckState())
		{
			parent->setCheckState(Qt::PartiallyChecked);
			//���ʱ��͸ý��и��ĸ��ڵ��� ��������
			parent->updateParenCheckState();
			return;
		}
	}
	parent->setCheckState(result);
	//���ʱ��͸ý��и��ĸ��ڵ��� ��������
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