import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts

Rectangle {
    id:root
    height: 88
    // width:parent? parent.width :200
    width: 600
    default property alias data:innerContainer.data

    Rectangle{
        id:ribbonRec
        anchors.fill: parent
        color:"#535353"
        gradient: Gradient {//线性渐变
            GradientStop { position: 0.0; color: "#d5d9de" }
            GradientStop{position:0.2;color:"#c1c6cf"}
            GradientStop{position:0.2;color:"#b4bbc5"}
            GradientStop { position: 1.0; color: "#ebebeb" }
        }


        RowLayout{
            id:innerContainer
            spacing: 2
            anchors{
                left: ribbonRec.left
                leftMargin: 3
                top: ribbonRec.top
                topMargin: 3
            }
        }
    }

    function addPanel(title,buttons){
        let panelComponent=Qt.createComponent("RibbonPanel.qml");
        if(panelComponent.status===Component.Ready){
            let panel=panelComponent.createObject(innerContainer,{"label":title});

            //添加按钮
            if(buttons&&buttons.length>0){
                for(let i=0;i<button.length;++i){
                    panel.addButton(buttons[i].icon,buttons[i].text,buttons[i].onClicked);
                }
            }
            return panel;
        }else{
            console.log("创建Panel失败",panelComponent.errorString());
            return null;
        }

    }
}
