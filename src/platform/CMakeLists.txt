# OSとの境界：ウィンドウ・入力・システム時計・音声・置き場所・セーブファイル（ADR 0009）
# 時計とセーブは core のインターフェース・変換を使うため core に依存する（ADR 0042）
add_library(dalmatian_platform STATIC
    window.hpp
    window.cpp
    input.hpp
    input.cpp
    system_clock.hpp
    system_clock.cpp
    audio.hpp
    audio.cpp
    miniaudio_impl.cpp
    paths.hpp
    paths.cpp
    save_storage.hpp
    save_storage.cpp)
target_include_directories(dalmatian_platform PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(dalmatian_platform
    PUBLIC dalmatian_core
    PRIVATE glfw miniaudio shell32 ole32)
dal_set_compiler_options(dalmatian_platform)
