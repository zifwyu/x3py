// Included by portimpl.h to implement functions on Windows.

#ifndef X3WIN_PORTABILITY_IMPL_H
#define X3WIN_PORTABILITY_IMPL_H

#include "x3win.h"

bool x3FreeLibrary(HMODULE hdll)
{
#ifdef __GNUC__
    typedef void (*F)();
    F f = (F)GetProcAddress(hdll, "x3FreePlugin");
    if (f) f();
#endif
    return !!FreeLibrary(hdll);
}

/**
* @brief 显示加载DLL，并返回其句柄
* @param[in] filename DLL文件名
* @return 加载成功，则返回其句柄；加载失败，则返回NULL
*/
HMODULE x3LoadLibrary(const char* filename)
{
    // LoadLibraryExA 显示加载DLL
    HMODULE hdll = LoadLibraryExA(filename, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

#ifdef __GNUC__
    typedef bool (*F)(HMODULE, HMODULE);
    F f = (F)GetProcAddress(hdll, "x3InitPlugin");

    if (f && !f(hdll, NULL))
    {
        x3FreeLibrary(hdll);
        hdll = NULL;
    }
#endif

    return hdll;
}

#endif