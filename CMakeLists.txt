cmake_minimum_required(VERSION 3.7)
project(cpio_api)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_library(cpio_api SHARED cpio_api.cpp)
target_link_libraries(cpio_api archive)
