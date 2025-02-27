cmake_minimum_required(VERSION 3.26)
project(NoobWarrior.NoGui)

add_executable(NoobWarrior.NoGui main.cpp)
set_target_properties(NoobWarrior.NoGui PROPERTIES OUTPUT_NAME "noobwarrior-cli")
target_include_directories(NoobWarrior.NoGui PUBLIC ${CMAKE_SOURCE_DIR}/Core)
target_link_libraries(NoobWarrior.NoGui NoobWarrior.Core)