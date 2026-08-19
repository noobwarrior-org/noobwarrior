# Two diagnostics for the place-preparation pipeline. EXCLUDE_FROM_ALL like RoundTripPlace: they
# are run by hand when a prepared place looks wrong, not built as part of the app.
#
# DiffPlace matches instances between a source place and a prepared one by tree path and reports
# properties invented, value-changed or dropped. Preparing a place must never invent a property on
# an instance that came out of the file -- a binary column carries a value for every instance of
# its class, so an invented name hands every one of them a zero.
#
# PlacePollution looks for the same fingerprint without needing the source: a column where a
# handful of instances carry real values and the rest hold the type's zero.
add_executable(NoobWarrior.DiffPlace EXCLUDE_FROM_ALL main.cpp)
target_link_libraries(NoobWarrior.DiffPlace PRIVATE NoobWarrior.Core)
target_compile_features(NoobWarrior.DiffPlace PRIVATE cxx_std_23)

add_executable(NoobWarrior.PlacePollution EXCLUDE_FROM_ALL pollution.cpp)
target_link_libraries(NoobWarrior.PlacePollution PRIVATE NoobWarrior.Core)
target_compile_features(NoobWarrior.PlacePollution PRIVATE cxx_std_23)
