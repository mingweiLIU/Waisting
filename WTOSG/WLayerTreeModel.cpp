#include "WLayerTreeModel.h"
#include <QFile>
#include "NanoID/nanoid.h"
WTNAMESPACESTART


WLayerTreeModel::WLayerTreeModel(QObject* parent/*=0*/)
{	
	root = new TreeItemNode("begin");
	TreeItemNode* child1 = new TreeItemNode("child1");
	TreeItemNode* child2 = new TreeItemNode("child2");
	TreeItemNode* child11 = new TreeItemNode("child11");
	TreeItemNode* child111 = new TreeItemNode("child111");
	TreeItemNode* child112 = new TreeItemNode("child112");
	TreeItemNode* child1121 = new TreeItemNode("child1121");
	TreeItemNode* child1122 = new TreeItemNode("child1122");
	TreeItemNode* child1123 = new TreeItemNode("child1123");
	TreeItemNode* child11211 = new TreeItemNode("child11211");
	TreeItemNode* child112111 = new TreeItemNode("child112111");
	TreeItemNode* child112112 = new TreeItemNode("child112112");
	TreeItemNode* child1121121 = new TreeItemNode("child1121121");
	TreeItemNode* child12 = new TreeItemNode("child12");
	TreeItemNode* child3 = new TreeItemNode("child3");
	TreeItemNode* child31 = new TreeItemNode("child31");
	TreeItemNode* child32 = new TreeItemNode("child32");

	root->addChild(child1);
	root->addChild(child2);
	root->addChild(child3);

	child1->addChild(child11);
	child1->addChild(child12);
	child11->addChild(child111);
	child11->addChild(child112);
	child112->addChild(child1121);
	child112->addChild(child1122);
	child112->addChild(child1123);
	child1121->addChild(child11211);
	child11211->addChild(child112111);
	child11211->addChild(child112112);
	child112112->addChild(child1121121);
	child3->addChild(child31);
	child3->addChild(child32);
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

	TreeItemNode* childItem = itemFromIndex(child);
	TreeItemNode* parentItem = childItem->getParent();

	if (parentItem == root)
	{
		return QModelIndex();
	}

	int row = 0;
	if (parentItem->getParent())
	{
		row = parentItem->getParent()->nthOf(parentItem);
	}
	return createIndex(row, 0, parentItem);
}

QVariant WLayerTreeModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
	if (!index.isValid()) return QVariant();

	TreeItemNode* item = itemFromIndex(index);
	switch (role)
	{
	case NAME: 
		return QString::fromLocal8Bit(item->name); 
	case CHECKSTATE:
		return item->getCheckState();
	case FOLDSTATE:
		return item->getFoldState();
	case INDEXSTATE:
		return item->getIndexState();
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
	 names[INDEXSTATE] = "indexState";
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

void WLayerTreeModel::checkItem(const QModelIndex& parent /*= QModelIndex()*/)
{
	TreeItemNode* item = static_cast<TreeItemNode*> (parent.internalPointer());
	item->getCheckState() == Qt::Checked ? item->setUnChecked() : item->setChecked();

	// �����¼�
	emit dataChanged(parent, parent);
	//����޸ĵıȽ϶� �����������޸�����
	//beginResetModel();
	//setData(.....)
	//endResetModel();
	//end����޸ĵıȽ϶� �����������޸�����
	
	//�ɼ��Ա仯
	emit  checkStateChangedTreeNode(item->UID, item->getCheckState() == Qt::Checked);
}

Q_INVOKABLE void WLayerTreeModel::zoomToItem(const QModelIndex& parent /*= QModelIndex()*/)
{
	TreeItemNode* item = static_cast<TreeItemNode*> (parent.internalPointer());
	emit zoomToTreeNode(item->UID);
}

void WLayerTreeModel::testAdd()
{

	TreeItemNode* child33 = new TreeItemNode("child33");
	beginResetModel();;
	//root->getNthChild(2)->getNthChild(1)->addChild(child33);
	root->addChild(child33);
	endResetModel();
}

bool WLayerTreeModel::addNode(std::string name, std::string uid, std::string parentUID /*= ""*/)
{
	//�ж������uidΪ�� ��ֱ����ӵ����ڵ���
	if (""==parentUID)
	{

		beginResetModel();;
		root->addChild(new TreeItemNode(name, uid));
		endResetModel();
		return true;
	}else {
		//�����ж�uid�Ĵ����� ���ǲ������ظ�������
		TreeItemNode* parentItem = root->getChildByUID(parentUID);
		if (parentItem)
		{
			beginResetModel();;
			parentItem->addChild(new TreeItemNode(name, uid));
			endResetModel();
			return true;
		}
		return false;
	}	
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
	//��Ҫ�ж��Ƿ��Ѿ�������
	auto findResultIt=std::find(children.begin(), children.end(), oneChild);
	if (findResultIt==children.end())
	{
		children.push_back(oneChild);
		oneChild->setParent(this);
	}
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

TreeItemNode::TreeItemNode(std::string name, std::string uid/*=""*/, TreeItemNode* parent /*= NULL*/)
{
	this->name = name;
	this->parent = parent;
	if ("" == uid)
	{
		uid = nanoid::NanoID::generate();
	}
	this->UID = uid;
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

int TreeItemNode::getIndexState()
{
	if (children.size() == 1) return 2;
	int index = parent->getIndex(this);
	if (0 == index)
		return 0;
	else if (parent->childCount() - 1 != index)
		return 1;
	else
		return 2;
}

int TreeItemNode::getIndex(TreeItemNode* checkChild)
{
	auto result=std::find(children.begin(), children.end(), checkChild);
	if (result == children.end()) return -1;

	return std::distance(children.begin(), result);
}

void TreeItemNode::clearChildren()
{
	for (TreeItemNode* item : children)
	{
		delete item;
	}
	std::vector<TreeItemNode*>().swap(children);
}

//���޸ĸ��ڵ�
TreeItemNode::~TreeItemNode()
{
	clearChildren();
}

TreeItemNode* TreeItemNode::getChildByUID(std::string childUID)
{
	for (TreeItemNode* oneChild:children)
	{
		if (oneChild->UID == childUID) return oneChild;
		else if (oneChild->childCount()>0)
		{
			return oneChild->getChildByUID(childUID);
		}
	}
	return nullptr;
}

WTNAMESPACEEND