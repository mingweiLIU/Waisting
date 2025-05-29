cmake_minimum_required(VERSION 3.5)
project (Test VERSION 1.0 LANGUAGES CXX)
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


set(includeDirs 
    # ${OPENSCENEGRAPH_INCLUDE_DIRS}
    # ${osgEarth_INCLUDE_DIRS}
    ${GDAL_INCLUDE_DIRS}
    ${TBB_INCLUDE_DIRS}

    # ${GeographicLib_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}  ${SourceFileNames} ${HeaderFileNames})
target_include_directories(${PROJECT_NAME}  PUBLIC ${includeDirs} ${HeaderFileNames})
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${GDAL_LIBRARY}
    TBB::tbb
    glm::glm
    WTDataManipulate
    WTFrame
    WTEarth
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

