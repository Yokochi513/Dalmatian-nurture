# glTF読込・スケルトン・アニメーション計算。OpenGL に依存しない（ADR 0009）
add_library(dalmatian_anim STATIC
    cgltf_impl.cpp)
target_include_directories(dalmatian_anim PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(dalmatian_anim
    PUBLIC glm::glm
    PRIVATE cgltf)
dal_set_compiler_options(dalmatian_anim)
