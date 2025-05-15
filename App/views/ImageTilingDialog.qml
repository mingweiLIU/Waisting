import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Window {
    id: paramDialog
    title: "切片参数设置"
    width: 550
    height: advancedOptionsVisible ? 90 : 520
    flags: Qt.Dialog  // 设置为对话框样式
    color: "#f5f5f5"  // 设置背景色

    // 添加窗口大小改变动画
    Behavior on height {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }

    // 参数属性
    property string inputFile: ""
    property string outputDir: ""
    property int minLevel: 15
    property int maxLevel: 18
    property var nodata: [0.0, 0.0, 0.0]
    property string outputFormat: "png"
    property string prjFilePath: ""
    property string wktString: ""
    property real progress: 0
    property bool advancedOptionsVisible: false
    property bool usePrj: true  // 新增：控制使用PRJ还是WKT

    signal accepted()
    signal rejected()

    // 主布局
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

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
            padding: 16  // 增加内容padding

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
            padding: 16  // 增加内容padding

            RowLayout {
                spacing: 20
                width: parent.width

                Label {
                    text: "级别范围:"
                    Layout.alignment: Qt.AlignVCenter
                    font.pixelSize: 14
                }

                // 修改SpinBox，增加宽度以显示数字，设置合理的步长
                SpinBox {
                    id: minLevelBox
                    value: minLevel
                    from: 0
                    to: maxLevel
                    onValueChanged: minLevel = value
                    Layout.preferredWidth: 120
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
                        border.color: "#cccccc"
                        border.width: 1
                        radius: 4
                    }
                }

                Label {
                    text: "到"
                    font.pixelSize: 14
                }

                SpinBox {
                    id: maxLevelBox
                    value: maxLevel
                    from: minLevel
                    to: 25
                    onValueChanged: maxLevel = value
                    Layout.preferredWidth: 120
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
                        border.color: "#cccccc"
                        border.width: 1
                        radius: 4
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
            padding: 16  // 增加内容padding

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
                    Layout.preferredWidth: 180

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

        // 高级选项内容 - 优化高度和排版
        ColumnLayout {
            id: advancedOptions
            Layout.fillWidth: true
            spacing: 16
            visible: advancedOptionsVisible
            opacity: advancedOptionsVisible ? 1.0 : 0

            // 添加动画
            Behavior on opacity {
                NumberAnimation { duration: 200 }
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
                padding: 16  // 增加内容padding

                ColumnLayout {
                    width: parent.width
                    spacing: 12

                    // 添加单选按钮组
                    RowLayout {
                        spacing: 20

                        RadioButton {
                            id: prjRadioButton
                            text: "使用PRJ文件"
                            checked: usePrj
                            onCheckedChanged: {
                                if (checked) {
                                    usePrj = true
                                    wktTextField.text = ""  // 清空WKT
                                }
                            }
                        }

                        RadioButton {
                            id: wktRadioButton
                            text: "使用WKT字符串"
                            checked: !usePrj
                            onCheckedChanged: {
                                if (checked) {
                                    usePrj = false
                                    prjFilePath = ""  // 清空PRJ文件路径
                                    prjFileField.text = ""
                                }
                            }
                        }
                    }

                    // PRJ文件选择区域
                    GridLayout {
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 12
                        width: parent.width
                        enabled: usePrj  // 根据单选按钮状态禁用/启用
                        opacity: usePrj ? 1.0 : 0.5  // 视觉上的禁用效果

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
                                    if (usePrj) {
                                        prjFilePath = text
                                    }
                                }
                                selectByMouse: true
                                background: Rectangle {
                                    radius: 4
                                    border.color: prjFileField.activeFocus && usePrj ? "#4285f4" : "#cccccc"
                                    border.width: prjFileField.activeFocus && usePrj ? 2 : 1
                                }
                            }
                            Button {
                                text: "浏览"
                                onClicked: {
                                    if (usePrj) {
                                        prjFileDialog.open()
                                    }
                                }
                                background: Rectangle {
                                    radius: 4
                                    color: parent.down && usePrj ? "#3875d7" : parent.hovered && usePrj ? "#4285f4" : usePrj ? "#5c95f5" : "#aaaaaa"
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

                    // WKT字符串输入区域
                    GridLayout {
                        columns: 2
                        rowSpacing: 10
                        columnSpacing: 12
                        width: parent.width
                        enabled: !usePrj  // 根据单选按钮状态禁用/启用
                        opacity: !usePrj ? 1.0 : 0.5  // 视觉上的禁用效果

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
                                if (!usePrj) {
                                    wktString = text
                                }
                            }
                            Layout.preferredHeight: 80
                            selectByMouse: true

                            background: Rectangle {
                                radius: 4
                                border.color: wktTextField.activeFocus && !usePrj ? "#4285f4" : "#cccccc"
                                border.width: wktTextField.activeFocus && !usePrj ? 2 : 1
                            }
                        }
                    }
                }
            }

            // Nodata值设置
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
                padding: 16  // 增加内容padding

                RowLayout {
                    spacing: 12
                    width: parent.width

                    Label {
                        text: "RGB值:"
                        font.pixelSize: 14
                    }

                    Repeater {
                        model: 3
                        TextField {
                            id: nodataField
                            text: nodata[index]
                            validator: DoubleValidator {}
                            onTextChanged: nodata[index] = parseFloat(text) || 0
                            Layout.preferredWidth: 80
                            horizontalAlignment: TextInput.AlignHCenter
                            selectByMouse: true

                            background: Rectangle {
                                radius: 4
                                border.color: nodataField.activeFocus ? "#4285f4" : "#cccccc"
                                border.width: nodataField.activeFocus ? 2 : 1
                            }

                            // 添加RGB标签
                            Label {
                                text: index === 0 ? "R" : index === 1 ? "G" : "B"
                                color: "#777777"
                                anchors.top: parent.top
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                                anchors.topMargin: -12
                                font.pixelSize: 12
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
        usePrj = true  // 重置为使用PRJ文件
    }
}
