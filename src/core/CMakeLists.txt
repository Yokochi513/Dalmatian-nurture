# 育成ロジック。標準C++ と nlohmann/json のみに依存する（ADR 0009）
add_library(dalmatian_core STATIC
    placeholder.cpp)
target_include_directories(dalmatian_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(dalmatian_core PUBLIC nlohmann_json::nlohmann_json)
dal_set_compiler_options(dalmatian_core)
