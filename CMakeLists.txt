cmake_minimum_required(VERSION 3.26)
project(NoobWarrior)

set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cache/cpm")
include("${CMAKE_SOURCE_DIR}/CMake/CPM.cmake")

find_program(LEGACY_COMPILER i686-w64-mingw32-g++)

if (LEGACY_COMPILER)
    set(SHOULD_TARGET_HOOK_BY_DEFAULT ON)
    set(SHOULD_TARGET_INJECTOR_BY_DEFAULT ON)
    message(STATUS "Found a 32-bit compiler. NoobWarrior.Hook and NoobWarrior.Injector will be added as a build target.")
else()
    set(SHOULD_TARGET_HOOK_BY_DEFAULT OFF)
    set(SHOULD_TARGET_INJECTOR_BY_DEFAULT OFF)
    message(STATUS "A 32-bit compiler was not found. Disregarding NoobWarrior.Hook and NoobWarrior.Injector.")
endif()

option(NOOBWARRIOR_LIBTYPE_STATIC "Compile noobWarrior as a static library" ON)
option(TARGET_NOOBWARRIOR_NOGUI "Target noobWarrior NoGui" ON)
option(TARGET_NOOBWARRIOR_QTGUI "Target noobWarrior QtGui" ON)
option(TARGET_NOOBWARRIOR_GAMELAUNCHER "Target noobWarrior GameLauncher" OFF)
option(TARGET_NOOBWARRIOR_HOOK "Target noobWarrior Hook" ${SHOULD_TARGET_HOOK_BY_DEFAULT})
option(TARGET_NOOBWARRIOR_INJECTOR "Target noobWarrior Injector" ${SHOULD_TARGET_INJECTOR_BY_DEFAULT})

add_subdirectory(Core)

if (TARGET_NOOBWARRIOR_NOGUI)
    add_subdirectory(NoGui)
endif()

if (TARGET_NOOBWARRIOR_QTGUI)
    find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

    add_subdirectory(QtShared)
    add_subdirectory(QtGui)
endif()

if (TARGET_NOOBWARRIOR_GAMELAUNCHER)
    add_subdirectory(GameLauncher)
endif()

if (TARGET_NOOBWARRIOR_HOOK)
    add_subdirectory(Hook)
endif()

if (TARGET_NOOBWARRIOR_INJECTOR)
    add_subdirectory(Injector)
endif()
