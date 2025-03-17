cmake_minimum_required(VERSION 3.26)
project(NoobWarrior)

set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cache/CPM")
include(CMake/CPM.cmake)

CPMAddPackage("gh:nlohmann/json#v3.11.3")

# I recommend that you just manually install these yourself using your package manager. if you're on windows then use msys2
# CPMAddPackage("gh:sqlite/sqlite#version-3.49.0")
# CPMAddPackage("gh:curl/curl#curl-8_12_1")

if (WIN32)
    set(SHOULD_BUILD_HOOK_BY_DEFAULT ON)
    set(SHOULD_BUILD_WINELAUNCHER_BY_DEFAULT OFF) # no point in doing this
elseif (UNIX)
    if (EXISTS "/usr/local/bin/x86_64-w64-mingw32-g++")
        set(SHOULD_BUILD_HOOK_BY_DEFAULT ON)
        set(SHOULD_BUILD_WINELAUNCHER_BY_DEFAULT ON)
    else()
        set(SHOULD_BUILD_HOOK_BY_DEFAULT OFF)
        set(SHOULD_BUILD_WINELAUNCHER_BY_DEFAULT OFF)
    endif()
endif()

option(BUILD_NOOBWARRIOR_NOGUI "Build noobWarrior NoGui" ON)
option(BUILD_NOOBWARRIOR_QTGUI "Build noobWarrior QtGui" ON)
option(BUILD_NOOBWARRIOR_HOOK "Build noobWarrior Hook" ${SHOULD_BUILD_HOOK_BY_DEFAULT})
option(BUILD_NOOBWARRIOR_WINELAUNCHER "Build noobWarrior WineLauncher" ${SHOULD_BUILD_WINELAUNCHER_BY_DEFAULT})

add_subdirectory(Core)

if (BUILD_NOOBWARRIOR_NOGUI)
    add_subdirectory(NoGui)
endif()

if (BUILD_NOOBWARRIOR_QTGUI)
    add_subdirectory(QtGui)
endif()

if (BUILD_NOOBWARRIOR_HOOK)
    add_subdirectory(Hook)
endif()

if (BUILD_NOOBWARRIOR_WINELAUNCHER)
    add_subdirectory(WineLauncher)
endif()