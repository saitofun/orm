cmake_minimum_required(VERSION 3.0)

project(orm_cpp)

set(PRJ_DIR ${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory (
    ${PRJ_DIR} PRJSRCS
)

add_executable(${PROJECT_NAME} ${PRJSRCS})

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -ggdb")


