#define MT_EXPORTS          // 必须在 #include "mylib.h" 前定义
#include "test.h"
#include <iostream>

int __cdecl add(int a, int b) { return a + b; }