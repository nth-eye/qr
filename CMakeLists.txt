cmake_minimum_required(VERSION 3.0.0)
project(qr VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories(inc)

add_executable(qr main.cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
