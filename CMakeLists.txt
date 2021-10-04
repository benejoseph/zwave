cmake_minimum_required (VERSION 2.8.11)
project (ZwaveExampleProject)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/cmake)

include_directories("/usr/include/openzwave")
include_directories(${CMAKE_SOURCE_DIR})

find_package(Threads REQUIRED)
find_package(OpenZWave)

add_executable(ZWaveExample 
  main.cc
  zwave_processor.cc
  zwave_processor.h
)

message(STATUS ${OpenZWave})
target_link_libraries(ZWaveExample ${OpenZWave_LIBRARIES}  Threads::Threads)
