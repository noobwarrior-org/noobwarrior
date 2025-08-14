set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_library(NoobWarrior.QtShared STATIC)
target_sources(NoobWarrior.QtShared PRIVATE
    LoadingDialog.cpp
    shared_resources.qrc
)
target_link_libraries(NoobWarrior.QtShared PUBLIC NoobWarrior.Core Qt6::Core Qt6::Widgets)
target_include_directories(NoobWarrior.QtShared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(NoobWarrior.QtShared PUBLIC cxx_std_23)