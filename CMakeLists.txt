cmake_minimum_required(VERSION 3.14)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
endif()

project("RobloxRandom" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_SRC_FILES
    "src/random/random.cpp"
)

add_library(RobloxRandom ${LIB_SRC_FILES})

target_include_directories(RobloxRandom PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_executable(roblox_random_cli "src/main.cpp")
    target_link_libraries(roblox_random_cli PRIVATE RobloxRandom)
endif()