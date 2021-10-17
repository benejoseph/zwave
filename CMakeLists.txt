cmake_minimum_required (VERSION 2.8.11)
project (ZwaveExampleProject)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(Threads REQUIRED)
find_package(OpenZWave)
find_package(WiringPi REQUIRED)

include_directories("/usr/include/openzwave")
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${WIRINGPI_INCLUDE_DIRS})


add_library(ZWaveProcessor
  zwave_processor.cc
  zwave_processor.h
)

target_link_libraries(ZWaveProcessor ${OpenZWave_LIBRARIES}  Threads::Threads)

add_executable(PiBoardSwitch
  pi_board_switch_main.cc
  pi_board_switch_processor.cc
  pi_board_switch_processor.h
)
target_link_libraries(PiBoardSwitch Threads::Threads  ZWaveProcessor ${WIRINGPI_LIBRARIES})

add_executable(ZWaveExample 
  zwave_example_main.cc
)
target_link_libraries(ZWaveExample Threads::Threads  ZWaveProcessor)
