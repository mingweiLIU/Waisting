import QtQuick
import QtQuick.Controls

Item {
    id:root
    property var source: "qrc:/qt/qml/Waisting/icon/cangku_zhongzhuanchuzhan.png"
    property alias label: label_text.text
//    property color color: "#2b2d30"
    property color color: "gray"
    property color hoverColor: "#e4eef7"
    property int imgSize:32

    signal clicked()

    Rectangle{
        id:button
        anchors.fill: parent
        radius: 2
        color:{
            if(buttonArea.containsPress){
                return Qt.lighter(root.color)
            }else if(buttonArea.containsMouse){
                return root.hoverColor
            }else{
                return root.color
            }
        }

        Image {
            id: image
            source: root.source
            width: root.imgSize
            height: root.imgSize
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: button.top
            anchors.topMargin: 5
        }
        Text {
            id: label_text
            anchors{
                top: image.bottom
                horizontalCenter: image.horizontalCenter
            }

            text: qsTr("text")
        }

        MouseArea{
            id:buttonArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                console.log("afafafasfd")
                root.clicked()
            }
            onEntered: {

            }
        }
    }
}
