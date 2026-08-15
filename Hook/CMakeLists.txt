if (NOT MSVC)
    return()
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NOOBWARRIOR_LOCALRCC_LUAU_VERSION "0.719" CACHE STRING
    "Luau compiler version embedded in this LocalRcc sidecar")
set_property(CACHE NOOBWARRIOR_LOCALRCC_LUAU_VERSION PROPERTY STRINGS
    "0.574" "0.719")
if (NOT NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.574" AND
    NOT NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.719")
    message(FATAL_ERROR
        "NOOBWARRIOR_LOCALRCC_LUAU_VERSION must be 0.574 or 0.719")
endif()

# original MinHook (used by legacy non-Hyperion build)
CPMAddPackage(
    URI "gh:TsudaKageyu/minhook#1e9ad1e"
    OPTIONS "BUILD_SHARED_LIBS OFF"
)

# Forked MinHook (syscall allocators, used by Hyperion build)
add_library(minhook_hyperion STATIC
    MinHookHyperion/src/buffer.c
    MinHookHyperion/src/hook.c
    MinHookHyperion/src/trampoline.c
    MinHookHyperion/src/hde/hde32.c
    MinHookHyperion/src/hde/hde64.c
)
target_include_directories(minhook_hyperion PUBLIC MinHookHyperion/include MinHookHyperion/src)
set_property(TARGET minhook_hyperion PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET minhook_hyperion PROPERTY C_STANDARD 99)

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
if (NOOBWARRIOR_LOCALRCC_LUAU_VERSION STREQUAL "0.574")
    set(_noobwarrior_luau_package_name luau_tag_0574)
else()
    set(_noobwarrior_luau_package_name luau_tag_0719)
endif()
CPMAddPackage(
    NAME ${_noobwarrior_luau_package_name}
    URL "https://github.com/luau-lang/luau/archive/refs/tags/${NOOBWARRIOR_LOCALRCC_LUAU_VERSION}.tar.gz"
    OPTIONS
        "LUAU_BUILD_CLI OFF"
        "LUAU_BUILD_TESTS OFF"
        "LUAU_BUILD_WEB OFF"
        "LUAU_STATIC_CRT ON"
)
set(_noobwarrior_luau_source_var
    "${_noobwarrior_luau_package_name}_SOURCE_DIR")
set(NOOBWARRIOR_LOCALRCC_LUAU_SOURCE_DIR
    "${${_noobwarrior_luau_source_var}}")

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
 "Patch/RemoveTrustCheck.cpp"  "Patch/FixSettingsKeyMustBeDefined.cpp" "Patch/FixInsertObjects.cpp"  "Patch/RemoveTLSVerification.cpp" "Patch/Patch.cpp" "Patch/Patches.h" "Patch/BypassPlaceIdVerification.cpp" "Patch/RemoveSignatureCheck.cpp"   "Patch/ClusterNullDerefGuard.cpp" "Patch/CrashDiagnostics.cpp" "Patch/CsgHeapGuard.cpp" "Patch/UnionRenderUnlock.cpp" "Patch/TrampolineIntegrityBypass.cpp" "ScriptExecutor/ScriptExecutor.cpp" "ScriptExecutor/ScriptExecutor.h" "ntdll.h" "ntdll.cpp" "ScriptExecutor/Instance.cpp" "ScriptExecutor/Instance.h")

# non-hyperion
add_library(NoobWarrior.Hook SHARED ${SOURCES})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(NoobWarrior.Hook PROPERTIES
        PREFIX ""
        OUTPUT_NAME "noobhook_x86-64"
    )
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set_target_properties(NoobWarrior.Hook PROPERTIES
        PREFIX ""
        OUTPUT_NAME "noobhook_x86"
    )
else()
    message(FATAL_ERROR "Unknown pointer size: ${CMAKE_SIZEOF_VOID_P}")
endif()
target_include_directories(NoobWarrior.Hook PRIVATE ${xxhash_SOURCE_DIR})
target_link_libraries(NoobWarrior.Hook PRIVATE ws2_32 winhttp crypt32 minhook patterns version nlohmann_json Luau.Compiler libzstd_static)
target_compile_features(NoobWarrior.Hook PUBLIC cxx_std_23)

target_link_libraries(NoobWarrior.Hook PRIVATE LIBCMT LIBCPMT)
target_link_options(NoobWarrior.Hook PRIVATE
    /NODEFAULTLIB:MSVCRTD
    /NODEFAULTLIB:MSVCRT
    /NODEFAULTLIB:LIBCMTD
)
set_property(TARGET NoobWarrior.Hook PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# hyperion
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Minimal loader-free bridge mapped before Player resolves its Winsock dispatch table.
    enable_language(ASM_MASM)
    # Keep the dependency-free assembly helper first in the clean Release image.
    add_library(NoobWarrior.HookBootstrap SHARED EarlyRedirectVehThunk.asm EarlyRedirect.cpp)
    set_target_properties(NoobWarrior.HookBootstrap PROPERTIES
        PREFIX ""
        OUTPUT_NAME "noobhook_x86-64_hyperion_bootstrap"
    )
    target_compile_options(NoobWarrior.HookBootstrap PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/GS->)
    target_link_options(NoobWarrior.HookBootstrap PRIVATE
        /NODEFAULTLIB
        /ENTRY:DllMain
        /MANIFEST:NO
        # Incremental linking creates an ILT thunk one byte before the assembly
        # callback. That address is not a CFG target in the signed backing image.
        /INCREMENTAL:NO
    )

    add_library(NoobWarrior.HookHyperion SHARED ${SOURCES})
    set_target_properties(NoobWarrior.HookHyperion PROPERTIES
        PREFIX ""
        OUTPUT_NAME "noobhook_x86-64_hyperion"
    )
    target_compile_definitions(NoobWarrior.HookHyperion PRIVATE NOOBHOOK_HYPERION)
    target_include_directories(NoobWarrior.HookHyperion PRIVATE ${xxhash_SOURCE_DIR})
    target_link_libraries(NoobWarrior.HookHyperion PRIVATE ws2_32 winhttp crypt32 minhook_hyperion patterns version nlohmann_json Luau.Compiler libzstd_static)
    target_compile_features(NoobWarrior.HookHyperion PUBLIC cxx_std_23)

    target_link_libraries(NoobWarrior.HookHyperion PRIVATE LIBCMT LIBCPMT)
    target_link_options(NoobWarrior.HookHyperion PRIVATE
        /NODEFAULTLIB:MSVCRTD
        /NODEFAULTLIB:MSVCRT
        /NODEFAULTLIB:LIBCMTD
    )
    set_property(TARGET NoobWarrior.HookHyperion PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_subdirectory(LocalRcc)
