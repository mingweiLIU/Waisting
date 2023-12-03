import QtQuick 6.2

Item {
    id:root
    property int preX
    property bool moveSplit: false
    property real spliteRatio:0.5


    Rectangle{
        id:left
        color:"gray"
        anchors{
            left:parent.left
            top:parent.top
            bottom: parent.bottom
            right: spliter.left
        }
    }
    Rectangle{
        id:right
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
        color: "blue"
        height: parent.height
        width: 2
        x: spliteRatio*parent.width

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
                    if(xPos>=0&&xPos<root.width){
                        spliter.x+=offset
                    }
                }
            }
            onReleased: {
                root.moveSplit=false
            }
        }
    }
}
