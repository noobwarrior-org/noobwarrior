CPMAddPackage(
    URI "gh:google/googletest#77f6bd3"
)

add_executable(NoobWarrior.UnitTest)
target_sources(NoobWarrior.UnitTest PRIVATE
    main.cpp
)
target_link_libraries(NoobWarrior.UnitTest PUBLIC NoobWarrior.Core GTest::gtest)
target_include_directories(NoobWarrior.UnitTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(NoobWarrior.UnitTest PUBLIC cxx_std_23)