cmake_minimum_required(VERSION 3.26)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(BUILD_DEPS ON)
set (CMAKE_BUILD_TYPE "Release" CACHE
      STRING "Choose the type of build.")
set(CMAKE_CXX_STANDARD 20)

project(yb_task)

add_subdirectory(apps)