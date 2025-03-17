cmake_minimum_required(VERSION 3.26)
project(NoobWarrior.Hook)

set(SOURCES
    Hook.cpp
    Hook.h
)

add_library(NoobWarrior.Hook SHARED ${SOURCES})
set_target_properties(NoobWarrior.Hook PROPERTIES
    PREFIX ""
    OUTPUT_NAME "noobWarriorHook"
)
target_link_libraries(NoobWarrior.Hook PRIVATE version)