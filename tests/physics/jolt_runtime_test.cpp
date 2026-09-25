#include "physics/jolt_runtime.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("JoltRuntime は初期化と後始末を繰り返せる")
{
    { const dal::physics::JoltRuntime runtime; }
    { const dal::physics::JoltRuntime runtime; }
    SUCCEED();
}
