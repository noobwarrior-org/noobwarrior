set(SOURCES
    Injector.cpp
)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SOURCES ${SOURCES} Bypass.cpp ManualMap.cpp Syscalls.cpp) # only required for 64 bit as this is for hyperion
endif()
add_executable(NoobWarrior.Injector WIN32 ${SOURCES})
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(NoobWarrior.Injector PROPERTIES
        OUTPUT_NAME "noobhook_x86-64_injector"
        WIN32_EXECUTABLE ON
    )
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set_target_properties(NoobWarrior.Injector PROPERTIES
        OUTPUT_NAME "noobhook_x86_injector"
        WIN32_EXECUTABLE ON
    )
else()
    message(FATAL_ERROR "Unknown pointer size: ${CMAKE_SIZEOF_VOID_P}")
endif()
if(MSVC)
    target_link_libraries(NoobWarrior.Injector PRIVATE version psapi)
endif()
target_compile_features(NoobWarrior.Injector PUBLIC cxx_std_20)
set_property(TARGET NoobWarrior.Injector PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
