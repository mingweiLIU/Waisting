import QtQuick 6.2
import QtQuick.Controls

//该按钮是用于在菜单栏中显示功能的按钮 其本质是一个图标按钮
//功能包括： 1.鼠标hover 点击颜色变化
//          2.设置图片图标
//          3.设置点击事件
//          4.设置点击后颜色
//          5.鼠标hover时显示提示框
Item {
    id:root
    property var tooltipText
    property var tooltipdirection
    property var source
    property color color: "gray"
    property alias  buttonImage: buttonImage

    signal clicked()

    Rectangle{
        id:button
        anchors.fill: parent
        radius:4
        color:{
            if(buttonArea.containsPress){
                return Qt.lighter(root.color)
            }else if(buttonArea.containsMouse){
                return "#393b40"
            }else{
                return root.color
            }
        }
        Image {
            id: buttonImage
            source: root.source
            anchors.fill: parent
        }

        MouseArea{
            id:buttonArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                root.clicked()
            }
            onEntered:{
                let x=root.x+width+mainWindowObj.offset
                let y=root.y
                mainWindowObj.setToolTip(root.tooltipText,x,y)
            }
            onExited:{
                mainWindowObj.clearToolTip()
            }
        }
    }
}
