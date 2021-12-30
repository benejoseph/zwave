cmake_minimum_required(VERSION 3.0)

project(pigpio LANGUAGES C VERSION 0.71)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

find_package(Threads REQUIRED)
find_package(RT REQUIRED)

option(BUILD_SHARED_LIBS "Create shared libraries" ON)

add_compile_options(-Wall)

# libpigpio.(so|a)
add_library(pigpio pigpio.c command.c custom.cext)

