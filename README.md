<!--
 * @Description: file content
 * @version: 0.0.1
 * @Author: 刘铭崴
 * @Date: 2020-08-17 20:50:57
 * @LastEditors: 刘铭崴
 * @LastEditTime: 2023-02-27 14:24:24
-->

简介
------
Waisting是一个闲得没事边学边整的三维GIS数据处理和场景编辑工具，其采用Qt6 QML作为UI框架，采用CMake作为编译工具

# 功能计划
## OSGB数据处理
- 裁剪
- 整平
- 提取点云
- 生成DSM
- 生成TDOM
- 全局与局部颜色调整
- 纹理替换
- 水面等破洞修补
- 全球网格剖分
- 粗糙层生成
- 与地形融合
## 3D Gauss 
- Colmap sfm
- 3D gauss 模型生成
- 3D gauss 模型查看
## 数据处理
- obj模型转3DTiles
- shp转3DTiles
- 影像切片
- 地形切片
- 三维管网建模
## 信息提取
- 房屋轮廓提取
- 白膜提取
- 纹理自动映射
## 数据采集
- 分层分户采集
## 格式转换
- 3DTiles
- GLTF
- I3S
- OBJ
- FBX
## 数据导入
- shp
- obj
- fbx
- osgb
- 影像
- 地形
- wms/wmts/tms
## 场景编辑
- 模型旋转
- 模型平移
- 模型缩放
- POI设置
- 文字标注
- 三维标绘
## 动画制作
- 按路径漫游
- 帧缓存漫游
- 视频输出
## 三维PPT制作
- 文字设置
- 时间设置
- 模型动画

## 场景设置
- 灯光
- 天气
- 视频流输出

# 代码说明
## 第三方依赖库
本代码中已将第三方库3rd文件夹过滤，编译时需要按照需要将3rd库下载放置到项目文件夹，目前依赖
- Qt6

## 工程说明
- App 为本项目中可以运行的应用
- WTDataManipulate 为数据处理库
- WTEarth 为地球相关的内容
- WTFrame 为框架性东西 如日志、错误信息、
- WTRender 为渲染相关
