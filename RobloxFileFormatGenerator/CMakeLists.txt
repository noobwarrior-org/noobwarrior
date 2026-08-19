CPMAddPackage(
    NAME libcppgenerate
    GITHUB_REPOSITORY rm5248/libcppgenerate
    GIT_TAG 6bf89d90ef27ea15e3a69331ee29647057c4f1a1
    OPTIONS "ENABLE_TESTS OFF" "ENABLE_EXAMPLES OFF"
)

if (NOT TARGET nlohmann_json)
    CPMAddPackage("gh:nlohmann/json#v3.11.3")
endif()

add_executable(NoobWarrior.RobloxFileFormatGenerator EXCLUDE_FROM_ALL main.cpp)
target_compile_features(NoobWarrior.RobloxFileFormatGenerator PRIVATE cxx_std_23)
target_include_directories(NoobWarrior.RobloxFileFormatGenerator PRIVATE
    ${libcppgenerate_SOURCE_DIR}
)
target_link_libraries(NoobWarrior.RobloxFileFormatGenerator PRIVATE
    cppgenerate_static
    nlohmann_json
)
