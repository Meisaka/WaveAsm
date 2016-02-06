cmake_minimum_required(VERSION 2.8)

project(WaveAsm)

option(BUILD_SHARED "Build shared library" OFF)

file(GLOB WAVESRC "*.cpp" "*.hpp" "*.c" "*.h")
file(GLOB WAVECLISRC "cli/*.c" "cli/*.h")

configure_file("setting.in" "setting.h")

include_directories(${CMAKE_SOURCE_DIR})

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)
endif()

if(${BUILD_SHARED})
	add_library(isf SHARED ${WAVESRC})
else()
	add_library(isf STATIC ${WAVESRC})
endif()

add_executable(waveasm ${WAVECLISRC})
target_link_libraries(waveasm isf)

