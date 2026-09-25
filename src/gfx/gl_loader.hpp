#pragma once

namespace dal::gfx {

using GlProc = void (*)();
using GlProcLoader = GlProc (*)(const char* name);

// OpenGL 4.6 Core の関数を読み込む。コンテキストを作った後に1回だけ呼ぶ
bool load_gl(GlProcLoader loader);

} // namespace dal::gfx
