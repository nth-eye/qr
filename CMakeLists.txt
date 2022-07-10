cmake_minimum_required(VERSION 3.0.0)
project(qr VERSION 0.1.0)

add_subdirectory(lib/utl)

add_library(libqr INTERFACE)
target_include_directories(libqr INTERFACE inc)
target_compile_features(libqr INTERFACE cxx_std_17)
target_compile_options(libqr INTERFACE "-Wall" "-Wextra" "-Wpedantic")
target_link_libraries(libqr INTERFACE libutl)

add_executable(qr main.cpp)
target_link_libraries(qr PRIVATE libqr)

# add_executable(testqr test/qr.cpp)
# target_link_libraries(testqr PRIVATE gtest_main libqr)
