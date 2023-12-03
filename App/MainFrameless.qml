/* This file is generated and only relevant for integrating the project into a Qt 6 and cmake based
C++ project. */

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import WTComponents

ApplicationWindow{
    id:window
    visible: true
    height: 480;width:640
    flags: Qt.FramelessWindowHint|Qt.Window
    color:"#2b2d30"
    property int bw: 3
    property int offset: 15                                 //各大小调整鼠标范围
    property int toolbarHeight: 45
    property string toolbarColor: "#3c3f41"                 //菜单栏底色
    property string splitLineColor:"#1e1f22"                //分割栏颜色


    function toggleMaximized() {
        if (window.visibility === Window.Maximized) {
            window.showNormal();
        } else {
            window.showMaximized();
        }
    }

    function onResizing(shouldMove=false){
        if (active) {
             const p = resizeHandler.centroid.position;
             const b = bw + offset; // Increase the corner size slightly

            if(shouldMove&&p.x>=b&&p.x<width-b&&p.y>=b&&p.y<height-b){
                window.startSystemMove()
            }else{
                let e = 0;
                if (p.x < b) { e |= Qt.LeftEdge }
                if (p.x >= width - b) { e |= Qt.RightEdge }
                if (p.y < b) { e |= Qt.TopEdge }
                if (p.y >= height - b) { e |= Qt.BottomEdge }
                window.startSystemResize(e);
            }
        }
    }

    // The mouse area is just for setting the right cursor shape
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: {
            const p = Qt.point(mouseX, mouseY);
            const b = bw + offset; // Increase the corner size slightly
            if (p.x < b && p.y < b) return Qt.SizeFDiagCursor;
            if (p.x >= width - b && p.y >= height - b) return Qt.SizeFDiagCursor;
            if (p.x >= width - b && p.y < b) return Qt.SizeBDiagCursor;
            if (p.x < b && p.y >= height - b) return Qt.SizeBDiagCursor;
            if (p.x < b || p.x >= width - b) return Qt.SizeHorCursor;
            if (p.y < b || p.y >= height - b) return Qt.SizeVerCursor;
        }
        acceptedButtons: Qt.NoButton // don't handle actual events
    }

    DragHandler {
        id: resizeHandler
        grabPermissions: TapHandler.TakeOverForbidden
        target: null
        onActiveChanged: {onResizing()}
    }

    ToolBar {
        width: parent.width
        height: toolbarHeight
        background: Rectangle{
            anchors.fill: parent
            color: toolbarColor
            Rectangle{
                height: 1
                width: parent.width
                anchors.bottom: parent.bottom
                border.color:splitLineColor
            }
        }
        //恢复菜单栏两侧和顶部能拉伸的操作
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: {
                const p = Qt.point(mouseX, mouseY);
                const b = bw + offset; // Increase the corner size slightly
                if (p.x < b && p.y < b) return Qt.SizeFDiagCursor;
                if (p.x >= width - b && p.y < b) return Qt.SizeBDiagCursor;
                if (p.x < b || p.x >= width - b) return Qt.SizeHorCursor;
                if (p.y < b) return Qt.SizeVerCursor;
            }
            acceptedButtons: Qt.NoButton // don't handle actual events
        }

        Item {
            anchors.fill: parent
            TapHandler {
                onTapped: if (tapCount === 2) toggleMaximized()
                gesturePolicy: TapHandler.DragThreshold
            }
            DragHandler {
                grabPermissions: TapHandler.CanTakeOverFromAnything
                onActiveChanged: {onResizing(true);}
            }
        }

        Frame{
            id:windowMMCBtnPanel
            width: 3*toolbarHeight;height: parent.height
            anchors.right: parent.right
            background: Rectangle{
                anchors.fill: parent
                width: parent.width;height:parent.height
                color:"gray"
            }
            Row{
                anchors{
                    verticalCenter: parent.verticalCenter
                }

                Rectangle{
                    id:minBtn
                    width: toolbarHeight;height: toolbarHeight
                    color: "yellow"
                }

                Rectangle{
                    id:maxBtn
                    width: toolbarHeight;height: toolbarHeight
                }

                Rectangle{
                    id:closeBtn
                    width: toolbarHeight;height: toolbarHeight
                    color: "yellow"
                }
            }

        }

        //下面创建各个按钮
//        Rectangle{
//            id:closeBtn
//            color:toolbarColor
//            height: toolbarHeight;width: toolbarHeight
//            onHover

//        }

    }
}
