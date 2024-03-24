cmake_minimum_required(VERSION 3.5)
project (WTTranscoder VERSION 1.0 LANGUAGES CXX)
add_compile_definitions(WTTranscoder_Export)

set(CMAKE_CXX_STANDARD 11)

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

find_package(ASSIMP REQUIRED)

add_library(${PROJECT_NAME} SHARED ${SourceFileNames})
target_include_directories(${PROJECT_NAME} PUBLIC ${HeaderFileNames} ${ASSIMP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${ASSIMP_LIBRARIES})

include(GNUInstallDirs)
install(TARGETS ${modelName}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)