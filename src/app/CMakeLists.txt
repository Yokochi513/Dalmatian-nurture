# main・ゲームループ・シーン・ImGui（ADR 0009・0013）
add_executable(dalmatian
    main.cpp)
target_link_libraries(dalmatian PRIVATE
    dalmatian_core
    dalmatian_gfx
    dalmatian_physics
    dalmatian_platform
    imgui)
dal_set_compiler_options(dalmatian)
dal_use_utf8_code_page(dalmatian)

# アセットとシェーダを実行ファイルの隣へコピーする（platform/paths.md）。
# ディレクトリを追加したときは CMake の構成をやり直す
foreach(dir assets shaders)
    if(EXISTS "${PROJECT_SOURCE_DIR}/${dir}")
        add_custom_command(TARGET dalmatian POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${PROJECT_SOURCE_DIR}/${dir}" "$<TARGET_FILE_DIR:dalmatian>/${dir}")
    endif()
endforeach()
