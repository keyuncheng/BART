cmake_minimum_required(VERSION 3.2)

project(BalancedConversion)

add_subdirectory(util)
add_subdirectory(model)
add_subdirectory(comm)

aux_source_directory(. DIR_SRCS)

add_executable(GenPlacement GenPlacement.cc)
add_executable(Simulator Simulator.cc)
add_executable(Controller Controller.cc)
add_executable(Agent Agent.cc)

target_link_libraries(GenPlacement util model)
target_link_libraries(Simulator util model)
target_link_libraries(Controller comm util model pthread sockpp)
target_link_libraries(Agent comm util model pthread sockpp)