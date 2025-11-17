CPMAddPackage(
    URI "gh:TsudaKageyu/minhook#1e9ad1e"
    OPTIONS "BUILD_SHARED_LIBS OFF"
)

set(SOURCES
    Hook.cpp
    Hook.h
)

add_library(NoobWarrior.Hook SHARED ${SOURCES})
set_target_properties(NoobWarrior.Hook PROPERTIES
    PREFIX ""
    OUTPUT_NAME "noobhook_x86"
    COMPILE_OPTIONS "-m32" LINK_FLAGS "-m32" # 32-bit DLL since our target is a 32-bit process
)
target_link_libraries(NoobWarrior.Hook PRIVATE minhook version -static -static-libgcc -static-libstdc++)
target_compile_features(NoobWarrior.Hook PUBLIC cxx_std_23)