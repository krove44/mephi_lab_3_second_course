cmake_minimum_required(VERSION 3.20)
project(mephi_lab_2_second_course)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")

find_package(Qt6 COMPONENTS Widgets DataVisualization REQUIRED)

add_executable(mephi_lab_3_second_course
        main.cpp
)

include(FetchContent)
FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(tests
        tests.cpp
)
target_link_libraries(tests GTest::gtest_main)
target_compile_features(tests PRIVATE cxx_std_20)

add_executable(ui
        ui.cpp
)
target_link_libraries(ui PRIVATE Qt6::Widgets Qt6::DataVisualization)
target_compile_features(ui PRIVATE cxx_std_20)
