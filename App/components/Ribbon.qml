import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts

Rectangle {
    id:root
    height: 112
    width:parent.width
    color:"#535353"

    //可自定义属性
    property alias tabs:rowOfTabs.children
    property alias content:stackLayout.children
    property int defaultTabIndex:0
    property int tabHeight:27
    property int tabLeftMargin:70
    property int panelHeight:88

    signal currentTabChanged(int index)

    Component.onCompleted:{
        if(tabs.length>0){
            checkTab(defaultTabIndex);
        }
    }


    Rectangle{
        id:ribbonBar
        height:tabHeight
        color:"transparent"
        anchors{
            left: parent.left
            right: parent.right
            top: parent.top
            leftMargin: tabLeftMargin
        }

        RowLayout{
            id:rowOfTabs
            spacing: 5
            anchors{
                bottom: parent.bottom
                bottomMargin: -3
            }
            //这里会放置通过addTab方法添加的RibbonTabButton
        }
    }


    StackLayout{
        id:stackLayout
        anchors{
            top:ribbonBar.bottom
        }
        width:root.width
        height: panelHeight

        // 这里会放置通过addTab方法添加的RibbonTab内容
    }

    //切换tab
    function checkTab(index){
        if(index<0 || index>=rowOfTabs.children.length) return;

        for(let i=0,i_up=rowOfTabs.children.length;i<i_up;++i){
            rowOfTabs.children[i].isActive=(index===i);
        }

        stackLayout.currentIndex=index;
        currentTabChanged(index);
    }

    //添加标签页
    function addTab(tabTitle,tabContent){
        let tabComponent=Qt.createComponent("RibbonTabButton.qml");
        if(tabComponent.status===Component.Ready){
            let tabButton=tabComponent.createObject(rowOfTabs,{
                "lableStr":tabTitle
            });

            let tabIndex=rowOfTabs.children.length-1;
            tabButton.clicked.connect(function(){
                checkTab(tabIndex);
            });

            if(tabContent){
                tabContent.parent=stackLayout;
            }
            return tabIndex;
        }else{
            console.error("创建Tab错误",tabComponent.errorString());
            return -1;
        }
    }

    //根据索引获取标签页
    function getTab(index){
        if (index >= 0 && index < stackLayout.children.length) {
            return stackLayout.children[index];
        }
        return null;
    }
    
}
