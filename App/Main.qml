import QtQuick
import QtQuick.Controls
import QtQuick.Window

import WTComponents
import WTOSG
import Test 1.0
import Waisting 1.0

Window{
    id:mainwindow
    visible: true
//    visibility: "Maximized"
    title:qsTr("三维空间场景可视化编辑系统")
    color:"#2b2d30"
    minimumHeight: 900
    minimumWidth: 1300

    property int leftMenuWidth: 40
    property int rightMenuWidth:40
    property int bottomMenuWidth:25
    property int splitlineWidth: 1

    property int buttonOffset: 5

    property color leftMenuColor:"#2b2d30"
    property color rightMenuColor:"#2b2d30"
    property color bottomMenuColor:"#2b2d30"
    property color workAreaColor:"#1e1f22"
    property color splitlineColor:"#1e1f22"


    //左侧菜单栏
    Rectangle{
        id:leftMenu
        width:leftMenuWidth-splitlineWidth
        height:parent.height
        color:leftMenuColor
        anchors{
            left:parent.left
            top:parent.top
            bottom:bottomMenu.top
        }
        //设置一个边线
        Rectangle{
            height: parent.height
            width:splitlineWidth
            anchors.right: parent.right
            border.color: splitlineColor
        }

        //设置各个功能按钮
        Column{
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 2*buttonOffset
            topPadding:buttonOffset
            WButton{
                id:packageBtn
                color:leftMenuColor
                width:leftMenuWidth-2*buttonOffset;height:leftMenuWidth-2*buttonOffset
                source:"qrc:/qt/qml/Waisting/svg/package.svg"
                tooltipdirection:right
                tooltipText:"刘铭崴"
                onClicked:{
                    mainWindowObj.buttonClicked(packageBtn)
                }
            }
            WButton{
                id:boxBtn
                color:leftMenuColor
                width:leftMenuWidth-2*buttonOffset;height:leftMenuWidth-2*buttonOffset
                source:"qrc:/qt/qml/Waisting/svg/box.svg"
                tooltipdirection:right
                tooltipText:"wweeeeeeeeee"
                onClicked:{
                    mainWindowObj.buttonClicked(boxBtn)
                }
            }
            WButton{
                id:getBtn
                color:leftMenuColor
                width:leftMenuWidth-2*buttonOffset;height:leftMenuWidth-2*buttonOffset
                source:"qrc:/qt/qml/Waisting/svg/get.svg"
                tooltipdirection:right
                tooltipText:"aa"
                onClicked:{
                    mainWindowObj.buttonClicked(getBtn)
                }
            }
            WButton{
                id:compareBtn
                color:leftMenuColor
                width:leftMenuWidth-2*buttonOffset;height:leftMenuWidth-2*buttonOffset
                source:"qrc:/qt/qml/Waisting/svg/compare.svg"
                tooltipdirection:right
                tooltipText:"dddd"
                onClicked:{
                    mainWindowObj.buttonClicked(compareBtn)
                }
            }
            WButton{
                id:exportBtn
                color:leftMenuColor
                width:leftMenuWidth-2*buttonOffset;height:leftMenuWidth-2*buttonOffset
                source:"qrc:/qt/qml/Waisting/svg/export.svg"
                tooltipdirection:right
                tooltipText:"刘铭崴ddd"
                onClicked:{
                    mainWindowObj.buttonClicked(exportBtn)
                }
            }
            WButton{
                id:iosmoreBtn
                color:leftMenuColor
                width:leftMenuWidth-2*buttonOffset;height:leftMenuWidth-2*buttonOffset
                source:"qrc:/qt/qml/Waisting/svg/iosmore.svg"
                tooltipdirection:right
                tooltipText:"刘铭崴啊发发的"
                onClicked:{
                    mainWindowObj.buttonClicked(iosmoreBtn)
                }
            }
        }
    }

    //右侧菜单栏
    Rectangle{
        id:rightMenu
        width:rightMenuWidth
        height:parent.height
        color:rightMenuColor
        anchors{
            right:parent.right
            top:parent.top
            bottom:bottomMenu.top
        }

        //设置一个边线
        Rectangle{
            height: parent.height
            width:splitlineWidth
            anchors.left: parent.left
            border.color: splitlineColor
        }
    }

    //底面消息栏
    Rectangle{
        id:bottomMenu
        width:parent.width
        height:bottomMenuWidth
        color:bottomMenuColor
        anchors.bottom:parent.bottom

        //设置一个边线
        Rectangle{
            width: parent.width
            height:splitlineWidth
            anchors.top: parent.top
            border.color: splitlineColor
        }
    }

    //中间工作区
    Rectangle{
        id:workArea
        width:parent.width-leftMenu.width-rightMenu.width
        height: parent.height-bottomMenu.height
        anchors{
            left:leftMenu.right
            right:rightMenu.left
            bottom:bottomMenu.top
            top:parent.top
        }
        color:workAreaColor

        WSpliter{
            anchors.fill:parent
            leftItemWidth:295
            leftItemMinWidth:200
            leftItem: [
//                DataHandlerTool{
//                    anchors.fill: parent
//                }
                WLayerTree{
                    anchors.fill: parent
                }

            ]
            rightItem:[
                 WTOSGViewer{
                    id:wtOSGViewer
                    anchors.fill:parent
                }
            ]
        }

    }

    Rectangle{
        id:toolTip
        Text {
            id: toolTipText
            text: qsTr("测试")
            anchors.centerIn: parent
        }
        height: leftMenuWidth-2*buttonOffset
        width: 56
        x:120
        y:126
        color: "#393b40"
        radius: 4
        border{
            width: 2
            color: "#43454a"
        }
        visible: false
    }

    WFileObject{
        id:globe_FileObject
    }

    QtObject{
        id:mainWindowObj
        property int charWidth:15
        property int x: 0
        property int y: 0
        property bool visible: false
        property int width:0
        property int height:leftMenuWidth-2*buttonOffset
        property int offset: 4*buttonOffset
        property var selectedBtn

        //计算长度
        function setToolTip(tips,x,y){
            if(tips===""||tips===undefined) return
            toolTip.visible=true
            toolTipText.text=tips
            let numChar=tips.length
            toolTip.width=numChar*charWidth
            toolTip.x=x;toolTip.y=y
        }
        function clearToolTip(){
            toolTipText.text=""
            toolTip.visible=false
        }
        //切换菜单栏上的按钮颜色
        function buttonClicked(btnID){
            if(mainWindowObj.selectedBtn!==undefined){
                mainWindowObj.selectedBtn.color=mainwindow.leftMenuColor
            }
            mainWindowObj.selectedBtn=btnID
            mainWindowObj.selectedBtn.color="#3574f0"
        }

    }
}
