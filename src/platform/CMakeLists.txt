# OSとの境界：ウィンドウ・入力・システム時計・音声・保存先パス（ADR 0009）
# 時計は core のインターフェースを実装するため core に依存する（ADR 0042）
add_library(dalmatian_platform STATIC
    window.cpp
    system_clock.cpp
    miniaudio_impl.cpp)
target_include_directories(dalmatian_platform PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(dalmatian_platform
    PUBLIC dalmatian_core
    PRIVATE glfw miniaudio)
dal_set_compiler_options(dalmatian_platform)
