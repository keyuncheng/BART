cmake_minimum_required(VERSION 3.1)

project(BART)

add_subdirectory(util)
add_subdirectory(model)
add_subdirectory(comm)

aux_source_directory(. DIR_SRCS)

add_executable(GenECStripe GenECStripe.cc)
add_executable(GenPrePlacement GenPrePlacement.cc)
add_executable(BTSGenerator BTSGenerator.cc)
add_executable(Controller Controller.cc)
add_executable(Agent Agent.cc)

target_link_libraries(GenECStripe comm util isal)
target_link_libraries(GenPrePlacement util model)
target_link_libraries(BTSGenerator util model)
target_link_libraries(Controller comm util model pthread sockpp)
target_link_libraries(Agent comm util model pthread sockpp isal)