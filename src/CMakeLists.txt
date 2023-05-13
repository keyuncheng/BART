cmake_minimum_required(VERSION 3.2)

project(BalancedConversion)

add_subdirectory(model)
add_subdirectory(util)
add_subdirectory(comm)

aux_source_directory(. DIR_SRCS)

add_executable(GenPlacement GenPlacement.cc)
add_executable(Simulator Simulator.cc)
add_executable(Coordinator Coordinator.cc)

target_link_libraries(GenPlacement model util)
target_link_libraries(Simulator model util)
target_link_libraries(Coordinator model util comm)