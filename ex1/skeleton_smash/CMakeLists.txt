cmake_minimum_required(VERSION 3.0.0)
project(ex1 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)

add_executable(
ex1
smash.cpp
Commands.cpp
Commands.h
signals.cpp
signals.h
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
