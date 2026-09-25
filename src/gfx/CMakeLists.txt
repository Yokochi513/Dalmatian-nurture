# OpenGL描画。GLFW には依存せず、OpenGL関数のローダは app から受け取る（ADR 0009）
add_library(dalmatian_gfx STATIC
    gl_loader.cpp
    renderer.cpp)
target_include_directories(dalmatian_gfx PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(dalmatian_gfx
    PUBLIC dalmatian_anim glad glm::glm)
dal_set_compiler_options(dalmatian_gfx)
