cmake_minimum_required(VERSION 2.6)

project(WaveAsm)

file(GLOB WAVESRC "*.c" "*.h")
file(GLOB WAVECLISRC "cli/*.c" "cli/*.h")

configure_file("setting.in" "setting.h")

include_directories(${CMAKE_SOURCE_DIR})

add_library(waveasm STATIC ${WAVESRC})

add_executable(WaveAsm ${WAVECLISRC})
target_link_libraries(WaveAsm waveasm)

