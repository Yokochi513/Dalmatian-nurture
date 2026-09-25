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
