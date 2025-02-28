cmake_minimum_required(VERSION 3.26)
project(NoobWarrior)

# set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cache/CPM")
# include(CMake/CPM.cmake)

# I recommend that you just manually install these yourself using your package manager. if you're on windows then use msys2
# CPMAddPackage("gh:sqlite/sqlite#version-3.49.0")
# CPMAddPackage("gh:curl/curl#curl-8_12_1")

option(BUILD_NOOBWARRIOR_NOGUI "Build noobWarrior NoGui" ON)
option(BUILD_NOOBWARRIOR_QTGUI "Build noobWarrior QtGui" ON)

add_subdirectory(Core)

if (BUILD_NOOBWARRIOR_NOGUI)
    add_subdirectory(NoGui)
endif()

if (BUILD_NOOBWARRIOR_QTGUI)
    add_subdirectory(QtGui)
endif()