#pragma once
#ifdef MT_EXPORTS
#  define MT_API __declspec(dllexport)   // 编译 DLL 时用
#else
#  define MT_API __declspec(dllimport)   // 调用 DLL 时用
#endif

#ifdef __cplusplus
extern "C" {          // 关闭 C++ 名字改编
#endif

MT_API int __cdecl add(int a, int b);

#ifdef __cplusplus
}
#endif