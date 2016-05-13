cmake_minimum_required(VERSION 3.5)
project(FLY)

# Include CMake modules local to this project.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_subdirectory(src)
