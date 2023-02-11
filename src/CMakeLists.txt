cmake_minimum_required(VERSION 3.2)

project(BalancedTransition)

add_subdirectory(model)
add_subdirectory(util)

aux_source_directory(. DIR_SRCS)

add_executable(GenPlacement GenPlacement.cc)
add_executable(Simulator Simulator.cc)

target_link_libraries(GenPlacement model util)
target_link_libraries(Simulator model util)