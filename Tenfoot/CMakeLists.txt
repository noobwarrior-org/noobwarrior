find_package(SDL3 REQUIRED)

CPMAddPackage(
    URI "gh:asmjit/asmjit#0bd5787"
    OPTIONS "ASMJIT_EMBED ON"
)
CPMAddPackage(
    URI "gh:blend2d/blend2d#6dbc2ce"
    OPTIONS "ASMJIT_DIR ${asmjit_SOURCE_DIR}" "BLEND2D_EXTERNAL_ASMJIT ON" "BLEND2D_STATIC ON" "BLEND2D_NO_INSTALL ON"
)

add_executable(NoobWarrior.Tenfoot)
target_sources(NoobWarrior.Tenfoot PRIVATE
    Tenfoot.cpp
    SceneManager.cpp
    Scene.cpp
    Node.cpp
)
target_link_libraries(NoobWarrior.Tenfoot NoobWarrior.Core SDL3::SDL3 blend2d::blend2d)
set_target_properties(NoobWarrior.Tenfoot PROPERTIES OUTPUT_NAME "noobwarrior-tenfoot")
