#pragma once
#if defined(_WIN32) || defined(_WIN64)
// 这里是 Windows（32 位或 64 位）
#include <windows.h>

struct mtPlatformData {
    HINSTANCE hInstance;
    HWND hwnd;
};

#endif
