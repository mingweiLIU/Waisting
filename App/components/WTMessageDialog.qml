// CustomMessageDialog.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

//使用信息弹出窗
    // WTMessageDialog {
    //     id: infoDialog


    //     // 处理确认事件
    //     onConfirmed: {
    //         console.log("用户点击了确定按钮")
    //     }

    //     // 处理取消事件
    //     onCancelled: {
    //         console.log("用户点击了取消按钮或关闭了对话框")
    //     }
    // }
    // WTMessageDialog {
    //     id: infoDialog
    //     title: "确认操作"
    //     showCancelButton: true
    //     okText: "确认"
    //     cancelText: "取消"

    //     onConfirmed: {
    //         statusLabel.text = "用户确认了操作"
    //     }

    //     onCancelled: {
    //         statusLabel.text = "用户取消了操作"
    //     }
    // }

Popup {
    id: root

    // 公开属性，便于外部调用和定制
    property string title: "提示"
    property string message: ""
    property string okText: "确定"
    property string cancelText: "取消"
    property bool showCancelButton: false
    property color backgroundColor: "#2b2d30"
    property color textColor: "#ffffff"
    property color buttonBackgroundColor: "#3c3f41"
    property color buttonHoverColor: "#4e5254"
    property color buttonPressedColor: "#5a5d5f"
    property color borderColor: "#1e1f22"

    // 信号，用于通知外部操作结果
    signal confirmed()
    signal cancelled()

    // 弹出框基本设置
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent
    width: Math.min(parent.width * 0.8, 400)
    padding: 0

    // 弹出和关闭动画
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
        NumberAnimation { property: "scale"; from: 0.8; to: 1.0; duration: 200 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 200 }
        NumberAnimation { property: "scale"; from: 1.0; to: 0.8; duration: 200 }
    }

    background: Rectangle {
        color: root.backgroundColor
        border.color: root.borderColor
        border.width: 1
        radius: 6
    }

    contentItem: ColumnLayout {
        spacing: 16
        width: parent.width
        z: 10 // 确保遮罩在最上层

        // 标题栏
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: Qt.darker(root.backgroundColor, 1.1)
            radius: 6

            // 圆角修正（只有顶部圆角）
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.radius
                color: parent.color
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Label {
                    text: root.title
                    font.bold: true
                    font.pixelSize: 14
                    color: root.textColor
                    Layout.fillWidth: true
                }

                Button {
                    flat: true
                    text: "✕"
                    Layout.preferredHeight: 24
                    Layout.preferredWidth: 24
                    padding: 0

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        color: root.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: parent.down ? "#aa3333" : (parent.hovered ? "#993333" : "transparent")
                        radius: 3
                    }

                    onClicked: {
                        root.cancelled()
                        root.close()
                    }
                }
            }
        }

        // 消息内容
        Label {
            text: root.message
            color: root.textColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }

        // 按钮区域
        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignRight
            Layout.rightMargin: 20
            Layout.bottomMargin: 16

            Button {
                id: cancelButton
                text: root.cancelText
                visible: root.showCancelButton
                Layout.preferredWidth: 80
                Layout.preferredHeight: 32

                contentItem: Text {
                    text: parent.text
                    color: root.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: parent.down ? root.buttonPressedColor : (parent.hovered ? root.buttonHoverColor : root.buttonBackgroundColor)
                    radius: 4
                    border.color: Qt.darker(color, 1.2)
                    border.width: 1
                }

                onClicked: {
                    root.cancelled()
                    root.close()
                }
            }

            Button {
                id: okButton
                text: root.okText
                Layout.preferredWidth: 80
                Layout.preferredHeight: 32

                contentItem: Text {
                    text: parent.text
                    color: root.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: parent.down ? root.buttonPressedColor : (parent.hovered ? root.buttonHoverColor : root.buttonBackgroundColor)
                    radius: 4
                    border.color: Qt.darker(color, 1.2)
                    border.width: 1
                }

                onClicked: {
                    root.confirmed()
                    root.close()
                }
            }
        }
    }

    // 简化调用的函数
    function showMessage(msg, title) {
        if (title) root.title = title
        root.message = msg
        root.open()
    }

    // 额外提供确认对话框模式
    function showConfirmation(msg, title) {
        if (title) root.title = title
        root.message = msg
        root.showCancelButton = true
        root.open()
    }
}
