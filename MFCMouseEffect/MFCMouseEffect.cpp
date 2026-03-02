#include "pch.h"

#include "Platform/PlatformEntryRunner.h"

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    return mousefx::platform::RunPlatformEntry();
}
