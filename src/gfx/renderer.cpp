#include "gfx/renderer.hpp"

#include <glad/gl.h>

namespace dal::gfx {

void begin_frame(int width, int height)
{
    glViewport(0, 0, width, height);
    glClearColor(0.53f, 0.71f, 0.86f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

} // namespace dal::gfx
