# core・anim・physics・platform の単体テスト（ADR 0009・0015・0044）
add_executable(dalmatian_tests
    core/fake_clock.hpp
    core/care_test.cpp
    core/growth_test.cpp
    core/tricks_test.cpp
    core/behavior_test.cpp
    core/save_test.cpp
    core/needs_test.cpp
    core/simulation_test.cpp
    physics/jolt_runtime_test.cpp
    platform/save_storage_test.cpp)
target_link_libraries(dalmatian_tests PRIVATE
    dalmatian_core
    dalmatian_anim
    dalmatian_physics
    dalmatian_platform
    Catch2::Catch2WithMain)
dal_set_compiler_options(dalmatian_tests)
dal_use_utf8_code_page(dalmatian_tests)

include(Catch)
catch_discover_tests(dalmatian_tests)
