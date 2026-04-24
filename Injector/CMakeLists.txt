add_executable(NoobWarrior.Injector WIN32 Injector.cpp)
set_target_properties(NoobWarrior.Injector PROPERTIES
    OUTPUT_NAME "noobhook_injector"
    WIN32_EXECUTABLE ON
)
target_link_libraries(NoobWarrior.Injector PRIVATE version -static-libgcc -static-libstdc++)
target_compile_features(NoobWarrior.Injector PUBLIC cxx_std_20)