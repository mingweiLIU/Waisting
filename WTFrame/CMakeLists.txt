cmake_minimum_required(VERSION 3.5)
project (WTFrame VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

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

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

find_package(GDAL REQUIRED gdal)
find_package(TBB REQUIRED)
find_package(GLM REQUIRED)
find_package(PNG REQUIRED)

set(ZLIB_USE_STATIC_LIBS ON)
find_package(ZLIB REQUIRED)

set(includeDirs 
    # ${OPENSCENEGRAPH_INCLUDE_DIRS}
    # ${osgEarth_INCLUDE_DIRS}
    ${GDAL_INCLUDE_DIRS}
    ${TBB_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${SPDLOG_Inc}    
    ${Jemolloc_ROOT}/include
    ${Jemolloc_ROOT}/include/msvc_compat
    # ${GeographicLib_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} STATIC ${SourceFileNames} ${HeaderFileNames})
target_include_directories(${PROJECT_NAME}  PUBLIC ${includeDirs} ${HeaderFileNames})
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${GDAL_LIBRARY}
    TBB::tbb
    glm::glm
    ${Jemolloc_ROOT}/lib/jemalloc_sd.lib
    ${GDAL_ROOT}/lib/libjpeg.lib
    ${PNG_LIBRARY}
    ${ZLIB_LIBRARY}
)

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        JEMALLOC_EXPORT=
        JEMALLOC_STATIC
        PNG_STATIC
        ZLIB_STATIC
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

