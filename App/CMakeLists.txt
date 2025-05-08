cmake_minimum_required(VERSION 3.16)

project(Waisting VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

qt_policy(SET QTP0004 NEW)


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

WTGetTypeFilesRelative(
    FileType "*.frag.qsb"
    OutRelative SHADERFileNames
    Recurse ON
)

WTGetTypeFilesRelative(
    FileType "*.qml"
    OutRelative QMLFileNames
    Recurse ON
)

qt_add_executable(${PROJECT_NAME}
    ${HeaderFileNames} ${SourceFileNames}
)

qt_add_qml_module(${PROJECT_NAME}
    URI Waisting
    VERSION 1.0
    RESOURCES qtquickcontrols2.conf
    RESOURCES ${PNGFileNames}
    RESOURCES ${SVGFileNames} ${SHADERFileNames}
    SOURCES ${HeaderFileNames} ${SourceFileNames}
    QML_FILES ${QMLFileNames}
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(${PROJECT_NAME} PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE NO
)

#如果是Qt的组件 修改这儿
set(SubDirs
    #WTComponents
    # WTOSG
)
#如果是纯C++组件 修改这儿
set(PureCXXLib
    # WTTools
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

target_link_libraries(${PROJECT_NAME}
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

target_include_directories(${PROJECT_NAME} PUBLIC ${PluginInclude} ${includeDirs})

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

