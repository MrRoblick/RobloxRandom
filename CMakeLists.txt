cmake_minimum_required(VERSION 3.14)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
endif()

project("RobloxRandomCli" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("internal/RobloxRandom")
target_compile_definitions(RobloxRandom PUBLIC ROBLOX_RANDOM_ENABLE_SEED_FINDER)

add_executable(roblox_random_cli "src/main.cpp")
target_link_libraries(roblox_random_cli PRIVATE RobloxRandom)