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
	//��ȡ���ڵ�
	TreeItemNode* getParent() { return parent; }
	//��ȡ����
	size_t childCount() { return children.size(); }
	//��ȡ��N���ֽڵ�
	TreeItemNode* getNthChild(size_t N);
	//�����ӽڵ�
	//��Ϊ��������ָ���ԭ�� ��Ҫ�ٵ���setParent
	void addChild(TreeItemNode* oneChild); 
	//���ø��ڵ�
	void setParent(TreeItemNode* parent);

	//��ѡ���ڵ�
	void setChecked();
	//ȡ����ѡ���ڵ�
	void setUnChecked();

	//�����۵�
	void setFolded() { isFold = true; }
	//����չ��
	void setUnFolded() { isFold = false; }
	bool getFoldState() { return isFold; }

	//��ȡ��ǰ�ڵ��״̬
	Qt::CheckState getCheckState();

	int nthOf(TreeItemNode* oneChild);

	//���㵱ǰ�ڵ���ӽڵ���ȫ��ѡ��ȫ�ǹ�ѡ���߰빴ѡ״̬
	Qt::CheckState checkAndUpdateCheckState();
public:
	//�ڵ�����
	std::string name;
private:
	//����ѡ�� Ϊ�˱�֤Ч�� ����ֻ�޸��Լ����ӽڵ��״̬ ��Ҫ��ר�ŵ��ø��ڵ���µĺ���
	void setCheckState(bool state);
	void setCheckState(Qt::CheckState state);

	//�������굱ǰ�ڵ�ı仯�� �������¸��ڵ��״̬
	void updateParenCheckState();



private:
	//�ýڵ��Ƿ񱻹�ѡ ע�� ��ѡ��Ӧ�ð������ӽڵ㹴ѡ��� �������ѡ�������ӽڵ㲻��ѡ ���Ϊ���ֹ�ѡ��ѡ�����ӽڵ�
	Qt::CheckState checkState = Qt::Unchecked;
	//�Ľڵ��Ƿ��۵� false��ʶչ�� true��ʶ�۵�
	bool isFold = false;//

	//�ýڵ���ӽڵ�
	std::vector<TreeItemNode*>children;
	//�ýڵ�ĸ��ڵ�
	TreeItemNode* parent;
};


class WLayerTreeModel:public QAbstractItemModel
{
	Q_OBJECT
	QML_ELEMENT
	QML_SINGLETON
	enum ItemRoles {
		NAME = Qt::UserRole + 1,//�ڵ�����
		CHECKSTATE = Qt::UserRole + 2,//�ڵ㹴ѡ״̬
		FOLDSTATE= Qt::UserRole + 3//�ڵ��۵�״̬
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
