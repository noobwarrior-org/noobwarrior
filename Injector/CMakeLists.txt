set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER ${LEGACY_COMPILER})
set(CMAKE_CXX_COMPILER ${LEGACY_COMPILER})

add_executable(NoobWarrior.Injector WIN32 Injector.cpp)
set_target_properties(NoobWarrior.Injector PROPERTIES
    OUTPUT_NAME "noobhook_x86_injector"
    WIN32_EXECUTABLE ON
    LINK_FLAGS "-m32"
)
target_compile_options(NoobWarrior.Injector PUBLIC -m32)
target_link_libraries(NoobWarrior.Injector PRIVATE version -static-libgcc -static-libstdc++)