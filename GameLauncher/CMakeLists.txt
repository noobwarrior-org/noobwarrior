find_package(SDL3 REQUIRED)

CPMAddPackage("gh:asmjit/asmjit#964e7c2")
set(ASMJIT_DIR ${asmjit_SOURCE_DIR})
set(BLEND2D_EXTERNAL_ASMJIT TRUE)
set(BLEND2D_STATIC TRUE)
CPMAddPackage("gh:blend2d/blend2d#007bf48")

add_executable(NoobWarrior.GameLauncher)
target_sources(NoobWarrior.GameLauncher PRIVATE
    GameLauncher.cpp
    Menu2D.cpp
)
target_link_libraries(NoobWarrior.GameLauncher NoobWarrior.Core SDL3::SDL3 blend2d::blend2d)
set_target_properties(NoobWarrior.GameLauncher PROPERTIES OUTPUT_NAME "noobwarrior-gamelauncher")
