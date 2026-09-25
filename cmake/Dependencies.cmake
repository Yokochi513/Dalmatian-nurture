# 依存ライブラリの取得（ADR 0005・0006・0014）。
# FetchContent で取得し、公式の CMake ターゲットがないものはここでターゲットを定義する。
include(FetchContent)

# --- GLFW（ウィンドウ・入力） ---
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE)

# --- glm（行列・ベクトル演算） ---
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE)

# --- nlohmann/json（セーブデータ） ---
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)

# --- Jolt Physics（当たり判定） ---
# 他のターゲットと同じく動的ランタイム（/MD）を使う
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
FetchContent_Declare(JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG v5.6.0
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR Build)

FetchContent_MakeAvailable(glfw glm nlohmann_json JoltPhysics)

# --- 公式の CMake ターゲットを使わないもの ---
# SOURCE_SUBDIR に存在しないディレクトリを指定し、取得だけを行う（add_subdirectory させない）
FetchContent_Declare(cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG v1.15
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _fetch_only)
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
    SOURCE_SUBDIR _fetch_only)
FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.25
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _fetch_only)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _fetch_only)
FetchContent_MakeAvailable(cgltf stb miniaudio imgui)

# ヘッダオンリのライブラリ。実装（*_IMPLEMENTATION）は使う側のターゲットの .cpp で1回だけ展開する
add_library(cgltf INTERFACE)
target_include_directories(cgltf SYSTEM INTERFACE "${cgltf_SOURCE_DIR}")

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE "${stb_SOURCE_DIR}")

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio SYSTEM INTERFACE "${miniaudio_SOURCE_DIR}")

# Dear ImGui（公式の CMakeLists.txt がないため、本体と backend を列挙する。ADR 0006）
add_library(imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
target_include_directories(imgui SYSTEM PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends")
target_link_libraries(imgui PUBLIC glfw)

# glad（生成物を third_party/glad にコミット済み。ADR 0006）
add_library(glad STATIC "${PROJECT_SOURCE_DIR}/third_party/glad/src/gl.c")
target_include_directories(glad SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/third_party/glad/include")

# --- テスト ---
if(DAL_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.16.0
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
