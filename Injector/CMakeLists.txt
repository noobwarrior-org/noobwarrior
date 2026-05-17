add_executable(NoobWarrior.Injector WIN32 Injector.cpp)
set_target_properties(NoobWarrior.Injector PROPERTIES
    OUTPUT_NAME "noobhook_injector"
    WIN32_EXECUTABLE ON
)
if(MSVC)
    target_link_libraries(NoobWarrior.Injector PRIVATE version)
else()
    target_link_libraries(NoobWarrior.Injector PRIVATE version -static-libgcc -static-libstdc++)
endif()
target_compile_features(NoobWarrior.Injector PUBLIC cxx_std_20)
set_property(TARGET NoobWarrior.Injector PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")