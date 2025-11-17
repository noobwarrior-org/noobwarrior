cmake_minimum_required(VERSION 3.26)
project(NoobWarrior)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cache/cpm")
include("${CMAKE_SOURCE_DIR}/CMake/CPM.cmake")

option(NOOBWARRIOR_LIBTYPE_STATIC "Compile noobWarrior as a static library" ON)
option(TARGET_NOOBWARRIOR_NOGUI "Add target noobWarrior NoGui" ON)
option(TARGET_NOOBWARRIOR_QTGUI "Add target noobWarrior QtGui" ON)
option(TARGET_NOOBWARRIOR_GAMELAUNCHER "Add target noobWarrior GameLauncher" ON)

# These targets must be compiled using a cross-compiler that can build a x86 32-bit Windows PE executable.
option(TARGET_NOOBWARRIOR_HOOK "Add target noobWarrior Hook" ON)
option(TARGET_NOOBWARRIOR_INJECTOR "Add target noobWarrior Injector" ON)

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
