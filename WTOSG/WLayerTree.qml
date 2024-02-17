import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import WTOSG

Item {
	id:root
	TreeView{
		id:modelTree
		anchors.fill:parent
		model:WLayerTreeModel
        boundsBehavior: Flickable.StopAtBounds
        boundsMovement: Flickable.StopAtBounds
		clip:true
		Component.onCompleted:modelTree.toggleExpanded(0)

		property int lastIndex:-1

		delegate:Item{
			id:treeDelegate
			implicitWidth:root.width>0 ? root.width : 260
			implicitHeight:30
			property int indentation:20
			property int leftMargin:4
			required property int checkState
			required property int foldState
			required property int indexState
			required property int index
			required property int depth

             // Assigned to by TreeView:
            required property TreeView treeView
            required property bool isTreeNode
            required property bool expanded
            required property int hasChildren
            required property int row
			required property int column
			
			Rectangle {
				anchors.fill:parent
                color: (treeDelegate.index === modelTree.lastIndex)
                    ? CommonDefines.selection
                    : (hoverHandler.hovered ? CommonDefines.active : "transparent")
            }

			Row{
				id:indicatorItem
				anchors.verticalCenter:parent.verticalCenter				
				x:treeDelegate.leftMargin+(treeDelegate.depth*treeDelegate.indentation)
				spacing:4
				Image{
					id: foldStateImage
					anchors.verticalCenter:parent.verticalCenter
					source:{
						if(0===treeDelegate.checkState) "qrc:/qt/qml/Waisting/icon/rectangle.png"
						else if(1===treeDelegate.checkState) "qrc:/qt/qml/Waisting/icon/clear-rectangle.png"
						else "qrc:/qt/qml/Waisting/icon/check-rectangle.png"
					}
					sourceSize.width:20
					sourceSize.height:20
					fillMode:Image.PreserveAspectFit
					smooth:true
					antialiasing:true
					asynchronous:true

					TapHandler{
						onTapped:{
							var modelIndex=treeDelegate.treeView.index(row,column)
							console.log("勾选")
							WLayerTreeModel.checkItem(modelIndex)
						}
					}
				}
				Image{
					id: fileTypeImage
					x:20
					anchors.verticalCenter:parent.verticalCenter
					source:{
						treeDelegate.hasChildren ? (treeDelegate.expanded
                                                    ? "qrc:/qt/qml/Waisting/icon/folder-open.png" : "qrc:/qt/qml/Waisting/icon/folder.png")
													: "qrc:/qt/qml/Waisting/icon/file.png"
					}
					sourceSize.width:20
					sourceSize.height:20
					fillMode:Image.PreserveAspectFit
					smooth:true
					antialiasing:true
					asynchronous:true
				}
				Text{
					text:model.name
					color:"#eeeeee"
				}
			}

			HoverHandler{
				id:hoverHandler
			}

			TapHandler{
				acceptedButtons:Qt.LeftButton|Qt.RightButton
                onSingleTapped:(evetPoint,button)=>{
                    switch(button){
                        case Qt.LeftButton:
							console.log("单点")
                            if(treeDelegate.hasChildren){
								modelTree.toggleExpanded(treeDelegate.row)
                            }
                        break;
                        case Qt.RightButton:
                            if(treeDelegate.hasChildren)
                                contextMenu.popup()
                        break;
                    }
                }
				onDoubleTapped:(eventPoint,button)=>{
					switch(button){
						case Qt.LeftButton:
							console.log("双击")
							if(!treeDelegate.hasChildren){
								var modelIndex=treeDelegate.treeView.index(row,column)
								WLayerTreeModel.zoomToItem(modelIndex);
								WLayerTreeModel.testAdd();
							}
						break;
					}
				}
			}
			WLayerTreeMenu{
				id:contextMenu
				Action{
					text:qsTr("缩放到目标")
					onTriggered:{
						var modelIndex=treeDelegate.treeView.index(row,column)
						modelTreeView.zoomToItem(modelIndex);
					}
				}
				Action{
					text:qsTr("rest root")
					onTriggered:{
						modelTreeView=-1
					}
				}
			}
		}
	}
}
