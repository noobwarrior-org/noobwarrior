if (MSVC)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

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

set(SOURCES
    Hook.cpp
    Hook.h
    Ping.cpp
    Ping.h
 "Patch/RemoveTrustCheck.cpp" "Patch/DoSomething1.cpp" "Patch/FixSettingsKeyMustBeDefined.cpp" "Patch/FixInsertObjects.cpp" "Patch/FixStudioUnableToConnect.cpp" "Patch/RemoveTLSVerification.cpp" "Patch/Patch.cpp" "Patch/Patches.h" "Patch/BypassPlaceIdVerification.cpp" "Patch/RemoveSignatureCheck.cpp" "Patch/BypassVersionOutOfDate.cpp" "Patch/DoSomething2.cpp" "Patch/FixPlayerMissingEdxNullCheck.cpp" "Patch/FixPlayerNullMovssDeref.cpp" "Patch/FixPlayerNullChunkMovqDeref.cpp")

add_library(NoobWarrior.Hook SHARED ${SOURCES})
set_target_properties(NoobWarrior.Hook PROPERTIES
    PREFIX ""
    OUTPUT_NAME "noobhook"
)
target_link_libraries(NoobWarrior.Hook PRIVATE wsock32 ws2_32 winhttp minhook patterns version nlohmann_json dbghelp)
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
    target_compile_features(NoobWarrior.Hook PUBLIC cxx_std_23) # doesnt work on msvc because it retarded
    target_link_libraries(NoobWarrior.Hook PRIVATE -static -static-libgcc -static-libstdc++)
endif()