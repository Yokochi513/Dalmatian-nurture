#include "gfx/gl_loader.hpp"
#include "gfx/renderer.hpp"
#include "physics/jolt_runtime.hpp"
#include "platform/window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>
#include <exception>

int main()
{
    try {
        dal::platform::Window window(1280, 720, "Dalmatian");
        if (!dal::gfx::load_gl(dal::platform::gl_proc_address)) {
            std::fprintf(stderr, "OpenGL 4.6 の関数を読み込めませんでした\n");
            return 1;
        }

        const dal::physics::JoltRuntime jolt;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window.native_handle(), true);
        ImGui_ImplOpenGL3_Init("#version 460");

        while (!window.should_close()) {
            window.poll_events();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGui::Begin("Dalmatian");
            ImGui::Text("%.1f FPS", static_cast<double>(ImGui::GetIO().Framerate));
            ImGui::End();
            ImGui::Render();

            int width = 0;
            int height = 0;
            window.framebuffer_size(width, height);
            dal::gfx::begin_frame(width, height);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            window.swap_buffers();
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
