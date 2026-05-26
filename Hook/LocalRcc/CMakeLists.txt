# LocalRcc -- sibling DLL loaded by noobhook.dll only when running inside
# RobloxStudioBeta.exe. Ported from rsblox/local_rcc.

if (NOT MSVC)
    return()
endif()

# Studio is 64-bit only on modern Roblox; no point building a 32-bit variant.
if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
CPMAddPackage(
    URI "gh:luau-lang/luau#0.575"
    OPTIONS
        "LUAU_BUILD_CLI OFF"
        "LUAU_BUILD_TESTS OFF"
        "LUAU_BUILD_WEB OFF"
        "LUAU_STATIC_CRT ON"
)

# ---- zstd -------------------------------------------------------------------
CPMAddPackage(
    URI "gh:facebook/zstd#48c0ed7"
    SOURCE_SUBDIR build/cmake
    OPTIONS
        "ZSTD_BUILD_STATIC ON"
        "ZSTD_BUILD_SHARED OFF"
        "ZSTD_BUILD_PROGRAMS OFF"
        "ZSTD_BUILD_TESTS OFF"
)

# ---- xxHash (header-only via XXH_INLINE_ALL) --------------------------------
# Lowercase NAME so ${xxhash_SOURCE_DIR} matches CPM/FetchContent's lookup.
CPMAddPackage(
    NAME xxhash
    URL "https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.2.tar.gz"
    DOWNLOAD_ONLY YES
)
if (NOT xxhash_SOURCE_DIR OR NOT EXISTS "${xxhash_SOURCE_DIR}/xxhash.h")
    message(FATAL_ERROR "LocalRcc: xxhash CPM fetch failed -- xxhash.h not at ${xxhash_SOURCE_DIR}")
endif()

add_library(NoobWarrior.LocalRcc SHARED
    LocalRcc.cpp
)
set_target_properties(NoobWarrior.LocalRcc PROPERTIES
    PREFIX ""
    OUTPUT_NAME "noobhook_localrcc"
)

target_include_directories(NoobWarrior.LocalRcc PRIVATE
    ${xxhash_SOURCE_DIR}
)

target_link_libraries(NoobWarrior.LocalRcc PRIVATE
    minhook
    patterns
    Luau.Compiler
    libzstd_static
)

target_link_libraries(NoobWarrior.LocalRcc PRIVATE
    LIBCMT
    LIBCPMT
)

target_link_options(NoobWarrior.LocalRcc PRIVATE
    /NODEFAULTLIB:MSVCRTD
    /NODEFAULTLIB:MSVCRT
    /NODEFAULTLIB:LIBCMTD
)

set_property(TARGET NoobWarrior.LocalRcc PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
