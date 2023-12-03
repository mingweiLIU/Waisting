cmake_minimum_required(VERSION 3.16)

set(modelName WTComponents)

WTGetTypeFilesRelative(
    FileType "*.qml"
    OutRelative QMLFileNames
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

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_library(${modelName} STATIC)
set_target_properties(${modelName} PROPERTIES AUTOMOC ON)
target_link_libraries(${modelName} PRIVATE Qt6::Quick)

qt_add_qml_module(${modelName}
    URI ${modelName}
    VERSION 1.0
    QML_FILES ${QMLFileNames}

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

