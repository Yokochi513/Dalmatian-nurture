# core・anim・physics の単体テスト（ADR 0009・0015）
add_executable(dalmatian_tests
    core/fake_clock.hpp
    core/needs_test.cpp
    core/simulation_test.cpp
    physics/jolt_runtime_test.cpp)
target_link_libraries(dalmatian_tests PRIVATE
    dalmatian_core
    dalmatian_anim
    dalmatian_physics
    Catch2::Catch2WithMain)
dal_set_compiler_options(dalmatian_tests)
dal_use_utf8_code_page(dalmatian_tests)

include(Catch)
catch_discover_tests(dalmatian_tests)
