cmake_minimum_required(VERSION 2.8)

project(WaveAsm)

file(GLOB WAVESRC "*.cpp" "*.hpp" "*.c" "*.h")
file(GLOB WAVECLISRC "cli/*.c" "cli/*.h")

configure_file("setting.in" "setting.h")

include_directories(${CMAKE_SOURCE_DIR})

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)
endif()

add_library(waveasm STATIC ${WAVESRC})

add_executable(WaveAsm ${WAVECLISRC})
target_link_libraries(WaveAsm waveasm)

