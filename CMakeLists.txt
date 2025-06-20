cmake_minimum_required(VERSION 3.26)
project(NoobWarrior)

if (WIN32)
    if (EXISTS "/msys64/mingw32/bin/g++.exe")
        set(SHOULD_TARGET_HOOK_BY_DEFAULT ON)
        message(STATUS "Found a 32-bit compiler. NoobWarrior.Hook will be added as a build target.")
    else()
        set(SHOULD_TARGET_HOOK_BY_DEFAULT OFF)
        message(STATUS "A 32-bit compiler was not found. Disregarding NoobWarrior.Hook.")
    endif()
    
    set(SHOULD_TARGET_WINELAUNCHER_BY_DEFAULT OFF) # no point in doing this
elseif (UNIX)
    if (EXISTS "$ENV{PATH}/x86_64-w64-mingw32-g++")
        set(SHOULD_TARGET_WINELAUNCHER_BY_DEFAULT ON)
        message(STATUS "Found a 64-bit MinGW compiler. NoobWarrior.WineLauncher will be added as a build target.")
    else()
        set(SHOULD_TARGET_WINELAUNCHER_BY_DEFAULT OFF)
        message(STATUS "A 64-bit MinGW compiler was not found. NoobWarrior.WineLauncher will be disregarded.")
    endif() 

    if (EXISTS "$ENV{PATH}/i686-w64-mingw32-g++")
        set(SHOULD_TARGET_HOOK_BY_DEFAULT ON)
        message(STATUS "Found a 32-bit MinGW compiler. NoobWarrior.Hook will be added as a build target.")
    else()
        set(SHOULD_TARGET_HOOK_BY_DEFAULT OFF)
        message(STATUS "A 32-bit MinGW compiler was not found. NoobWarrior.Hook will be disregarded.")
    endif()
endif()

option(TARGET_NOOBWARRIOR_NOGUI "Target noobWarrior NoGui" ON)
option(TARGET_NOOBWARRIOR_QTGUI "Target noobWarrior QtGui" ON)
option(TARGET_NOOBWARRIOR_GAMELAUNCHER "Target noobWarrior GameLauncher" ON)
option(TARGET_NOOBWARRIOR_HOOK "Target noobWarrior Hook" ${SHOULD_TARGET_HOOK_BY_DEFAULT})
option(TARGET_NOOBWARRIOR_WINELAUNCHER "Target noobWarrior WineLauncher" ${SHOULD_TARGET_WINELAUNCHER_BY_DEFAULT})

add_subdirectory(Core)

if (TARGET_NOOBWARRIOR_NOGUI)
    add_subdirectory(NoGui)
endif()

if (TARGET_NOOBWARRIOR_QTGUI)
    add_subdirectory(QtGui)
endif()

if (TARGET_NOOBWARRIOR_GAMELAUNCHER)
    add_subdirectory(GameLauncher)
endif()

if (TARGET_NOOBWARRIOR_HOOK)
    add_subdirectory(Hook)
endif()

if (TARGET_NOOBWARRIOR_WINELAUNCHER)
    add_subdirectory(WineLauncher)
endif()
