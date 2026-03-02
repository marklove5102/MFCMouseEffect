#include "Platform/macos/Shell/MacosEventLoopService.h"

#include <cstdio>

int main() {
    mousefx::MacosEventLoopService loop;
    if (!loop.PostTask([&loop]() {
            loop.RequestExit();
        })) {
        std::fprintf(stderr, "mfx_shell_macos_smoke: failed to post exit task\n");
        return 2;
    }
    return loop.Run();
}
