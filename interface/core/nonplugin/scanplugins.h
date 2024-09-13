// x3py framework: https://github.com/rhcad/x3py
#ifndef X3_NONPLUGIN_SCANPLUGINS_H
#define X3_NONPLUGIN_SCANPLUGINS_H

#include <utilfunc/scanfiles.h>
#include <utilfunc/loadmodule.h>

#if defined(X3_CORE_PORTABILITY_H) && !defined(X3_EXCLUDE_CREATEOBJECT)
#include <portability/portimpl.h>
#endif

#ifndef PLUGINS_MAXCOUNT
#define PLUGINS_MAXCOUNT 40
#endif

namespace x3 {

static HMODULE  s_modules[PLUGINS_MAXCOUNT] = { NULL }; // 静态数组，存储DLL的句柄
static int      s_nmods = 0; //静态变量，表示s_modules数组中的元素个数

/**
* @brief 加载DLL
* @param[in] filename 要加载DLL的绝对路径
* @param[in] ext 对DLL做后缀名校验，只有后缀与ext相同才加载
* @return 返回DLL句柄数组中元素是否已满，若已满，返回false。未满返回true
*/
static bool loadfilter(const char* filename, const char* ext)
{
    // _stricmp 不区分大小写比较两个字符串，相等则返回0
    // PathFindFileNameA 由文件路径获得文件名
    // GetModuleHandleA 返回指定模块的句柄
    if (_stricmp(ext, ".pln") == 0
        && GetModuleHandleA(PathFindFileNameA(filename)) == NULL)
    {
        // 如果模块句柄为NULL，则显式加载此模块
        s_modules[s_nmods] = x3LoadLibrary(filename);

        // 若加载成功，则句柄数组中元素数量加1
        if (s_modules[s_nmods])
            s_nmods++;
    }
    return s_nmods < PLUGINS_MAXCOUNT;
}

int loadScanPlugins(const char* folder = "plugins")
{
    char path[MAX_PATH];

    //获取exe可执行文件的绝对路径
    GetModuleFileNameA(NULL, path, MAX_PATH);

    //从路径中删除尾随文件名和反斜杠（如果存在）
    PathRemoveFileSpecA(path);

    //字符串拼接，将folder拼接到path后面
    PathAppendA(path, folder);

    // Load x3manager before others, so others can find it in x3InitPlugin().
    // 最开始要先加载x3manager，这样才能管理后续插件

    // 获取模块管理器的句柄，如果句柄为NULL，就加载模块管理器
    
    //GetModuleHandleA 返回指定模块的句柄
    if (!GetModuleHandleA("x3manager.pln"))
    {
        //字符串拼接，将"x3manager.pln"拼接到path后面
        PathAppendA(path, "x3manager.pln");
        loadfilter(path, ".pln");

        //从路径中删除尾随文件名和反斜杠（如果存在）
        PathRemoveFileSpecA(path);
    }

    // 定义一个函数指针
    using LOADF = int (*)(const char*);

    // GetProcAddress 根据DLL句柄和函数名称，找到DLL中导出函数的函数地址
    LOADF fload = (LOADF)GetProcAddress(GetModuleHandleA("x3manager.pln"), "x3LoadPlugins");

    // 若函数指针不为空，则运行该函数，即调用模块管理器中的x3LoadPlugins函数，参数为所有模块DLL所在的文件夹路径
    int extcount = fload ? fload(folder) : 0;
    
    if (!fload) // 无论 x3manager 插件如何，都加载插件。
        x3::scanfiles(loadfilter, path, true);

    return s_nmods + extcount;
}

void unloadScanPlugins()
{
    typedef int (*UF)();
    UF uf = (UF)GetProcAddress(GetModuleHandleA("x3manager.pln"), "x3UnloadPlugins");
    if (uf) uf();

    while (s_nmods > 0)
    {
        x3FreeLibrary(s_modules[--s_nmods]);
    }
}

#if !defined(X3_EXCLUDE_CREATEOBJECT) && !defined(CREATEOBJECTIMPL)
#define CREATEOBJECTIMPL

class IObject;
/**
* @brief 创建接口的实现类的对象
* @param[in] clsid 实现类的UUID
* @param[in] iid 接口的唯一标识
* @param[out] 创建出的实现类的指针的指针
*/
LOCALAPI bool createObject(const char* clsid, long iid, IObject** p)
{
    using F = bool (*)(const char*, long, IObject**);

    // Q s_modules[0]应该一直是插件管理器的句柄？
    // 这是是调用插件管理器的"x3CreateObject"函数？
    F f = (F)GetProcAddress(s_modules[0], "x3CreateObject");
    return f && f(clsid, iid, p);
}
HMODULE getManagerModule() { return s_modules[0]; }
#endif // CREATEOBJECTIMPL

} // x3
#endif