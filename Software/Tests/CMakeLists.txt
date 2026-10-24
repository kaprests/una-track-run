cmake_minimum_required(VERSION 3.21)
project(trackrun-host-tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(GTest REQUIRED)

set(LIBS ${CMAKE_CURRENT_LIST_DIR}/../Libs)

add_executable(trackrun-host-tests
    SegmentCorrector_test.cpp
    TrackDistance_test.cpp
    ${LIBS}/Sources/SegmentCorrector.cpp
)
target_include_directories(trackrun-host-tests PRIVATE ${LIBS}/Header/service)
target_link_libraries(trackrun-host-tests PRIVATE GTest::gtest_main)
