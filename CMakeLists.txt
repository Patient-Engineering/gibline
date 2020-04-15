cmake_minimum_required(VERSION 3.10)

project(gibline)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    gibline
    gibline.cpp
    gibline.h
)
target_include_directories(gibline PUBLIC .)

add_executable(dummy dummy.cpp)
target_link_libraries(dummy gibline)
