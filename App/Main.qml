import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs

import "./components"
import "./views"

Window {
    id:mainAppWindow
    width: 800
    height: 500
    title: "QDS"
    visible: true
    visibility: Window.Maximized

     property var tileParamDialog: null

    //定义一些事件
    signal hideModal()   // 请求隐藏矩形的信号
    signal showModal()   // 请求显示矩形的信号
    // signal showInfo(string info,string title)    // 显示弹窗

    Connections{
        target:mainAppWindow
        function onHideModal(){ mainOverlay.visible = false;}
        function onShowModal(){ mainOverlay.visible = true;}
        // function onShowInfo(info,title){
        //     infoDialog.showConfirmation(info,title)
        // }
    }

    // 半透明黑色遮罩（变暗背景） 给弹出框用的
    Rectangle {
        id: mainOverlay
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.6) // 50%透明黑色
        visible: false
        z: 10 // 确保遮罩在最上层
        // 点击遮罩不关闭弹窗（增强模态行为）
        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }
    }

    //信息弹出窗
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

    Ribbon{
        id: mainRibbon
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        // 组件初始化完成后添加标签页
        Component.onCompleted: {
            // 创建第一个标签页及其内容
            let tab1Content = Qt.createComponent("./components/RibbonTab.qml").createObject();
            let tab1Index = addTab("文件", tab1Content);

            // 添加面板到第一个标签页
            let filePanel = tab1Content.addPanel("文件操作");
            filePanel.addButton("qrc:/qt/qml/Waisting/icon/add-rectangle.png", "新建", function() {
                console.log("新建文件");
            });
            filePanel.addButton("qrc:/qt/qml/Waisting/icon/max.png", "打开", function() {
                console.log("打开文件");
            });
            filePanel.addButton("qrc:/qt/qml/Waisting/icon/caiwu.png", "保存", function() {
                console.log("保存文件");
            });

            let dataHandlerPanel = tab1Content.addPanel("数据轻量化处理");
            dataHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/shitujuzhen.png", "影像切片", function() {
                // let imageTilerUI =Qt.createComponent("./views/ImageTilingDialog.qml").createObject(mainAppWindow,{"mainWindow":mainAppWindow});
                // // mainOverlay.visible=true;
                // imageTilerUI.show();
                if (!tileParamDialog) {
                    var component = Qt.createComponent("./views/ImageTilingDialog.qml");
                    if (component.status === Component.Ready) {
                        tileParamDialog = component.createObject(mainAppWindow, {
                            "mainWindow": mainAppWindow,
                            "x": (mainAppWindow.width - 550) / 2,
                            "y": (mainAppWindow.height - 500) / 2
                        });

                        // 连接信号
                        tileParamDialog.accepted.connect(function() {
                            console.log("对话框已接受");
                            // 这里可以处理对话框的返回值
                        });

                        tileParamDialog.rejected.connect(function() {
                            console.log("对话框已取消");
                        });
                    }
                }

                if (tileParamDialog) {
                    tileParamDialog.open();
                }
            });
            dataHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/yingyongAPP.png", "STK地形切片", function() {
                console.log("STK地形切片");
            });
            dataHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/banjia.png", "矢量转3DTiles", function() {
                console.log("矢量转3DTiles");
            });
            dataHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/baoguo_hezi.png", "OSGB转3DTiles", function() {
                console.log("OSGB转3DTiles");
            });
            dataHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/dakaiwenjian1.png", "离散OBJ转3DTiles", function() {
                console.log("离散OBJ转3DTiles");
            });

            // let importPanel = tab1Content.addPanel("导入数据");
            // importPanel.addButton("qrc:/qt/qml/Waisting/icon/tupian.png", "导入影像", function() {
            //     console.log("导入影像");
            // });
            // importPanel.addButton("qrc:/qt/qml/Waisting/icon/X1/Mesh Derivative.png", "导入地形", function() {
            //     console.log("导入地形");
            // });
            // importPanel.addButton("qrc:/qt/qml/Waisting/icon/cangku.png", "导入三维模型", function() {
            //     console.log("导入三维模型");
            // });

            let osgbHandlerPanel = tab1Content.addPanel("osgb数据处理");
            osgbHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/X1/Mesh To Points.png", "提取点云", function() {
                console.log("OSGB提取点云");
            });
            osgbHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/X1/Regular Terrain Triangulation.png", "生成DSM", function() {
                console.log("OSGB生成DSM");
            });
            osgbHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/X1/Quad Viewports.png", "生成TDOM", function() {
                console.log("OSGB生成TDOM");
            });
            osgbHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/cengji.png", "粗糙层生成", function() {
                console.log("OSGB粗糙层生成");
            });
            osgbHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/X1/Swap Creases _ Edges.png", "与地形融合", function() {
                console.log("OSGB与地形融合");
            });
            osgbHandlerPanel.addButton("qrc:/qt/qml/Waisting/icon/X1/Volume Rendering.png", "颜色调整", function() {
                console.log("OSGB颜色调整");
            });

            //参数化建模
            let paramModelingTabContent = Qt.createComponent("./components/RibbonTab.qml").createObject();
            addTab("参数化建模", paramModelingTabContent);

            // 管网参数化建模
            let pipePanel = paramModelingTabContent.addPanel("管网参数化建模");
            pipePanel.addButton("qrc:/qt/qml/Waisting/icon/baoguo_hezi.png", "配置管网数据", function() {
                console.log("配置管网数据");
            });
            pipePanel.addButton("qrc:/qt/qml/Waisting/icon/duoxuanxuanzhong.png", "参数设置", function() {
                console.log("管网参数设置");
            });
            pipePanel.addButton("qrc:/qt/qml/Waisting/icon/X1/Stitch two Holes.png", "管网建模", function() {
                console.log("管网建模");
            });
            // 道路参数化建模
            let roadPanel = paramModelingTabContent.addPanel("道路参数化建模");
            roadPanel.addButton("qrc:/qt/qml/Waisting/icon/X1/Voxels Path.png", "配置路网数据", function() {
                console.log("配置路网数据");
            });
            roadPanel.addButton("qrc:/qt/qml/Waisting/icon/lianjie.png", "参数设置", function() {
                console.log("道路参数设置");
            });
            roadPanel.addButton("qrc:/qt/qml/Waisting/icon/X1/Stitch two Holes.png", "道路建模", function() {
                console.log("道路建模");
            });
            // 风机参数化建模
            let windmillPanel = paramModelingTabContent.addPanel("风机参数化建模");
            windmillPanel.addButton("qrc:/qt/qml/Waisting/icon/gongzuoliu.png", "风机断面数据", function() {
                console.log("配置风机断面数据");
            });
            windmillPanel.addButton("qrc:/qt/qml/Waisting/icon/gongzuoliu.png", "参数设置", function() {
                console.log("风机参数设置");
            });
            windmillPanel.addButton("qrc:/qt/qml/Waisting/icon/fuwuguanli.png", "风机建模", function() {
                console.log("风机建模");
            });


            // 创建第一个标签页及其内容
            let systemContent = Qt.createComponent("./components/RibbonTab.qml").createObject();
            let systemIndex = addTab("系统", systemContent);


            let systemPanel = systemContent.addPanel("系统设置");
            systemPanel.addButton("qrc:/qt/qml/Waisting/icon/guanyu.png", "关于", function() {
                console.log("关于系统");
            });

            // 设置默认选中第一个标签页
            checkTab(0);
        }
    }
    RowLayout{
        anchors{
            top:mainRibbon.bottom
            left: parent.left
            right: parent.right
            bottom:parent.bottom
        }
        spacing:0

        //创建下面的部分
        SplitView{
            Layout.fillWidth:true
            Layout.fillHeight:true
            handle:Rectangle{
                implicitWidth:2
                color:SplitHandle.pressed? "#417bbf" : "#3f4347"
                Behavior on opacity{
                    OpacityAnimator{
                        duration:400
                    }
                }
            }

            //左侧面板
            Rectangle{
                id:leftPanel
                SplitView.minimumWidth:350
                SplitView.fillHeight:true

                RowLayout{
                    anchors.fill:parent
                    spacing:0

                    //最左侧的菜单栏
                    Rectangle{
                        id:mostLeftMenuBar
                        width:50
                        Layout.fillHeight:true
                        color:"#2b2d30"
                        border.color:"#1e1f22"
                    }
                    //图层树等

                    Rectangle{
                        id:mostLeftMenuBarView
                        Layout.fillWidth:true
                        Layout.fillHeight:true
                        color:"#2b2d30"
                        border.color:"#1e1f22"
                    }
                    }
            }

            //主视图
            Rectangle{
                id:mainViewer
                color:"#1e1f22"
                SplitView.fillWidth:true
                SplitView.fillHeight:true
            }
        }
    }
}
