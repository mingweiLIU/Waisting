//数据处理页面的功能按钮
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs


import WTComponents
Item {
    id:root

    //opentype：modelFile=====打开模型文件 如osgb
    //          osgbGroup=====打开倾斜摄影测量模型
    signal selectedFileEvent(string filePath,string opentype)

    FileDialog {
         id: fileDialog
         title: "选择一个文件"
         nameFilters: ["osgb (*.osgb)", "其他三维模型 (*.*)"]
         currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
         onAccepted: {
             var filePath=JSON.stringify(selectedFile)
             selectedFileEvent(JSON.parse(filePath.replace("file:///","")),"modelFile")//去掉"file:///"
         }
    }

    FolderDialog{
        id:folderDialog
        title:"选择需要打开的文件夹"
        currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
        onAccepted: {
            var folderPath=JSON.stringify(currentFolder)
            selectedFileEvent(JSON.parse(folderPath.replace("file:///","")) ,"osgbGroup")
        }
    }

    Flow{
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10
        WImageLabelButton{
            width:60
            height:62
            label:"加载模型"
            source: "qrc:/qt/qml/Waisting/icon/dakaiwenjian1.png"
            onClicked:{
                fileDialog.open()
            }
        }
        WImageLabelButton{
            width:60
            height:62
            label:"加载osgb组"
            source: "qrc:/qt/qml/Waisting/icon/banjia.png"
            onClicked:{
                folderDialog.open()
            }
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

    Component.onCompleted: {
        selectedFileEvent.connect(wtOSGViewer.slot_loadFile)
    }
}
