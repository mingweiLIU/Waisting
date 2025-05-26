cmake_minimum_required(VERSION 3.16)

project(Waisting VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gltfSDK/Modules")
include(atom)
include(GLTFPlatform)

# include(FindOsgEarth)#如何控制debug引用的dll文件？？



# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/exe/bin)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/exed/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/exe/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/exe/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/exe/lib)

#设置Assimp的东西
#如果更新了assimp记得修改assimp的
#ASSIMP_BUILD_DRACO ON
#BUILD_SHARED_LIBS OFF
#ASSIMP_BUILD_TESTS OFF
#ASSIMP_INSTALL OFF
#end设置Assimp的东西

find_package(Qt6 6.9 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.9)

set(ThirdPartyPath ${CMAKE_CURRENT_SOURCE_DIR}/3rd)
set(OSG_DIR ${ThirdPartyPath}/osg)
set(osgEarth_DIR ${ThirdPartyPath}/osg/cmake)
set(GeographicLib_DIR ${ThirdPartyPath}/GeographicLib2.3/cmake/GeographicLib)
set(GDAL_ROOT ${ThirdPartyPath}/gdal)                                               #空间数据处理
set(RapidJson_Inc ${ThirdPartyPath}/RapidJSON/include)                              #json处理
set(taskflow_Inc ${ThirdPartyPath}/taskflow)                                        #图流程
set(ASSIMP_DIR ${ThirdPartyPath}/assimp/lib/cmake/assimp-5.4)                       #模型格式
set(SPDLOG_Inc ${ThirdPartyPath}/spdlog/include)
set(TBB_ROOT ${ThirdPartyPath}/tbb/lib/cmake/TBB)
set(GLM_ROOT ${ThirdPartyPath}/glm/share/glm)
set(Jemolloc_ROOT ${ThirdPartyPath}/jemalloc)
set(PNG_ROOT ${ThirdPartyPath}/libpng)
set(ZLIB_ROOT ${ThirdPartyPath}/zlib)


set(SubDirs
    # WTComponents
    App
    WTDataManipulate
    WTEarth
    WTFrame
    Test
    # WTUlits
    # WTOSG
    # WTTranscoder
    # GLTFSDK
    # assimp-5.4.0
)

#全局设置
# 强制关闭优化（适用于所有编译器）
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
        "$<$<C_COMPILER_ID:MSVC>:/Od>"          # MSVC: 禁用优化
        "$<$<NOT:$<C_COMPILER_ID:MSVC>>:-O0>"   # GCC/Clang: -O0
    )
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc  /Ob2")
link_directories(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
foreach(oneDir ${SubDirs} )
    include_directories(${oneDir})
endforeach( )
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${SPDLOG_Inc} ${ThirdPartyPath})

foreach(oneDir ${SubDirs} )
    add_subdirectory(${oneDir})
endforeach( )