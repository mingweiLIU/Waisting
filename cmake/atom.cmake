#计算一个路径中去掉某个前置路径后得得到得值，比如有个list:[D:/a/12.x;D:/a/23.x] 前置路径为：D:/a/ 则返回【12.x 23.x】
#示例：获取一个文件夹下所有得qml文件名
# file(GLOB QMLFiles "*.qml")
# set(QMLFileNames )
# GetRelativePath(
#     AbsPath ${CMAKE_CURRENT_SOURCE_DIR}/        #因为是路径 所以把多的/符去掉
#     ListPath ${QMLFiles}
#     OutRelative QMLFileNames
# )

function(GetRelativePath)
    set(prefix getRelativePath)
    set(options )
    set(oneValueKeyWords AbsPath)
    set(multiValueKeyWords ListPath OutRelative)
    cmake_parse_arguments(${prefix} "${options}" "${oneValueKeyWords}" "${multiValueKeyWords}" ${ARGN})
    #计算相对路径
    set(tempList)
    foreach(onePath ${${prefix}_ListPath})
        set(tempStr "")
        string(REPLACE ${${prefix}_AbsPath} "" tempStr ${onePath})
        LIST(APPEND tempList ${tempStr})
    endforeach()
    set(${${prefix}_OutRelative} ${tempList} PARENT_SCOPE) 
endfunction(GetRelativePath)

#获取当前目录下下的指定类型的文件
#示例：
# WTGetTypeFilesRelative(
#     FileType "*.qml"
#     OutRelative QMLFileNames
#     Recurse OFF #ON表示需要深度获取子文件夹中的文件 OFF表示只获取当前的
# )
function(WTGetTypeFilesRelative)
    set(prefix getRelativePath)
    set(options )
    set(oneValueKeyWords FileType Recurse)
    set(multiValueKeyWords OutRelative)
    cmake_parse_arguments(${prefix} "${options}" "${oneValueKeyWords}" "${multiValueKeyWords}" ${ARGN})

    #获取文件
    if(${prefix}_Recurse)
        file(GLOB_RECURSE Files ${${prefix}_FileType})
    else()
        file(GLOB Files ${${prefix}_FileType})
    endif()
    
    #计算相对路径
    set(tempList)
    foreach(onePath ${Files})
        set(tempStr "")
        string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "" tempStr ${onePath})        
        LIST(APPEND tempList ${tempStr})
    endforeach()
    set(${${prefix}_OutRelative} ${tempList} PARENT_SCOPE) 
endfunction(WTGetTypeFilesRelative)

