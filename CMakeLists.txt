cmake_minimum_required(VERSION 3.26)
project(NoobWarrior)

set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cache/CPM")
include(CMake/CPM.cmake)

CPMAddPackage("gh:sqlite/sqlite#version-3.49.0")

option(BUILD_NOOBWARRIOR_NOGUI "Build noobWarrior NoGui" ON)
option(BUILD_NOOBWARRIOR_QTGUI "Build noobWarrior QtGui" ON)

add_subdirectory(Core)

if (BUILD_NOOBWARRIOR_NOGUI)
    add_subdirectory(NoGui)
endif()

if (BUILD_NOOBWARRIOR_QTGUI)
    add_subdirectory(QtGui)
endif()