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

if (NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.719")
    # The WebRTC peer stays beside the exact 0.719 RemoteEvent and SDP codec
    # bridge in LocalRCC. Core owns shared networking policy, public TURN
    # reachability and credential issuance; the sidecar owns Studio-versioned
    # media negotiation and RTP forwarding. Keep it self-contained: Mbed TLS
    # avoids deploying datachannel.dll and OpenSSL runtime DLLs beside Studio.
    # Mbed TLS is Apache-2.0 and libdatachannel is MPL-2.0; both dependency
    # revisions are pinned for reproducible builds.
    CPMAddPackage(
        NAME noobwarrior_mbedtls
        GITHUB_REPOSITORY "Mbed-TLS/mbedtls"
        GIT_TAG "b336acaa2b8074c63d48a6ff43c74413102b6fdb"
        OPTIONS
            "ENABLE_PROGRAMS OFF"
            "ENABLE_TESTING OFF"
            "USE_STATIC_MBEDTLS_LIBRARY ON"
            "USE_SHARED_MBEDTLS_LIBRARY OFF"
            "DISABLE_PACKAGE_CONFIG_AND_INSTALL ON"
            "MBEDTLS_FATAL_WARNINGS OFF"
    )
    if (NOT TARGET MbedTLS::mbedtls OR
        NOT TARGET MbedTLS::mbedx509 OR
        NOT TARGET MbedTLS::mbedcrypto)
        message(FATAL_ERROR "LocalRcc: static Mbed TLS targets were not created")
    endif()

    # Mbed TLS keeps RFC 5764 negotiation behind an optional compile-time
    # switch. libdatachannel's DTLS-SRTP transport uses the corresponding
    # types, profile constants and APIs, so the definition must be present
    # both while Mbed TLS itself is compiled and while its headers are parsed
    # by libdatachannel.
    target_compile_definitions(mbedtls PUBLIC MBEDTLS_SSL_DTLS_SRTP)

    # The pinned libSRTP submodule still calls find_package(MbedTLS) even when
    # Mbed TLS is already part of the build. Put our adapter ahead of
    # libdatachannel's installed-library finder and force the crypto selection
    # because other noobWarrior dependencies may have cached these generic
    # options with different values earlier in the top-level configure.
    set(NOOBWARRIOR_LOCALRCC_MBEDTLS_INCLUDE_DIR
        "${noobwarrior_mbedtls_SOURCE_DIR}/include")
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")
    set(USE_MBEDTLS ON CACHE BOOL "" FORCE)
    set(USE_GNUTLS OFF CACHE BOOL "" FORCE)
    set(ENABLE_MBEDTLS ON CACHE BOOL "" FORCE)
    set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(ENABLE_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

    CPMAddPackage(
        NAME noobwarrior_libdatachannel
        GITHUB_REPOSITORY "paullouisageneau/libdatachannel"
        GIT_TAG "297244ec812b2afe65fb52d01a2d0e2e0f1dd4f1"
        GIT_SUBMODULES_RECURSE YES
        OPTIONS
            "BUILD_SHARED_LIBS OFF"
            "BUILD_SHARED_DEPS_LIBS OFF"
            "USE_GNUTLS OFF"
            "USE_MBEDTLS ON"
            "USE_NICE OFF"
            "PREFER_SYSTEM_LIB OFF"
            "USE_SYSTEM_SRTP OFF"
            "USE_SYSTEM_JUICE OFF"
            "USE_SYSTEM_USRSCTP OFF"
            "USE_SYSTEM_PLOG OFF"
            "USE_SYSTEM_JSON OFF"
            "NO_WEBSOCKET ON"
            "NO_MEDIA OFF"
            "NO_EXAMPLES ON"
            "NO_TESTS ON"
            "WARNINGS_AS_ERRORS OFF"
            "CAPI_STDCALL OFF"
            "RTC_UPDATE_VERSION_HEADER OFF"
            "ENABLE_OPENSSL OFF"
            "ENABLE_MBEDTLS ON"
    )
    if (NOT TARGET LibDataChannel::LibDataChannelStatic)
        message(FATAL_ERROR "LocalRcc: static libdatachannel target was not created")
    endif()
    foreach(_noobwarrior_datachannel_target datachannel datachannel-static)
        if (TARGET ${_noobwarrior_datachannel_target})
            target_compile_definitions(${_noobwarrior_datachannel_target} PRIVATE
                MBEDTLS_SSL_DTLS_SRTP
            )
        endif()
    endforeach()
endif()

add_library(NoobWarrior.LocalRcc SHARED
    LocalRcc.cpp
)
if (NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.719")
    target_sources(NoobWarrior.LocalRcc PRIVATE
        VoiceChat719.cpp
    )
endif()
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
if (NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.719")
    target_link_libraries(NoobWarrior.LocalRcc PRIVATE
        LibDataChannel::LibDataChannelStatic
        nlohmann_json
        iphlpapi
        winhttp
    )
    target_compile_definitions(NoobWarrior.LocalRcc PRIVATE
        NOOBWARRIOR_LOCALRCC_VOICE_WEBRTC=1
    )
endif()
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
