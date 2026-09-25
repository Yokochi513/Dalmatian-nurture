#include "gfx/gl_loader.hpp"

#include <glad/gl.h>

namespace dal::gfx {

bool load_gl(GlProcLoader loader)
{
    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(loader));
    return GLAD_VERSION_MAJOR(version) > 4
        || (GLAD_VERSION_MAJOR(version) == 4 && GLAD_VERSION_MINOR(version) >= 6);
}

} // namespace dal::gfx
