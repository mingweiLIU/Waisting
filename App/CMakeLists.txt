cmake_minimum_required(VERSION 3.16)

project(Waisting VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)


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

WTGetTypeFilesRelative(
    FileType "*.png"
    OutRelative PNGFileNames
    Recurse ON
)

WTGetTypeFilesRelative(
    FileType "*.svg"
    OutRelative SVGFileNames
    Recurse ON
)

qt_add_executable(appWaisting
    ${HeaderFileNames} ${SourceFileNames}
)

qt_add_qml_module(appWaisting
    URI Waisting
    VERSION 1.0
    QML_FILES MainFrameless.qml
    RESOURCES qtquickcontrols2.conf
    RESOURCES ${PNGFileNames}
    RESOURCES ${SVGFileNames}
    SOURCES ${HeaderFileNames} ${SourceFileNames}
    QML_FILES Main.qml
    QML_FILES DataHandlerTool.qml
    SOURCES QImage2QMLProvider.h QImage2QMLProvider.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appWaisting PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appWaisting
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE NO
)

#如果是Qt的组件 修改这儿
set(SubDirs
    WTComponents
    WTOSG
)
#如果是纯C++组件 修改这儿
set(PureCXXLib
    WTTools
)

set(SelfPlugins)
set(PluginInclude)
foreach(oneDir ${SubDirs} )
    LIST(APPEND SelfPlugins ${oneDir}plugin)
    LIST(APPEND PluginInclude ../${oneDir})
endforeach()

foreach(oneDir ${PureCXXLib} )
    LIST(APPEND SelfPlugins ${oneDir})
    LIST(APPEND PluginInclude ../${oneDir})
endforeach()

target_link_libraries(appWaisting
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    ${SelfPlugins}
)
find_package(OpenSceneGraph REQUIRED osg osgDB osgViewer osgUtil osgGA )

set(includeDirs
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
)

target_include_directories(appWaisting PUBLIC ${PluginInclude} ${includeDirs})

include(GNUInstallDirs)
install(TARGETS appWaisting
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

