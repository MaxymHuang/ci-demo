cmake_minimum_required(VERSION 3.21)

project(xri_demo
    VERSION 0.1.0
    DESCRIPTION "X-Ray Inspection demo app - CI proof of concept"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Test)
qt_standard_project_setup()

enable_testing()

add_subdirectory(src/core)
add_subdirectory(src/app)
add_subdirectory(tests)
