# 自前のターゲットにだけ適用するコンパイラ設定（ADR 0005）。
# 依存ライブラリには適用しない。
function(dal_set_compiler_options target)
    if(MSVC)
        # /utf-8: ソースの日本語コメントを、システムのコードページに関係なく UTF-8 として読む
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# 実行ファイルに UTF-8 のコードページを使うマニフェストを埋め込む
function(dal_use_utf8_code_page target)
    if(WIN32)
        target_sources(${target} PRIVATE "${PROJECT_SOURCE_DIR}/cmake/utf8.manifest")
    endif()
endfunction()
