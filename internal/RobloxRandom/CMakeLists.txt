cmake_minimum_required(VERSION 3.14)

project("RobloxRandom" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(RobloxRandom STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/random.cpp")
target_include_directories(RobloxRandom PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")