# 当たり判定。Jolt を直接使うのはこのターゲットだけとし、公開インターフェースに Jolt の型を出さない（ADR 0015）
add_library(dalmatian_physics STATIC
    jolt_runtime.cpp)
target_include_directories(dalmatian_physics PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(dalmatian_physics
    PUBLIC glm::glm
    PRIVATE Jolt)
dal_set_compiler_options(dalmatian_physics)
