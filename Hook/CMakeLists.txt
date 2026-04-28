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
 "Patch/RemoveTrustCheck.cpp" "Patch/DoSomething1.cpp" "Patch/FixSettingsKeyMustBeDefined.cpp" "Patch/RemoveTLSVerification.cpp" "Patch/Patch.cpp" "Patch/Patches.h" "Patch/BypassPlaceIdVerification.cpp" "Patch/RemoveSignatureCheck.cpp" "Patch/BypassVersionOutOfDate.cpp" "Patch/DoSomething2.cpp")

add_library(NoobWarrior.Hook SHARED ${SOURCES})
set_target_properties(NoobWarrior.Hook PROPERTIES
    PREFIX ""
    OUTPUT_NAME "noobhook"
)
target_link_libraries(NoobWarrior.Hook PRIVATE wsock32 ws2_32 minhook patterns version -static -static-libgcc -static-libstdc++)
if (MSVC)
    target_link_libraries(NoobWarrior.Hook PRIVATE
        LIBCMT
    )

    target_link_options(NoobWarrior.Hook PRIVATE
        /NODEFAULTLIB:MSVCRT
        /NODEFAULTLIB:MSVCRTD
    )

    set_property(TARGET NoobWarrior.Hook PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_compile_features(NoobWarrior.Hook PUBLIC cxx_std_23) # doesnt work on msvc because it retarded
endif()