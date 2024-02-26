set(modelName WTOSG)

WTGetTypeFilesRelative(
    FileType "*.qml"
    OutRelative QMLFileNames
    Recurse OFF
)

#设置单例的qml文件
set_source_files_properties(CommonDefines.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

WTGetTypeFilesRelative(
    FileType "*.h"
    OutRelative HeaderFileNames
    Recurse OFF
)
WTGetTypeFilesRelative(
    FileType "*.cpp"
    OutRelative SourceFileNames
    Recurse OFF
)

#如果有js文件
# file(GLOB JSFiles "*.js")
# set(JSFileNames )
# GetRelativePath(
#     AbsPath ${CMAKE_CURRENT_SOURCE_DIR}/        #因为是路径 所以把多的/符去掉
#     ListPath ${QMLFiles}
#     OutRelative JSFileNames
# )
# set_source_files_properties(${JSFileNames} PROPERTIES
#     QT_QML_SINGLETON_TYPE TRUE
# )
#end 如果有js文件

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick)
qt_standard_project_setup(REQUIRES 6.5)

find_package(OpenSceneGraph REQUIRED osg osgDB osgViewer osgUtil osgGA )
find_package(OSGEarth REQUIRED osgearth)

set(includeDirs 
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
    ${OSGEARTH_INCLUDE_DIRS}
)

qt_add_library(${modelName} STATIC
    WLayerTree.qml
    CommonDefines.qml
    WLayerTreeMenu.qml
)
set_target_properties(${modelName} PROPERTIES AUTOMOC ON)
target_include_directories(${modelName} PUBLIC ${includeDirs})
target_link_libraries(${modelName} PRIVATE 
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    ${OPENSCENEGRAPH_LIBRARIES}
    ${OSGEARTH_LIBRARY}
    WTTools.lib
)

qt_add_qml_module(${modelName}
    URI ${modelName}
    VERSION 1.0
    QML_FILES ${QMLFileNames}

    SOURCES
    ${HeaderFileNames} ${SourceFileNames}
    #如果有资源
    # RESOURCE_PREFIX /wt
    # RESOURCES ${RESLIST}  #后面引用的使用"qrc:/wt/WTComponents/文件路径" 其实就是"qrc:/RESOURCE_PREFIX/URI/文件路径
)

include(GNUInstallDirs)
install(TARGETS ${modelName}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

