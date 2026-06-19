if (NOT MSVC)
    return()
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

CPMAddPackage(
    URI "gh:TsudaKageyu/minhook#1e9ad1e"
    OPTIONS "BUILD_SHARED_LIBS OFF"
)

CPMAddPackage(
    URI "gh:ThirteenAG/Hooking.Patterns#460a47b"
)

CPMAddPackage("gh:nlohmann/json#v3.11.3")

if (Hooking.Patterns_ADDED)
    add_library(patterns STATIC ${Hooking.Patterns_SOURCE_DIR}/Hooking.Patterns.cpp)
    set_property(TARGET patterns PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    target_include_directories(patterns PUBLIC ${Hooking.Patterns_SOURCE_DIR})
else()
    message(FATAL_ERROR "Failed to add Hooking.Patterns package")
endif()

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
CPMAddPackage(
    URI "gh:luau-lang/luau#0.575"
    OPTIONS
        "LUAU_BUILD_CLI OFF"
        "LUAU_BUILD_TESTS OFF"
        "LUAU_BUILD_WEB OFF"
        "LUAU_STATIC_CRT ON"
)

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
    message(FATAL_ERROR "xxhash CPM fetch failed -- xxhash.h not at ${xxhash_SOURCE_DIR}")
endif()

set(SOURCES
    Hook.cpp
    Hook.h
    Ping.cpp
    Ping.h
 "Patch/RemoveTrustCheck.cpp"  "Patch/FixSettingsKeyMustBeDefined.cpp" "Patch/FixInsertObjects.cpp"  "Patch/RemoveTLSVerification.cpp" "Patch/Patch.cpp" "Patch/Patches.h" "Patch/BypassPlaceIdVerification.cpp" "Patch/RemoveSignatureCheck.cpp"   "Patch/ClusterNullDerefGuard.cpp" "Patch/CrashDiagnostics.cpp" "Patch/CsgHeapGuard.cpp" "ScriptExecutor/ScriptExecutor.cpp" "ScriptExecutor/ScriptExecutor.h" "ntdll.h" "ntdll.cpp" "ScriptExecutor/Instance.cpp" "ScriptExecutor/Instance.h")

add_library(NoobWarrior.Hook SHARED ${SOURCES})
set_target_properties(NoobWarrior.Hook PROPERTIES
    PREFIX ""
    OUTPUT_NAME "noobhook"
)
target_include_directories(NoobWarrior.Hook PRIVATE
    ${xxhash_SOURCE_DIR}
)
target_link_libraries(NoobWarrior.Hook PRIVATE dbghelp wsock32 ws2_32 winhttp minhook patterns version nlohmann_json Luau.Compiler libzstd_static)
target_compile_features(NoobWarrior.Hook PUBLIC cxx_std_23)
if (MSVC)
    target_link_libraries(NoobWarrior.Hook PRIVATE
        LIBCMT
        LIBCPMT
    )

    target_link_options(NoobWarrior.Hook PRIVATE
        /NODEFAULTLIB:MSVCRTD
        /NODEFAULTLIB:MSVCRT
        /NODEFAULTLIB:LIBCMTD
    )

    set_property(TARGET NoobWarrior.Hook PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # LocalRcc -- Studio-only sibling DLL ported from rsblox/local_rcc.
    # Loaded at runtime by Hook.cpp when DetectSide() == Studio.
    add_subdirectory(LocalRcc)
else()
    target_link_libraries(NoobWarrior.Hook PRIVATE -static -static-libgcc -static-libstdc++)
endif()