import QtQuick 2.15
import QtQuick.Controls 2.15
//import QtQuick.Studio.DesignEffects
import QtQuick.Shapes

Item {
    id:root
    property string lableStr: "零秒我我我"
    property int buttonRadius:4         //button的弧度
    property int shadowOffset:2         //阴影偏移值
    property int textSize:12            //文字大小
    property int hoverBorderWidth:1     //active状态下 hover的边框大小
    property string hoverBorderColor:"#e8bb72" //active状态下 hover的边框颜色

    property string unActiveColor:"#535353"
    property bool isActive:false

    signal clicked()

    width:label.contentWidth+30
    height:26

    Gradient{
        id:unActiveHoverGradient
        GradientStop{position: 0.0;color:"#969695"}
        GradientStop{position: 0.8;color:"#a48e47"}
        GradientStop{position: 1.0;color:"#edc226"}
    }

    Rectangle{
        id:ribbonBtn
        anchors.fill: parent
        color:{
            if(isActive){
                return  "#ccd0d3"
            }else{
                return unActiveColor
            }
        }

        // border.width: 3
        radius: buttonRadius
        layer.effect: ShaderEffect {
            property var colorSource: ribbonBtn;
            fragmentShader: "show.frag.qsb"
        }

        MouseArea{
            id:buttonArea
            anchors.fill: parent
            hoverEnabled: true
            onEntered:{
                if(isActive){
                    ribbonBtn.border.color=hoverBorderColor;
                    ribbonBtn.border.width=hoverBorderWidth;
                }else{                   
                    ribbonBtn.layer.enabled= true
                }
            }

            onExited: {
                ribbonBtn.border.width=0;
                ribbonBtn.border.color="transparent"
                ribbonBtn.layer.enabled= false
            }
            onClicked: {
                root.clicked()
            }
        }
    }

    Text{
        id:label
        text: lableStr
        anchors {
            verticalCenter: parent.verticalCenter
            horizontalCenter: parent.horizontalCenter
        }
        font.pixelSize: textSize
        color: isActive ? "black": "white"
    }

}
