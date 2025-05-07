import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts



Rectangle {
    id: root
    width: Math.max(innerContainer.width+spaceWidth,labelText.contentWidth)+4
    height: 82
    color: "#eaecee"
    radius: recRadius
    border.color: "#b5bcbc"
    gradient:{
        if(!hoverHanlder.hovered){
            return gradientMap
        }else{
            return gradientDefaultColor
        }
    }


    property int  lineWidth: 1
    property int recRadius:2
    property int spaceWidth: 3
    property int contentNameHeight:14
    property string  label: "这就是一个测asdfaf试"
    property int labelSize: 12
    default property alias data :innerContainer.data
    property alias buttons: innerContainer.children

    Gradient {//线性渐变
        id:gradientTop
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#adafb3" }
        GradientStop{position:0.3;color:"#adafb2"}
    }

    Gradient {//线性渐变
        id:gradientVertical
        GradientStop { position: 0.0; color: "#adb0b4" }
        GradientStop{position:0.3;color:"#8a8d8e"}
    }

    Gradient {//线性渐变
        id:gradientDefaultColor
        GradientStop { position: 0.0; color: "#f2f4f5" }
        GradientStop{position:1.0;color:"#f2f4f5"}
    }

    Gradient {//线性渐变
        id:gradientMap
        GradientStop { position: 0.0; color: "#d5d9de" }
        GradientStop{position:0.2;color:"#c1c6cf"}
        GradientStop{position:0.2;color:"#b4bbc5"}
        GradientStop { position: 1.0; color: "#ebebeb" }
    }

    Rectangle{
        id:topLine
        width: parent.width-2*recRadius
        height: lineWidth
        gradient: gradientTop
        anchors.horizontalCenter: parent.horizontalCenter
    }
    Rectangle{
        id:leftLine
        width: lineWidth
        height: parent.height-2*recRadius
        gradient: gradientVertical
        anchors.verticalCenter: parent.verticalCenter
    }
    Rectangle{
        id:rightLine
        width: lineWidth
        height: parent.height-2*recRadius
        gradient: gradientVertical
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
    }

    Rectangle{
        id:bottomLine
        width: parent.width-recRadius
        height: lineWidth
        color: "#808080"
        anchors.bottom:  parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Rectangle{
        id:contentRec
        width:innerContainer.width
        height: parent.height-lineWidth-spaceWidth-contentNameHeight
        color:"transparent"

        anchors{
            top:inTopSpace.bottom
            left:parent.left
            leftMargin: spaceWidth
            // rightMargin: spaceWidth
            horizontalCenter: parent.horizontalCenter
        }
        RowLayout{
            id:innerContainer
            spacing: 1
            anchors{
                centerIn: parent
            }
            //动态添加的按钮将会在此处
        }
    }
    Rectangle{
        id:outRightSpace
        width: lineWidth
        height: parent.height
        color: "#eef1f2"
        anchors.verticalCenter: parent.verticalCenter
        anchors{
            top:parent.top
            topMargin: recRadius
            left: parent.right
        }
    }
    Rectangle{
        id:inLeftSpace
        width:lineWidth
        height: parent.height
        color:"#eef1f2"
        anchors.verticalCenter: parent.verticalCenter
        anchors{
            top:parent.top
            topMargin: recRadius
            left: leftLine.right
        }
    }
    Rectangle{
        id:inTopSpace
        height:lineWidth-0.3
        width: parent.width-2*recRadius
        color:"#e9ebee"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors{
            top:topLine.bottom
            left: leftLine.right
        }
    }
    Rectangle{
        id:panelNameArea
        width:parent.width-2*lineWidth
        height: 14
        color: {
            if(!hoverHanlder.hovered){
                return "#a9abab"
            }else{
               return "#797a7a"
            }
        }
        anchors{
            top:contentRec.bottom
            left:leftLine.right
        }
        Text {
            id: labelText
            text: label
            anchors{
                verticalCenter: parent.verticalCenter
                horizontalCenter: parent.horizontalCenter
            }
            color: "white"
            font.pixelSize: labelSize
        }
    }

    HoverHandler{
        id:hoverHanlder
        acceptedDevices: PointerDevice.Mouse
    }

    function addButton(icon,text,clickCallback){
        let buttonComponent=Qt.createComponent("RibbonButton.qml");
        if(buttonComponent.status===Component.Ready){
            let button=buttonComponent.createObject(innerContainer,{
                "image": icon,
                "labelText": text
            });

            if (clickCallback) {
                button.clicked.connect(clickCallback);
            }

            return button;
        } else {
            console.error("Error creating button:", buttonComponent.errorString());
            return null;
        }
    }
}
