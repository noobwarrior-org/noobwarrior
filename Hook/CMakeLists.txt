cmake_minimum_required(VERSION 3.26)
project(NoobWarrior.Hook)

if (UNIX AND EXISTS "/usr/local/bin/x86_64-w64-mingw32-g++" AND NOT WIN32)
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_C_COMPILER /usr/local/bin/x86_64-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER /usr/local/bin/x86_64-w64-mingw32-g++)
endif()

set(SOURCES
    Hook.cpp
    Hook.h
)

add_library(NoobWarrior.Hook SHARED ${SOURCES})
set_target_properties(NoobWarrior.Hook PROPERTIES
    PREFIX ""
    OUTPUT_NAME "NoobWarriorHook"
    COMPILE_OPTIONS "-m32" LINK_FLAGS "-m32" # 32-bit DLL since our target is a 32-bit process
)
target_link_libraries(NoobWarrior.Hook PRIVATE version)