cmake_minimum_required(VERSION 3.24)

project(Dalmatian LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DAL_BUILD_TESTS "単体テスト（dalmatian_tests）をビルドする" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerOptions)
include(Dependencies)

add_subdirectory(src)

if(DAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
