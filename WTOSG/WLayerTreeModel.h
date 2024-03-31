#ifndef WLAYERTREEMODEL_H
#define WLAYERTREEMODEL_H
#include <QAbstractItemModel>
#include <QVariantList>
#include <QQmlEngine>
#include <functional>
#include <osgEarth/Common>
#include "WTFrame/WTDefines.h"
#pragma execution_character_set("utf-8")
WTNAMESPACESTART

using TreeNodeEventList = std::vector<std::function<void(int parentUID, int currentUID,bool state)>>;
class TreeItemNode
{
public:
	explicit TreeItemNode(std::string name, int uid=osgEarth::createUID(), TreeItemNode* parent = nullptr, bool visible = true);
	//��� �����޸ĸ��ڵ�
	~TreeItemNode();

	//��ȡ���ڵ�
	TreeItemNode* getParent() { return parent; }

	//��ȡ�ӽڵ�
	std::vector<TreeItemNode*> getChildren() { return children; }

	//��ȡ�ӽڵ������
	int getIndex(TreeItemNode* checkChild);

	//��ȡ����
	size_t childCount() { return children.size(); }

	//��ȡ��N���ֽڵ�
	TreeItemNode* getNthChild(size_t N);

	//ͨ��UID����ȡ�ӽڵ�
	TreeItemNode* getChildByUID(int childUID);

	//����ӽڵ�
	void addChild(TreeItemNode* oneChild); 

	//����ӽڵ�
	void clearChildren();

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

	//��ȡ��ǰ�ڵ������ֵܽڵ�Ĺ�ϵ 0��ʾ��һ�� 1��ʾ�м� 2��ʾ���һ�� ���ֻ��һ�� �������һ��2
	int getIndexState();

	//��ȡ��ǰ�ڵ��״̬
	Qt::CheckState getCheckState();

	int nthOf(TreeItemNode* oneChild);

	//���㵱ǰ�ڵ���ӽڵ���ȫ��ѡ��ȫ�ǹ�ѡ���߰빴ѡ״̬
	Qt::CheckState checkAndUpdateCheckState();

	//���״̬�޸��¼�
	void addCheckeStateChangedEvents(std::function<void(int parentUID, int currentUID, bool state)> event);
	//��ӵ���¼�
	void addClickEvents(std::function<void(int parentUID, int currentUID, bool state)> event);

public:
	//�ڵ�����
	std::string name;
	int UID;//ԭ���ƻ���ʹ��nannoid ����Ϊ�˱��ֺ�osgearth��һ�� ʹ��ȫ�ֵ�int��ͳһ ע���κιҵ����ϵĶ���Ӧ�õ���osgearth::createUID�������Լ���Ψһid
private:
	//����ѡ�� Ϊ�˱�֤Ч�� ����ֻ�޸��Լ����ӽڵ��״̬ ��Ҫ��ר�ŵ��ø��ڵ���µĺ���
	void setCheckState(bool state);
	void setCheckState(Qt::CheckState state);

	//�������굱ǰ�ڵ�ı仯�� �������¸��ڵ��״̬
	void updateParenCheckState();

	//ִ�����еĴ����¼�
	void doOnCheckStateChanged(int parentUID, int currentUID,bool state);
	//ִ�����еĵ���¼�
	void doOnClicked(int parentUID, int currentUID);
private:
	//�ýڵ��Ƿ񱻹�ѡ ע�� ��ѡ��Ӧ�ð������ӽڵ㹴ѡ��� �������ѡ�������ӽڵ㲻��ѡ ���Ϊ���ֹ�ѡ��ѡ�����ӽڵ�
	Qt::CheckState checkState = Qt::Unchecked;
	//�Ľڵ��Ƿ��۵� false��ʶչ�� true��ʶ�۵�
	bool isFold = false;//

	//�ýڵ���ӽڵ�
	std::vector<TreeItemNode*>children;
	//�ýڵ�ĸ��ڵ�
	TreeItemNode* parent;

	//��ѡ״̬�޸��¼�
	TreeNodeEventList onCheckStateChanged;
	//����¼�
	TreeNodeEventList onClicked;
};


class WLayerTreeModel:public QAbstractItemModel
{
	Q_OBJECT
	QML_ELEMENT
	QML_SINGLETON
	enum ItemRoles {
		NAME = Qt::UserRole + 1,//�ڵ�����
		CHECKSTATE = Qt::UserRole + 2,//�ڵ㹴ѡ״̬
		FOLDSTATE= Qt::UserRole + 3,//�ڵ��۵�״̬
		INDEXSTATE = Qt::UserRole + 4//��һ������0 �м�����1 ���һ���ڵ�����2
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

	Q_INVOKABLE Qt::CheckState itemChecked(int row, int column, const QModelIndex& parent = QModelIndex())const;
	Q_INVOKABLE void checkItem(const QModelIndex& parent = QModelIndex());
	Q_INVOKABLE void test() {
		emit testMessage();
	}
	Q_INVOKABLE void zoomToItem(const QModelIndex& parent = QModelIndex());
	Q_INVOKABLE void testAdd();

public slots:
	bool slot_addNode(std::string name, int uid, std::optional<int> parentUID,bool visible=true);

signals:
	//չ����Ϣ
	void expandTreeNode(QModelIndex index);
	//�۵���Ϣ
	void foldTreeNode(QModelIndex index);
	//��ѡ״̬�仯
	void signal_checkStateChangedTreeNode(int layerUID, int mapUID,bool checkState);
	//ѡ�ж���
	void selectTreeNode(int layerUID, int mapUID);
	//�ɵ�����
	void zoomToTreeNode(int layerUID,int mapUID);

	void testMessage();
private:
	TreeItemNode* itemFromIndex(const QModelIndex& index) const;
	QModelIndex indexFromItem(TreeItemNode* item)const;
	TreeItemNode* root=nullptr;
};
WTNAMESPACEEND
#endif // WLAYERTREEMODEL_H
