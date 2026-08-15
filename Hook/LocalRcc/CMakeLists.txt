if (NOT MSVC)
    return()
endif()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Use a separate build tree for each value because CPM can retain an already-populated
# Luau source tree when this cache variable is changed in place.
if (NOT NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.574" AND
    NOT NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.719")
    message(FATAL_ERROR
        "NOOBWARRIOR_LOCALRCC_LUAU_VERSION must be 0.574 or 0.719")
endif()

if (NOT TARGET Luau.Compiler OR
    NOT NOOBWARRIOR_LOCALRCC_LUAU_SOURCE_DIR)
    message(FATAL_ERROR
        "LocalRcc requires the versioned Luau package from Hook/CMakeLists.txt")
endif()

# CPM's shared cache can leave Luau_SOURCE_DIR pointing at an already-populated
# checkout after the requested tag changes. Modern tags can emit the same
# bytecode version, so compile-time bytecode-version checks cannot detect a mix-up.
# Pin one upstream compiler source blob per supported tag and fail configuration
# instead of producing a correctly-labelled DLL with the wrong compiler inside.
if (NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.574")
    set(_noobwarrior_expected_luau_compiler_sha256
        "97844CC4BE411EEB4719FC62D6633331C9B8EF9A614986DB29F4002A28EC5EB7")
elseif (NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.719")
    set(_noobwarrior_expected_luau_compiler_sha256
        "65C60F15EB7C782DCE563AAF86778716533DF8C03902FDA012E57EC9610657E6")
endif()
set(_noobwarrior_luau_compiler_source
    "${NOOBWARRIOR_LOCALRCC_LUAU_SOURCE_DIR}/Compiler/src/Compiler.cpp")
if (NOT EXISTS "${_noobwarrior_luau_compiler_source}")
    message(FATAL_ERROR
        "LocalRcc: Luau compiler source is missing from "
        "${NOOBWARRIOR_LOCALRCC_LUAU_SOURCE_DIR}")
endif()
file(SHA256 "${_noobwarrior_luau_compiler_source}"
     _noobwarrior_actual_luau_compiler_sha256)
string(TOUPPER "${_noobwarrior_actual_luau_compiler_sha256}"
       _noobwarrior_actual_luau_compiler_sha256)
if (NOT _noobwarrior_actual_luau_compiler_sha256 STREQUAL
        _noobwarrior_expected_luau_compiler_sha256)
    message(FATAL_ERROR
        "LocalRcc: Luau source does not match requested tag "
        "${NOOBWARRIOR_LOCALRCC_LUAU_VERSION}. Use a fresh build directory; "
        "the current CPM source cache contains a different Luau revision.")
endif()

CPMAddPackage(
    URI "gh:facebook/zstd#48c0ed7"
    SOURCE_SUBDIR build/cmake
    OPTIONS
        "ZSTD_BUILD_STATIC ON"
        "ZSTD_BUILD_SHARED OFF"
        "ZSTD_BUILD_PROGRAMS OFF"
        "ZSTD_BUILD_TESTS OFF"
)

CPMAddPackage(
    NAME xxhash
    URL "https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.2.tar.gz"
    DOWNLOAD_ONLY YES
)
if (NOT xxhash_SOURCE_DIR OR NOT EXISTS "${xxhash_SOURCE_DIR}/xxhash.h")
    message(FATAL_ERROR "LocalRcc: xxhash CPM fetch failed -- xxhash.h not at ${xxhash_SOURCE_DIR}")
endif()

# Player 0.719 accepts the deterministic legacy (scheme 0) BLAKE3 trailer when
# DFIntLsbOptimizeMin is served as zero. No client verifier patch is required.
CPMAddPackage(
    NAME BLAKE3
    GITHUB_REPOSITORY "BLAKE3-team/BLAKE3"
    GIT_TAG "93a431c78a52d7ccf0f366f106467f5070e6075e"
    DOWNLOAD_ONLY YES
)
add_library(noobwarrior_blake3 STATIC
    ${BLAKE3_SOURCE_DIR}/c/blake3.c
    ${BLAKE3_SOURCE_DIR}/c/blake3_dispatch.c
    ${BLAKE3_SOURCE_DIR}/c/blake3_portable.c
)
target_include_directories(noobwarrior_blake3 PUBLIC ${BLAKE3_SOURCE_DIR}/c)
target_compile_definitions(noobwarrior_blake3 PRIVATE
    BLAKE3_NO_SSE2
    BLAKE3_NO_SSE41
    BLAKE3_NO_AVX2
    BLAKE3_NO_AVX512
)
set_property(TARGET noobwarrior_blake3 PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(NoobWarrior.LocalRcc SHARED
    LocalRcc.cpp
)
if (NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.574")
    set(_noobwarrior_localrcc_output_name
        "noobhook_x86-64_localrcc_0574")
else()
    set(_noobwarrior_localrcc_output_name
        "noobhook_x86-64_localrcc_0719")
endif()
set_target_properties(NoobWarrior.LocalRcc PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${_noobwarrior_localrcc_output_name}"
)

target_include_directories(NoobWarrior.LocalRcc PRIVATE
    ${xxhash_SOURCE_DIR}
)

target_link_libraries(NoobWarrior.LocalRcc PRIVATE
    minhook
    patterns
    Luau.Compiler
    libzstd_static
    noobwarrior_blake3
)
target_compile_definitions(NoobWarrior.LocalRcc PRIVATE
    NOOBWARRIOR_LOCALRCC_LUAU_VERSION="${NOOBWARRIOR_LOCALRCC_LUAU_VERSION}"
)
if (NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.574")
    target_compile_definitions(NoobWarrior.LocalRcc PRIVATE
        NOOBWARRIOR_LOCALRCC_LEGACY_LUAU_ENCODER=1
    )
endif()

target_link_libraries(NoobWarrior.LocalRcc PRIVATE
    LIBCMT
    LIBCPMT
)

target_link_options(NoobWarrior.LocalRcc PRIVATE
    /NODEFAULTLIB:MSVCRTD
    /NODEFAULTLIB:MSVCRT
    /NODEFAULTLIB:LIBCMTD
    /INCLUDE:_beginthreadex
)

set_property(TARGET NoobWarrior.LocalRcc PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# noobhook loads this stable entry point in Studio. The loader reads Studio's
# exact PE fingerprint and then loads the sidecar with the matching Luau tag.
# This keeps 0.574 and 0.719 payloads beside each other without an overwrite.
add_library(NoobWarrior.LocalRccLoader SHARED
    LocalRccLoader.cpp
)
set_target_properties(NoobWarrior.LocalRccLoader PROPERTIES
    PREFIX ""
    OUTPUT_NAME "noobhook_x86-64_localrcc"
)
target_link_libraries(NoobWarrior.LocalRccLoader PRIVATE
    LIBCMT
    LIBCPMT
)
target_link_options(NoobWarrior.LocalRccLoader PRIVATE
    /NODEFAULTLIB:MSVCRTD
    /NODEFAULTLIB:MSVCRT
    /NODEFAULTLIB:LIBCMTD
)
set_property(TARGET NoobWarrior.LocalRccLoader PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
