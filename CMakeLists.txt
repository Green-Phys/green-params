cmake_minimum_required(VERSION 3.18)
project(green-params CXX)

set(CMAKE_CXX_STANDARD 17)

execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE TMP_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
if ( PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME )
    set(GIT_HASH "${PROJECT_NAME}:${TMP_HASH}" CACHE INTERNAL "")
else ()
    set(GIT_HASH "${GIT_HASH}**${PROJECT_NAME}:${TMP_HASH}" CACHE INTERNAL "")
endif ()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

add_subdirectory(src)
target_include_directories(params INTERFACE libs)
add_library(GREEN::PARAMS ALIAS params)

option(Build_Tests "Build tests" ON)
if (Build_Tests)
    enable_testing()
    add_subdirectory(test)
endif (Build_Tests)



