cmake_minimum_required(VERSION 3.16)

project(Waisting VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(./cmake/atom.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/exe/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/exe/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/exe/lib)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.5)

set(ThirdPartyPath ${CMAKE_CURRENT_SOURCE_DIR}/3rd)
set(OSG_DIR ${ThirdPartyPath}/osg)

set(SubDirs
    App
    WTComponents
    WTTools
    WTOSG
)

#全局设置
link_directories(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
foreach(oneDir ${SubDirs} )
    include_directories(${oneDir})
endforeach( )


foreach(oneDir ${SubDirs} )
    add_subdirectory(${oneDir})
endforeach( )
