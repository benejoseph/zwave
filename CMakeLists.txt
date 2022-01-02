cmake_minimum_required (VERSION 2.8.11)
project (ZwaveExampleProject)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(Threads REQUIRED)
find_package(OpenZWave)

include_directories("/usr/include/openzwave")
include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory( pigpio)

add_library(ZWaveProcessor STATIC
  zwave_processor.cc
  zwave_processor.h
)

target_link_libraries(ZWaveProcessor ${OpenZWave_LIBRARIES}  Threads::Threads)

add_executable(PiBoardSwitch
  pi_board_switch_main.cc
  pi_board_switch_processor.cc
  pi_board_switch_processor.h
  pi_board_switch_controller.cc
  pi_board_switch_controller.h
)
target_link_libraries(PiBoardSwitch Threads::Threads  ZWaveProcessor pigpio)

#add_executable(ZWaveExample
#  zwave_example_main.cc
#)
#target_link_libraries(ZWaveExample Threads::Threads  ZWaveProcessor)
