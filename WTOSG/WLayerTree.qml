import QtQuick
import QtQuick.Controls
Item {
    id:root
    TreeView{
        id:testTree
        anchors.fill: parent
        model: WLayerTreeModel
        boundsBehavior: Flickable.StopAtBounds
        boundsMovement: Flickable.StopAtBounds
        clip: true
        Component.onCompleted: testTree.toggleExpanded(0)

        delegate: Item {
            id: treeDelegate
            implicitWidth: padding + label.x + label.implicitWidth + padding
            implicitHeight: label.implicitHeight * 1.5

            readonly property real indent: 20
            readonly property real padding: 5

            // Assigned to by TreeView:
            required property TreeView treeView
            required property bool isTreeNode
            required property bool expanded
            required property int hasChildren
            required property int depth
            required property int checkState

            TapHandler {
                onTapped: treeView.toggleExpanded(row)
            }

            Image {
                id: image
                x: (treeDelegate.isTreeNode ? (treeDelegate.depth + 1) * treeDelegate.indent : 0)-25
                anchors.verticalCenter: parent.verticalCenter
                source: treeDelegate.hasChildren ? (treeDelegate.expanded
                                                    ? "qrc:/qt/qml/Waisting/icon/cangku.png" : "qrc:/qt/qml/Waisting/icon/caiwu.png")
                                                : "qrc:/qt/qml/Waisting/icon/dakaiwenjian1.png"
                width: 20
                height: 20
                fillMode: Image.PreserveAspectFit
                smooth: true
                antialiasing: true
                asynchronous: true
            }

            // CheckBox{
            //     id:checkbox
            // }

            CheckBox {
                id: label
                tristate:true
                x: padding + (treeDelegate.isTreeNode ? (treeDelegate.depth + 1) * treeDelegate.indent : 0)
                width: treeDelegate.width - treeDelegate.padding - x
                clip: true
                text: model.name
                anchors.verticalCenter: parent.verticalCenter
//                checkState: {
////                    var mi = treeDelegate.treeView.index(row, column)
//                    treeView.model.itemChecked(row,column)
//                }
                checkState: treeDelegate.checkState//model.checked
            }
        }
    }
}
