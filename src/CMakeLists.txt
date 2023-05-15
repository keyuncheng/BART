cmake_minimum_required(VERSION 3.2)

project(BalancedConversion)

add_subdirectory(util)
add_subdirectory(model)
add_subdirectory(comm)

aux_source_directory(. DIR_SRCS)

add_executable(GenPlacement GenPlacement.cc)
add_executable(Simulator Simulator.cc)
add_executable(Coordinator Coordinator.cc)

target_link_libraries(GenPlacement util model)
target_link_libraries(Simulator util model)
target_link_libraries(Coordinator comm util model)