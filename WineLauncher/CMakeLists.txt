cmake_minimum_required(VERSION 3.26)
project(NoobWarrior.WineLauncher)

if (UNIX AND EXISTS "$ENV{PATH}/x86_64-w64-mingw32-g++" AND NOT WIN32)
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_C_COMPILER "$ENV{PATH}/x86_64-w64-mingw32-gcc")
    set(CMAKE_CXX_COMPILER "$ENV{PATH}/x86_64-w64-mingw32-g++")
endif()

add_executable(NoobWarrior.WineLauncher WIN32 WineLauncher.cpp)
target_include_directories(NoobWarrior.NoGui PUBLIC NoobWarrior.Core)
target_link_libraries(NoobWarrior.NoGui NoobWarrior.Core)