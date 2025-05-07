import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts

Rectangle {
    id: root
    property int btnWidth:Math.max(Math.max(label.contentWidth,img.width)+8,72);
    property int btnHeight: 63
    property int btnBoardWidth: 1
    property int btnRadius: 2
    property int labelSize:10
    property string btnHoverBorderColor: "#d4c6a9"
    property string btnPressBorderColor: "#b7a17b"
    property string image: "./imgs/5.png"
    property string labelText: "按钮"

    width: btnWidth
    height: btnHeight
    opacity:1.0
    color:"transparent"
    radius: btnRadius

    signal clicked()

    gradient: {
        if(tapHandler.pressed){
            return pressGradient
        }else if(hoverHandler.hovered){
            return hoverGradient
        }else{
            return undefined
        }
    }

    border.color: {
        if(tapHandler.pressed){
            return btnPressBorderColor
        }else if(hoverHandler.hovered){
            return btnHoverBorderColor
        }else{
            return ""
        }
    }

    border.width: {
        if(tapHandler.pressed||hoverHandler.hovered){
            return btnBoardWidth
        }else{
            return 0
        }
    }

    Gradient{
        id:hoverGradient
        GradientStop{position: 0.0;color: "#fffddc"}
        GradientStop{position: 0.4;color: "#ffe797"}
        GradientStop{position: 0.4;color: "#ffd754"}
        GradientStop{position: 1.0;color: "#ffdf89"}
    }
    Gradient{
        id:pressGradient
        GradientStop{position: 0.0;color: "#f7b56a"}
        GradientStop{position: 0.4;color: "#fb903e"}
        GradientStop{position: 0.4;color: "#fb923e"}
        GradientStop{position: 1.0;color: "#fbbc40"}
    }

    Image{
        id:img
        width: 32
        height: 32
        source: image
        anchors.horizontalCenter: parent.horizontalCenter
        anchors{
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: 6
        }
    }
    Text {
        id: label
        font.pixelSize: labelSize
        text:labelText
        anchors{
            top:img.bottom
            horizontalCenter: parent.horizontalCenter
            topMargin: 4
        }

    }

    HoverHandler{
        id:hoverHandler
        acceptedDevices: PointerDevice.Mouse
        // cursorShape: Qt.PointingHandCursor
    }

    TapHandler{
        id:tapHandler
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onTapped: clicked()
    }

}
