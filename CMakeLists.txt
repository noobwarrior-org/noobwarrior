cmake_minimum_required(VERSION 3.22)
project(NoobWarrior)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cache/cpm")
include("${CMAKE_SOURCE_DIR}/ThirdParty/cmake/CPM.cmake")

if (ANDROID)
    CPMAddPackage(
        NAME openssl-cmake
        GIT_REPOSITORY "https://github.com/viaduck/openssl-cmake.git"
        GIT_TAG "4edd36a8dab5f85a8f92650b5bdf6e0cab13aab8"
        OPTIONS
        "OPENSSL_BUILD_VERSION 3.5.0"
        "OPENSSL_USE_STATIC_LIBS ON"
        "OPENSSL_INSTALL OFF"
    )
    if (TARGET ssl AND NOT TARGET OpenSSL::SSL)
        add_library(OpenSSL::SSL ALIAS ssl)
    endif()
    if (TARGET crypto AND NOT TARGET OpenSSL::Crypto)
        add_library(OpenSSL::Crypto ALIAS crypto)
    endif()

    # FindOpenSSL shim: accepts the viaduck targets above and short-circuits the system-path search.
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/ThirdParty/cmake")
endif()

if (ANDROID)
    set(CMAKE_SKIP_INSTALL_RULES ON)

    # fake it til you make it
    set(FAKE_PKG_CONFIG "${CMAKE_SOURCE_DIR}/ThirdParty/pkgconfig/fake-pkg-config.sh")
    file(CHMOD "${FAKE_PKG_CONFIG}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    set(PKG_CONFIG_EXECUTABLE "${FAKE_PKG_CONFIG}" CACHE FILEPATH "pkg-config executable" FORCE)

    add_compile_definitions(_GNU_SOURCE)
endif()

option(NOOBWARRIOR_LIBTYPE_STATIC "Compile noobWarrior as a static library" ON)
option(NOOBWARRIOR_TARGET_CORE "Add target noobWarrior Core" ON)
option(NOOBWARRIOR_TARGET_UNITTEST "Add target noobWarrior UnitTest" ON)
option(NOOBWARRIOR_TARGET_NOGUI "Add target noobWarrior NoGui" ON)
option(NOOBWARRIOR_TARGET_QTGUI "Add target noobWarrior QtGui" ON)
option(NOOBWARRIOR_TARGET_TENFOOT "Add target noobWarrior Tenfoot" ON)

# If you are on a Unix OS, these targets must be compiled using a cross-compiler that can build a Windows PE file.
option(NOOBWARRIOR_TARGET_HOOK "Add target noobWarrior Hook" ON)
option(NOOBWARRIOR_TARGET_INJECTOR "Add target noobWarrior Injector" ON)

if (ANDROID)
    set(NOOBWARRIOR_LIBTYPE_STATIC OFF CACHE BOOL "" FORCE)
    set(NOOBWARRIOR_TARGET_UNITTEST OFF CACHE BOOL "" FORCE)
    set(NOOBWARRIOR_TARGET_NOGUI    OFF CACHE BOOL "" FORCE)
    set(NOOBWARRIOR_TARGET_QTGUI    OFF CACHE BOOL "" FORCE)
    set(NOOBWARRIOR_TARGET_TENFOOT  OFF CACHE BOOL "" FORCE)
    set(NOOBWARRIOR_TARGET_HOOK     OFF CACHE BOOL "" FORCE)
    set(NOOBWARRIOR_TARGET_INJECTOR OFF CACHE BOOL "" FORCE)
endif()

if (NOOBWARRIOR_TARGET_CORE)
    add_subdirectory(Core)
endif()

if (NOOBWARRIOR_TARGET_UNITTEST)
    add_subdirectory(UnitTest)
endif()

if (NOOBWARRIOR_TARGET_NOGUI)
    add_subdirectory(NoGui)
endif()

if (NOOBWARRIOR_TARGET_QTGUI)
    find_package(Qt6 COMPONENTS Core Widgets Multimedia MultimediaWidgets REQUIRED)

    add_subdirectory(QtShared)
    add_subdirectory(QtGui)
endif()

if (NOOBWARRIOR_TARGET_HOOK)
    add_subdirectory(Hook)
endif()

if (NOOBWARRIOR_TARGET_INJECTOR)
    add_subdirectory(Injector)
endif()
