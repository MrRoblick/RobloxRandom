cmake_minimum_required (VERSION 3.10)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("RobloxRandom")

add_executable (roblox_random "src/main.cpp" "src/random/random.cpp")
target_include_directories(roblox_random PRIVATE "include/")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET roblox_random PROPERTY CXX_STANDARD 26)
endif()
