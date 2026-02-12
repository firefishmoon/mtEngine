#pragma once
// #if defined(_WIN32) || defined(_WIN64)
// #include <windows.h>

#include "defines.h"
struct GLFWwindow;

struct mtPlatformData {
    // HINSTANCE hInstance;
    // HWND hwnd;
    GLFWwindow* window;
    // u32 wndWidth;
    // u32 wndHeight;
};

// #endif
