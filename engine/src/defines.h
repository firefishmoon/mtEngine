#pragma once
#ifdef MT_EXPORTS
#  define MT_API __declspec(dllexport)   // 编译 DLL 时用
#else
#  define MT_API __declspec(dllimport)   // 调用 DLL 时用
#endif

using b8 = bool;
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
using s8 = char;
using s16 = short;
using s32 = int;
using s64 = long long;
using f32 = float;
using f64 = double;


// #ifdef __cplusplus
// extern "C" {          // 关闭 C++ 名字改编
// #endif
//
// MT_API int __cdecl add(int a, int b);
//
// #ifdef __cplusplus
// }
// #endif
