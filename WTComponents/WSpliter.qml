import QtQuick 6.2

Item {
    id:root
    property int preX
    property bool moveSplit: false
    property real leftItemWidth:200
    property real leftItemMinWidth: 100

    property alias leftItem: leftItem.children
    property alias rightItem: rightItem.children


    Rectangle{
        id:leftItem
        color:"#2b2d30"
        anchors{
            left:parent.left
            top:parent.top
            bottom: parent.bottom
            right: spliter.left
        }
    }
    Rectangle{
        id:rightItem
        color:"green"
        anchors{
            right: parent.right
            left:spliter.right
            top:parent.top
            bottom: parent.bottom
        }
    }
    Rectangle{
        id:spliter
        color: "#1e1f22"
        height: parent.height
        width: 2
        x: leftItemWidth

        MouseArea{
            anchors.fill: parent
            cursorShape: Qt.SizeHorCursor
            hoverEnabled: true
            onPressed: {
                root.preX=mouseX
                root.moveSplit=true
            }
            onMouseXChanged: {
                if(root.moveSplit){
                    var offset=mouseX-root.preX
                    var xPos=offset+spliter.x
                    if(xPos<leftItemMinWidth) return
                    if(xPos>=0&&xPos<root.width){
                        spliter.x=xPos
                    }
                }
            }
            onReleased: {
                root.moveSplit=false
            }
        }
    }
}
