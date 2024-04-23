cmake_minimum_required(VERSION 3.5)
project (WTFrame VERSION 1.0 LANGUAGES CXX)
add_compile_definitions(WTFRAME_Export)

set(CMAKE_CXX_STANDARD 11)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

WTGetTypeFilesRelative(
    FileType "*.h"
    OutRelative HeaderFileNames
    Recurse ON
)
WTGetTypeFilesRelative(
    FileType "*.cpp"
    OutRelative SourceFileNames
    Recurse ON
)

add_library(${PROJECT_NAME} SHARED ${SourceFileNames} ${HeaderFileNames}
    wtFrameDefines.h)
target_include_directories(${PROJECT_NAME} PUBLIC ${HeaderFileNames})

target_link_libraries(${PROJECT_NAME} Threads::Threads)

include(GNUInstallDirs)
install(TARGETS ${modelName}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
