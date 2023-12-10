//数据处理页面的功能按钮
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs


import WTComponents
Item {
    id:root

    FileDialog {
         id: fileDialog
         title: "选择一个文件"
         nameFilters: ["osgb (*.osgb)", "其他三维模型 (*.*)"]
         currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
         onAccepted: {
             globe_FileObject.selectedPath=selectedFile
             globe_FileObject.handleType=WFileObject.OPENFILE
             globe_FileObject.handleFile()
         }
     }

    Flow{
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10
        WImageLabelButton{
            width:60
            height:62
            label:"打开osgb文件"
            source: "qrc:/qt/qml/Waisting/icon/dakaiwenjian1.png"
            onClicked:{
                fileDialog.open()
            }
        }
        WImageLabelButton{
            width:60
            height:62
            label:"lmw"
            source: "qrc:/qt/qml/Waisting/icon/banjia.png"
        }
        WImageLabelButton{
            width:60
            height:62
            label:"lmw"
            source: "qrc:/qt/qml/Waisting/icon/caiwu.png"
        }
        WImageLabelButton{
            width:60
            height:62
            label:"lmw"
            source: "qrc:/qt/qml/Waisting/icon/jizhang.png"
        }
        WImageLabelButton{
            width:60
            height:62
            label:"lmw"
            source: "qrc:/qt/qml/Waisting/icon/weixiu.png"
        }
        WImageLabelButton{
            width:60
            height:62
            label:"lmw"
            source: "qrc:/qt/qml/Waisting/icon/yagnhu.png"
        }
        WImageLabelButton{
            width:60
            height:62
            label:"lmw"
            source: "qrc:/qt/qml/Waisting/icon/kaifa.png"
        }
        WImageLabelButton{
            width:60
            height:62
            label:"lmw"
            source: "qrc:/qt/qml/Waisting/icon/zhuanli.png"
        }
    }
}
