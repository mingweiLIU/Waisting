import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Window {
    id: paramDialog
    title: "切片参数设置"
    width: 550 + (helpPanelVisible ? 300 : 0)  // 主窗口宽度固定，帮助面板宽度额外计算
    height: advancedOptionsVisible ? 890 : 520
    flags: Qt.Dialog  // 设置为对话框样式
    color: "#f5f5f5"  // 设置背景色

    // 只为高度添加动画
    Behavior on height {
        NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
    }

    // 参数属性
    property string inputFile: ""
    property string outputDir: ""
    property int minLevel: 15
    property int maxLevel: 18
    property var nodata:[] //[0.0, 0.0, 0.0]
    property string outputFormat: "png"
    property string prjFilePath: ""
    property string wktString: ""
    property real progress: 0
    property bool advancedOptionsVisible: false
    property bool helpPanelVisible: false
    property bool usePrj: false  // 控制使用PRJ
    property bool useCoordinateSystem: true  // 控制是否使用坐标系统
    property int coordinateSystemType: 0  // 0=影像自带坐标, 1=PRJ文件, 2=WKT字符串

    signal accepted()
    signal rejected()

    // 主布局 - 改为水平布局容器
    Item {
        anchors.fill: parent

        // 主设置区域 - 宽度固定，不受帮助面板影响
        Rectangle {
            id: mainSettingsContainer
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 550
            color: "#f5f5f5"

            ColumnLayout {
                id: mainSettingsArea
                anchors.fill: parent
                spacing: 16
                anchors.margins: 16

                // 文件设置
                GroupBox {
                    title: "文件设置"
                    Layout.fillWidth: true

                    // 自定义标题样式
                    label: Label {
                        text: parent.title
                        font.bold: true  // 标题粗体
                        font.pixelSize: 14
                        padding: 6  // 增加标题上下padding
                    }

                    background: Rectangle {
                        color: "#ffffff"
                        border.color: "#cccccc"
                        radius: 6
                    }
                    topPadding:30
                    rightPadding:16
                    GridLayout {
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 12
                        width: parent.width

                        Label {
                            text: "输入文件:"
                            Layout.alignment: Qt.AlignVCenter
                            font.pixelSize: 14
                        }
                        RowLayout {
                            TextField {
                                id: inputFileField
                                text: inputFile
                                Layout.fillWidth: true
                                placeholderText: "请选择输入文件"
                                onTextChanged: inputFile = text
                                selectByMouse: true
                                background: Rectangle {
                                    radius: 4
                                    border.color: inputFileField.activeFocus ? "#4285f4" : "#cccccc"
                                    border.width: inputFileField.activeFocus ? 2 : 1
                                }
                            }
                            Button {
                                text: "浏览"
                                onClicked: fileDialog.open()
                                background: Rectangle {
                                    radius: 4
                                    color: parent.down ? "#3875d7" : parent.hovered ? "#4285f4" : "#5c95f5"
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }

                        Label {
                            text: "输出目录:"
                            Layout.alignment: Qt.AlignVCenter
                            font.pixelSize: 14
                        }
                        RowLayout {
                            TextField {
                                id: outputDirField
                                text: outputDir
                                Layout.fillWidth: true
                                placeholderText: "请选择输出目录"
                                onTextChanged: outputDir = text
                                selectByMouse: true
                                background: Rectangle {
                                    radius: 4
                                    border.color: outputDirField.activeFocus ? "#4285f4" : "#cccccc"
                                    border.width: outputDirField.activeFocus ? 2 : 1
                                }
                            }
                            Button {
                                text: "浏览"
                                onClicked: dirDialog.open()
                                background: Rectangle {
                                    radius: 4
                                    color: parent.down ? "#3875d7" : parent.hovered ? "#4285f4" : "#5c95f5"
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }

                // 切片级别设置
               GroupBox {
                   title: "切片级别设置"
                   Layout.fillWidth: true

                   // 自定义标题样式
                   label: Label {
                       text: parent.title
                       font.bold: true  // 标题粗体
                       font.pixelSize: 14
                       padding: 6  // 增加标题上下padding
                   }

                   background: Rectangle {
                       color: "#ffffff"
                       border.color: "#cccccc"
                       radius: 6
                   }
                   topPadding: 30
                   rightPadding: 16  // 增加内容padding

                   RowLayout {
                       spacing: 20
                       width: parent.width

                       Label {
                           id:levellabel
                           text: "级别范围:"
                           Layout.alignment: Qt.AlignVCenter
                           font.pixelSize: 14
                       }


                       RowLayout{
                           width: parent.width-100
                           // 修改SpinBox，增加宽度以显示数字，设置合理的步长
                           SpinBox {
                               id: minLevelBox
                               value: minLevel
                               from: 0
                               to: maxLevel
                               onValueChanged: minLevel = value
                               Layout.preferredWidth: 120
                               Layout.preferredHeight: 30  // 设置为30的高度
                               editable: true

                               // 自定义显示
                               contentItem: TextInput {
                                   text: minLevelBox.textFromValue(minLevelBox.value, minLevelBox.locale)
                                   font: minLevelBox.font
                                   horizontalAlignment: Qt.AlignHCenter
                                   verticalAlignment: Qt.AlignVCenter
                                   readOnly: !minLevelBox.editable
                                   validator: minLevelBox.validator
                                   selectByMouse: true
                                   color: "#333333"
                               }

                               // 美化样式
                               background: Rectangle {
                                   implicitWidth: 120
                                   implicitHeight: 30  // 设置高度为30
                                   border.color: "#cccccc"
                                   border.width: 1
                                   radius: 4
                               }

                               // 自定义按钮
                               up.indicator: Rectangle {
                                   anchors.right: parent.right
                                   height: 30
                                   width: 30  // 设置按钮宽度为30
                                   implicitWidth: 30
                                   color: minLevelBox.up.pressed ? "#d0d0d0" : "#f0f0f0"
                                   border.color: "#cccccc"
                                   border.width: 1
                                   radius: 4
                                   Text {
                                       text: "+"
                                       color: "#333333"
                                       anchors.centerIn: parent
                                       font.pixelSize: 14
                                   }
                               }

                               down.indicator: Rectangle {
                                   anchors.left: parent.left
                                   height: 30
                                   width: 30  // 设置按钮宽度为30
                                   implicitWidth: 30
                                   color: minLevelBox.down.pressed ? "#d0d0d0" : "#f0f0f0"
                                   border.color: "#cccccc"
                                   border.width: 1
                                   radius: 4
                                   Text {
                                       text: "-"
                                       color: "#333333"
                                       anchors.centerIn: parent
                                       font.pixelSize: 14
                                   }
                               }
                           }

                           Label {
                               text: "到"
                               font.pixelSize: 14
                               Layout.alignment: Qt.AlignVCenter  // 垂直居中对齐
                               Layout.fillWidth: true  // 关键：让Label填充剩余空间
                               horizontalAlignment: Text.AlignHCenter  // 文字居中
                           }

                           SpinBox {
                               id: maxLevelBox
                               value: maxLevel
                               from: minLevel
                               to: 25
                               onValueChanged: maxLevel = value
                               Layout.preferredWidth: 120
                               Layout.preferredHeight: 30  // 设置为30的高度
                               editable: true

                               // 自定义显示
                               contentItem: TextInput {
                                   text: maxLevelBox.textFromValue(maxLevelBox.value, maxLevelBox.locale)
                                   font: maxLevelBox.font
                                   horizontalAlignment: Qt.AlignHCenter
                                   verticalAlignment: Qt.AlignVCenter
                                   readOnly: !maxLevelBox.editable
                                   validator: maxLevelBox.validator
                                   selectByMouse: true
                                   color: "#333333"
                               }

                               // 美化样式
                               background: Rectangle {
                                   implicitWidth: 120
                                   implicitHeight: 30  // 设置高度为30
                                   border.color: "#cccccc"
                                   border.width: 1
                                   radius: 4
                               }

                               // 自定义按钮
                               up.indicator: Rectangle {
                                   anchors.right: parent.right
                                   height: 30
                                   width: 30  // 设置按钮宽度为30
                                   implicitWidth: 30
                                   color: maxLevelBox.up.pressed ? "#d0d0d0" : "#f0f0f0"
                                   border.color: "#cccccc"
                                   border.width: 1
                                   radius: 4
                                   Text {
                                       text: "+"
                                       color: "#333333"
                                       anchors.centerIn: parent
                                       font.pixelSize: 14
                                   }
                               }

                               down.indicator: Rectangle {
                                   anchors.left: parent.left
                                   height: 30
                                   width: 30  // 设置按钮宽度为30
                                   implicitWidth: 30
                                   color: maxLevelBox.down.pressed ? "#d0d0d0" : "#f0f0f0"
                                   border.color: "#cccccc"
                                   border.width: 1
                                   radius: 4
                                   Text {
                                       text: "-"
                                       color: "#333333"
                                       anchors.centerIn: parent
                                       font.pixelSize: 14
                                   }
                               }
                           }

                       }
                   }
               }

                // 输出格式 - 优化排版
                GroupBox {
                    title: "输出设置"
                    Layout.fillWidth: true

                    // 自定义标题样式
                    label: Label {
                        text: parent.title
                        font.bold: true  // 标题粗体
                        font.pixelSize: 14
                        padding: 6  // 增加标题上下padding
                    }

                    background: Rectangle {
                        color: "#ffffff"
                        border.color: "#cccccc"
                        radius: 6
                    }
                    leftPadding: 16  // 增加内容padding
                    topPadding:30

                    GridLayout {
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 12
                        width: parent.width

                        Label {
                            text: "输出格式:"
                            Layout.alignment: Qt.AlignVCenter
                            font.pixelSize: 14
                        }

                        ComboBox {
                            id: formatComboBox
                            model: ["png", "jpg", "webp", "tif"]
                            currentIndex: model.indexOf(outputFormat)
                            onCurrentTextChanged: outputFormat = currentText
                            Layout.preferredWidth: 350
                            Layout.preferredHeight: 30  // 增加高度

                            // 美化样式
                            background: Rectangle {
                                radius: 4
                                border.color: formatComboBox.activeFocus ? "#4285f4" : "#cccccc"
                                border.width: formatComboBox.activeFocus ? 2 : 1
                            }

                            // 下拉项样式
                            popup: Popup {
                                y: formatComboBox.height
                                width: formatComboBox.width
                                padding: 1

                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: formatComboBox.popup.visible ? formatComboBox.delegateModel : null

                                    ScrollIndicator.vertical: ScrollIndicator {}
                                }

                                background: Rectangle {
                                    border.color: "#cccccc"
                                    radius: 4
                                }
                            }

                            // 确保下拉菜单项有合理的高度
                            delegate: ItemDelegate {
                                width: formatComboBox.width
                                height: 30  // 与ComboBox高度匹配
                                contentItem: Text {
                                    text: modelData
                                    color: "#333333"
                                    font: formatComboBox.font
                                    elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                                highlighted: formatComboBox.highlightedIndex === index
                            }
                        }
                    }
                }

                // 高级选项标题 - 美化
                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "#e9f0fd"  // 浅蓝色背景
                    border.color: "#bbccee"
                    radius: 6

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        Label {
                            text: "高级选项"
                            font.bold: true
                            font.pixelSize: 14
                            color: "#2255aa"
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: advancedOptionsVisible ? "▲" : "▼"
                            flat: true
                            onClicked: advancedOptionsVisible = !advancedOptionsVisible

                            background: Rectangle {
                                color: "transparent"
                            }

                            contentItem: Text {
                                text: parent.text
                                color: "#2255aa"
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: advancedOptionsVisible = !advancedOptionsVisible
                        cursorShape: Qt.PointingHandCursor
                    }
                }

                // 高级选项内容
                Item {
                    id: advancedOptionsContainer
                    Layout.fillWidth: true
                    implicitHeight: advancedOptions.implicitHeight // 预先计算高度
                    Layout.preferredHeight: advancedOptionsVisible ? advancedOptions.implicitHeight : 0
                    visible: advancedOptionsVisible // 使用可见性代替裁剪，避免布局抖动

                    // 添加高度动画
                    Behavior on Layout.preferredHeight {
                        NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
                    }

                    ColumnLayout {
                        id: advancedOptions
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 16
                        opacity: advancedOptionsVisible ? 1.0 : 0

                        // 添加不透明度动画
                        Behavior on opacity {
                            NumberAnimation { duration: 300 }
                        }

                        // 坐标系统设置
                        GroupBox {
                            title: "坐标系统设置"
                            Layout.fillWidth: true

                            // 自定义标题样式
                            label: Label {
                                text: parent.title
                                font.bold: true  // 标题粗体
                                font.pixelSize: 14
                                padding: 6  // 增加标题上下padding
                            }

                            background: Rectangle {
                                color: "#ffffff"
                                border.color: "#cccccc"
                                radius: 6
                            }
                            leftPadding: 16  // 增加内容padding
                            topPadding:30

                            ColumnLayout {
                                width: parent.width
                                spacing: 12

                                // 修改为三个单选按钮
                                ColumnLayout {
                                    spacing: 12

                                    // 单选按钮组 - 使用影像自带坐标系统
                                    RadioButton {
                                        id: useImageCoordinateRadioButton
                                        text: "使用影像自带坐标"
                                        checked: coordinateSystemType === 0
                                        onCheckedChanged: {
                                            if (checked) {
                                                coordinateSystemType = 0
                                                prjFilePath = ""  // 清空PRJ文件路径
                                                wktString = ""    // 清空WKT
                                                prjFileField.text = ""
                                                wktTextField.text = ""
                                            }
                                        }
                                    }

                                    // 单选按钮组 - 使用PRJ文件
                                    RadioButton {
                                        id: prjRadioButton
                                        text: "使用PRJ文件"
                                        checked: coordinateSystemType === 1
                                        onCheckedChanged: {
                                            if (checked) {
                                                coordinateSystemType = 1
                                                wktString = ""    // 清空WKT
                                                wktTextField.text = ""
                                            }
                                        }
                                    }

                                    // PRJ文件选择区域
                                    GridLayout {
                                        columns: 2
                                        rowSpacing: 10
                                        columnSpacing: 12
                                        width: parent.width
                                        enabled: coordinateSystemType === 1  // 根据单选按钮状态禁用/启用
                                        opacity: coordinateSystemType === 1 ? 1.0 : 0.5  // 视觉上的禁用效果
                                        Layout.leftMargin: 24  // 缩进，表示层级

                                        Label {
                                            text: "PRJ文件:"
                                            font.pixelSize: 14
                                        }

                                        RowLayout {
                                            TextField {
                                                id: prjFileField
                                                text: prjFilePath
                                                Layout.fillWidth: true
                                                placeholderText: "请选择PRJ文件"
                                                onTextChanged: {
                                                    if (coordinateSystemType === 1) {
                                                        prjFilePath = text
                                                    }
                                                }
                                                selectByMouse: true
                                                background: Rectangle {
                                                    radius: 4
                                                    border.color: prjFileField.activeFocus && coordinateSystemType === 1 ? "#4285f4" : "#cccccc"
                                                    border.width: prjFileField.activeFocus && coordinateSystemType === 1 ? 2 : 1
                                                }
                                            }
                                            Button {
                                                text: "浏览"
                                                onClicked: {
                                                    if (coordinateSystemType === 1) {
                                                        prjFileDialog.open()
                                                    }
                                                }
                                                background: Rectangle {
                                                    radius: 4
                                                    color: parent.down && coordinateSystemType === 1 ? "#3875d7" :
                                                          parent.hovered && coordinateSystemType === 1 ? "#4285f4" :
                                                          coordinateSystemType === 1 ? "#5c95f5" : "#aaaaaa"
                                                }
                                                contentItem: Text {
                                                    text: parent.text
                                                    color: "white"
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }
                                            }
                                        }
                                    }

                                    // 单选按钮组 - 使用WKT字符串
                                    RadioButton {
                                        id: wktRadioButton
                                        text: "使用WKT字符串"
                                        checked: coordinateSystemType === 2
                                        onCheckedChanged: {
                                            if (checked) {
                                                coordinateSystemType = 2
                                                prjFilePath = ""  // 清空PRJ文件路径
                                                prjFileField.text = ""
                                            }
                                        }
                                    }

                                    // WKT字符串输入区域
                                    GridLayout {
                                        columns: 2
                                        rowSpacing: 10
                                        columnSpacing: 12
                                        width: parent.width
                                        enabled: coordinateSystemType === 2  // 根据单选按钮状态禁用/启用
                                        opacity: coordinateSystemType === 2 ? 1.0 : 0.5  // 视觉上的禁用效果
                                        Layout.leftMargin: 24  // 缩进，表示层级

                                        Label {
                                            text: "WKT字符串:"
                                            Layout.alignment: Qt.AlignTop
                                            font.pixelSize: 14
                                        }

                                        TextArea {
                                            id: wktTextField
                                            text: wktString
                                            Layout.fillWidth: true
                                            placeholderText: "请输入WKT坐标字符串"
                                            wrapMode: Text.Wrap
                                            onTextChanged: {
                                                if (coordinateSystemType === 2) {
                                                    wktString = text
                                                }
                                            }
                                            Layout.preferredHeight: 80
                                            selectByMouse: true

                                            background: Rectangle {
                                                radius: 4
                                                border.color: wktTextField.activeFocus && coordinateSystemType === 2 ? "#4285f4" : "#cccccc"
                                                border.width: wktTextField.activeFocus && coordinateSystemType === 2 ? 2 : 1
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Nodata值设置 - 修改RGB标签位置
                        GroupBox {
                            title: "Nodata值设置"
                            Layout.fillWidth: true

                            // 自定义标题样式
                            label: Label {
                                text: parent.title
                                font.bold: true  // 标题粗体
                                font.pixelSize: 14
                                padding: 6  // 增加标题上下padding
                            }

                            background: Rectangle {
                                color: "#ffffff"
                                border.color: "#cccccc"
                                radius: 6
                            }
                            leftPadding: 16  // 增加内容padding
                            topPadding:30

                            RowLayout {
                                spacing: 12
                                width: parent.width

                                Label {
                                    text: "RGB值:"
                                    font.pixelSize: 14
                                }

                                // 修改后的RGB输入框，标签与输入框在同一行
                                RowLayout {
                                    spacing: 8

                                    // R值输入框
                                    RowLayout {
                                        spacing: 4
                                        Label {
                                            text: "R"
                                            color: "#777777"
                                            font.pixelSize: 14
                                        }
                                        TextField {
                                            id: nodataFieldR
                                            text: nodata[0]
                                            validator: DoubleValidator {}
                                            onTextChanged: nodata[0] = parseFloat(text) || 0
                                            Layout.preferredWidth: 70
                                            horizontalAlignment: TextInput.AlignHCenter
                                            selectByMouse: true

                                            background: Rectangle {
                                                radius: 4
                                                border.color: nodataFieldR.activeFocus ? "#4285f4" : "#cccccc"
                                                border.width: nodataFieldR.activeFocus ? 2 : 1
                                            }
                                        }
                                    }

                                    // G值输入框
                                    RowLayout {
                                        spacing: 4
                                        Label {
                                            text: "G"
                                            color: "#777777"
                                            font.pixelSize: 14
                                        }
                                        TextField {
                                            id: nodataFieldG
                                            text: nodata[1]
                                            validator: DoubleValidator {}
                                            onTextChanged: nodata[1] = parseFloat(text) || 0
                                            Layout.preferredWidth: 70
                                            horizontalAlignment: TextInput.AlignHCenter
                                            selectByMouse: true

                                            background: Rectangle {
                                                radius: 4
                                                border.color: nodataFieldG.activeFocus ? "#4285f4" : "#cccccc"
                                                border.width: nodataFieldG.activeFocus ? 2 : 1
                                            }
                                        }
                                    }

                                    // B值输入框
                                    RowLayout {
                                        spacing: 4
                                        Label {
                                            text: "B"
                                            color: "#777777"
                                            font.pixelSize: 14
                                        }
                                        TextField {
                                            id: nodataFieldB
                                            text: nodata[2]
                                            validator: DoubleValidator {}
                                            onTextChanged: nodata[2] = parseFloat(text) || 0
                                            Layout.preferredWidth: 70
                                            horizontalAlignment: TextInput.AlignHCenter
                                            selectByMouse: true

                                            background: Rectangle {
                                                radius: 4
                                                border.color: nodataFieldB.activeFocus ? "#4285f4" : "#cccccc"
                                                border.width: nodataFieldB.activeFocus ? 2 : 1
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 进度条 - 美化
                ProgressBar {
                    value: progress
                    from: 0
                    to: 1
                    Layout.fillWidth: true
                    visible: progress > 0
                    indeterminate: progress === -1

                    background: Rectangle {
                        implicitWidth: 200
                        implicitHeight: 6
                        color: "#e0e0e0"
                        radius: 3
                    }

                    contentItem: Rectangle {
                        width: parent.width * parent.value
                        height: parent.height
                        radius: 3
                        color: "#4285f4"
                    }
                }

                // 按钮行 - 美化
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 12

                    Button {
                        id: helpButton
                        text: "帮助"
                        onClicked: helpPanelVisible = !helpPanelVisible
                        checkable: true
                        checked: helpPanelVisible

                        background: Rectangle {
                            radius: 4
                            color: helpPanelVisible ? "#4285f4" : (parent.down ? "#dadada" : parent.hovered ? "#e8e8e8" : "#f0f0f0")
                            border.color: helpPanelVisible ? "#3875d7" : "#cccccc"
                            border.width: 1
                        }

                        contentItem: Text {
                            text: parent.text
                            color: helpPanelVisible ? "white" : "#333333"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        text: "重置默认值"
                        onClicked: resetParameters()

                        background: Rectangle {
                            radius: 4
                            color: parent.down ? "#dadada" : parent.hovered ? "#e8e8e8" : "#f0f0f0"
                            border.color: "#cccccc"
                            border.width: 1
                        }

                        contentItem: Text {
                            text: parent.text
                            color: "#333333"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        text: "取消"
                        onClicked: {
                            rejected()
                            close()
                        }

                        background: Rectangle {
                            radius: 4
                            color: parent.down ? "#dadada" : parent.hovered ? "#e8e8e8" : "#f0f0f0"
                            border.color: "#cccccc"
                            border.width: 1
                        }

                        contentItem: Text {
                            text: parent.text
                            color: "#333333"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Button {
                        text: "确定"
                        highlighted: true
                        onClicked: {
                            accepted()
                            close()
                        }

                        background: Rectangle {
                            radius: 4
                            color: parent.down ? "#3875d7" : parent.hovered ? "#4285f4" : "#5c95f5"
                        }

                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }

        // 帮助面板 - 改为独立区域，右侧附加
        Rectangle {
            id: helpPanel
            anchors.left: mainSettingsContainer.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: helpPanelVisible ? 300 : 0
            color: "#f9f9f9"
            border.color: "#dddddd"
            border.width: helpPanelVisible ? 1 : 0

            // 宽度过渡动画
            Behavior on width {
                NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
            }

            // 帮助面板内容
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16
                opacity: helpPanelVisible ? 1 : 0

                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }

                Label {
                    text: "帮助信息"
                    font.bold: true
                    font.pixelSize: 16
                    color: "#333333"
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    clip: true

                    Label {
                        width: helpPanel.width - 40
                        wrapMode: Text.Wrap
                        text: "切片参数设置说明：\n\n" +
                              "文件设置：\n" +
                              "- 输入文件：选择需要切片的图像文件，支持TIF、PNG、JPG格式\n" +
                              "- 输出目录：指定切片结果保存的文件夹\n\n" +
                              "切片级别设置：\n" +
                              "- 级别范围：设置瓦片切片的缩放级别范围，级别越高细节越丰富，数据量也越大\n\n" +
                              "输出设置：\n" +
                              "- 输出格式：选择切片后的瓦片格式，PNG适合透明背景，JPG适合照片类影像，WEBP具有更高压缩率，TIF支持更多元数据\n\n" +
                              "高级选项：\n" +
                              "- 坐标系统设置：设置输出瓦片的坐标系统\n" +
                              "  * 使用影像自带坐标系统：勾选表示启用影像自带坐标系统，为默认设置\n" +
                              "  * 使用PRJ文件：通过PRJ文件定义坐标系统\n" +
                              "  * 使用WKT字符串：通过WKT文本字符串定义坐标系统\n" +
                              "- Nodata值设置：指定透明或无数据区域的RGB颜色值\n\n" +
                              "操作说明：\n" +
                              "- 完成参数设置后，点击\"确定\"开始切片处理\n" +
                              "- 切片过程中会显示进度条\n" +
                              "- 可以随时点击\"取消\"终止操作\n" +
                              "- 点击\"重置默认值\"恢复默认设置"
                        color: "#333333"
                        lineHeight: 1.4
                    }
                }

                Button {
                    text: "关闭帮助"
                    Layout.alignment: Qt.AlignRight
                    onClicked: helpPanelVisible = false

                    background: Rectangle {
                        radius: 4
                        color: parent.down ? "#dadada" : parent.hovered ? "#e8e8e8" : "#f0f0f0"
                        border.color: "#cccccc"
                        border.width: 1
                    }

                    contentItem: Text {
                        text: parent.text
                        color: "#333333"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }


    // 文件选择对话框
    FileDialog {
        id: fileDialog
        title: "选择输入文件"
        nameFilters: ["图像文件 (*.tif *.tiff *.png *.jpg)"]
        onAccepted: inputFile = selectedFile.toString().replace("file://", "")
    }

    FolderDialog {
        id: dirDialog
        title: "选择输出目录"
        onAccepted: outputDir = selectedFolder.toString().replace("file://", "")
    }

    FileDialog {
        id: prjFileDialog
        title: "选择PRJ文件"
        nameFilters: ["PRJ文件 (*.prj)"]
        onAccepted: prjFilePath = selectedFile.toString().replace("file://", "")
    }

    // 重置参数
    function resetParameters() {
        inputFile = ""
        outputDir = ""
        minLevel = 15
        maxLevel = 18
        nodata = [0.0, 0.0, 0.0]
        outputFormat = "png"
        prjFilePath = ""
        wktString = ""
        progress = 0
        useCoordinateSystem = true
        usePrj = true
        helpPanelVisible = false
    }
}
