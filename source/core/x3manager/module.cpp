// x3py framework: https://github.com/rhcad/x3py
#include <module/plugininc.h>
#include <module/modulemacro.h>
#define X3_EXCLUDE_CREATEOBJECT
#include <module/pluginimpl.h>

#include <utilfunc/safecall.h>
#include <utilfunc/scanfiles.h>
#include <utilfunc/loadmodule.h>
#include <utilfunc/lockcount.h>

#include "plugins.h"
#include "workpath.h"


BEGIN_NAMESPACE_X3

class CManager : public CPlugins, public CWorkPath
{
    X3BEGIN_CLASS_DECLARE(CManager, clsidManager)
        X3USE_INTERFACE_ENTRY(CPlugins)
        X3USE_INTERFACE_ENTRY(CWorkPath)
    X3END_CLASS_DECLARE()
protected:
    CManager() {}

private:
    virtual int loadExtraPlugins(const char* folder);
    virtual int unloadExtraPlugins();
};

static std::vector<HMODULE> _plns; //静态vector变量，存储已经加载的插件模块的句柄
static long                 _loading = 0;

///////////// 宏展开////////////
//XBEGIN_DEFINE_MODULE()
//    XDEFINE_CLASSMAP_ENTRY_Singleton(CManager)
//XEND_DEFINE_MODULE_DLL()
///////////// 宏展开////////////
static const x3::ClassEntry s_classes[] = 
{
    x3::ClassEntry(x3::MIN_SINGLETON_TYPE, "SingletonObject<" "CManager" ">", CManager::_getClassID(), (x3::ObjectCreator)(&x3::SingletonObject<CManager>::create), (x3::HASIID)(&x3::SingletonObject<CManager>::hasInterface)),
    x3::ClassEntry() 
}; 
const x3::ClassEntry* const x3::ClassEntry::classes[] = 
{ 
    s_classes, 0 
}; 
extern "C" BOOL __stdcall DllMain(HANDLE hmod, DWORD dwReason, LPVOID) 
{
    if (dwReason == 1) 
    {
        return ::x3InitPlugin((HMODULE)hmod, 0);
    }
    else if (dwReason == 0) 
    {
        ::x3FreePlugin();
    } 
    return 1;
}
///////////// 宏展开////////////

OUTAPI bool x3InitializePlugin()
{
    return true;
}

OUTAPI void x3UninitializePlugin()
{
    Object<IPlugins> reg(clsidManager);
    SafeCall(reg, unloadExtraPlugins());
}

OUTAPI bool x3RegisterPlugin(Creator creator, HMODULE hmod, const char** clsids)
{
    bool needInit = (_loading == 0);
    Object<IRegister> reg(clsidManager);

    SafeCall(reg, registerPlugin(creator, hmod, clsids));

    return needInit;
}

OUTAPI bool x3UnregisterPlugin(Creator creator)
{
    bool needFree = (_loading == 0);
    Object<IRegister> reg(clsidManager);

    if (reg && creator)
        reg->unregisterPlugin(creator);

    return needFree;
}

OUTAPI bool x3CreateObject(const char* clsid, long iid, IObject** p)
{
    if (x3InternalCreate(clsid, iid, p))
        return true;
    Object<IRegister> reg(clsidManager);
    return reg && reg->createFromOthers(clsid, iid, p);
}

OUTAPI bool x3RegisterObserver(const char* type, PROC handler, Creator creator)
{
    Object<IRegister> reg(clsidManager);
    return reg && reg->registerObserver(type, handler, creator);
}

OUTAPI bool x3RegisterObserverObject(const char* type, ObserverObject* obj, 
                                     ON_EVENT handler, Creator creator)
{
    Object<IRegister> reg(clsidManager);
    return reg && reg->registerObserver(type, obj, handler, creator);
}

OUTAPI void x3UnregisterObserverObject(ObserverObject* obj)
{
    Object<IRegister> reg(clsidManager);
    if (reg && obj)
        reg->unregisterObserver(obj);
}

OUTAPI int x3FireEvent(const char* type, EventDispatcher dispatcher, void* data)
{
    Object<IRegister> reg(clsidManager);
    return SafeCallIf(reg, fireEvent(type, dispatcher, data), 0);
}

OUTAPI int x3FireObjectEvent(const char* type, ObjectEventDispatcher dispatcher, void* data)
{
    Object<IRegister> reg(clsidManager);
    return SafeCallIf(reg, fireEvent(type, dispatcher, data), 0);
}

#ifndef _WIN32
OUTAPI HMODULE unixFindModule(const char* filename)
{
    Object<IRegister> reg(clsidManager);
    return SafeCallIf(reg, findModuleByFileName(filename), NULL);
}
#endif // UNIX

/**
* @brief 加载插件DLL
* @param[in] filename 插件DLL的绝对路径
* @param[in] ext 扩展名校验，只有ext为".pln"，才会加载
* @return 一直为true
* @todo 当不加载时，是否应返回false
* @note 
*/
static bool loadfilter(const char* filename, const char* ext)
{
    // _stricmp(ext, ".pln") 字符串比较，两字符串相等返回0，这里是对扩展名做校验
    // PathFindFileNameA(filename) 根据文件绝对路径，获取文件名
    // GetModuleHandleA 获取某插件的句柄，返回NULL说明没有加载该插件
    if (_stricmp(ext, ".pln") == 0
        && GetModuleHandleA(PathFindFileNameA(filename)) == NULL)
    {
        HMODULE hmod = x3LoadLibrary(filename);

        // GetProcAddress 根据DLL句柄和函数名称，找到DLL中导出函数的函数地址        
        if (hmod && GetProcAddress(hmod, "x3InitializePlugin"))
        {
            // 插件DLL中存在导出函数"x3InitializePlugin"，才会加载
            _plns.push_back(hmod);
        }            
        else if (hmod)
        {
            x3FreeLibrary(hmod);
        }           
    }
    return true;
}

/**
* @brief 加载管理器模块路径/folder/下的所有插件
* @param[in] folder 插件所在路径
* @return 返回加载插件的数量
*/
int CManager::loadExtraPlugins(const char* folder)
{
    char path[MAX_PATH];
    LockCount locker(&_loading);
    int from = (int)_plns.size();
    int ret = 0;

    //路径处理，得到插件所在文件夹的绝对路径
    GetModuleFileNameA(getModuleHandle(), path, MAX_PATH);
    PathRemoveFileSpecA(path);
    PathRemoveFileSpecA(path);
    PathAppendA(path, folder);

    x3::scanfiles(loadfilter, path, true);
    locker.Unlock();

    //遍历刚刚新加载的插件模块句柄
    for (int i = from; i < (int)_plns.size(); i++)
    {
        //获取插件中导出函数"x3InitializePlugin"的函数指针
        using INITF = bool (*)();
        INITF init = (INITF)GetProcAddress(_plns[i], "x3InitializePlugin");

        //函数指针存在 且调用该函数返回false，就将插件释放
        // 所以每个插件刚被加载，就会调一次其"x3InitializePlugin"函数
        if (init && !init())
        {
            x3FreeLibrary(_plns[i]);
            _plns[i] = NULL;
        }
        else
        {
            // Q 这里有问题呀，若函数指针不存在，也会走这个逻辑？
            ret++;
        }
    }

    return ret;
}

int CManager::unloadExtraPlugins()
{
    LockCount locker(&_loading);
    int count = 0;

    std::vector<HMODULE>::reverse_iterator it = _plns.rbegin();
    for (; it != _plns.rend(); it++)
    {
        typedef bool (*UF)();
        UF f = (UF)GetProcAddress(*it, "x3UninitializePlugin");
        if (f) f();
    }

    while (!_plns.empty())
    {
        HMODULE hmod = _plns.back();
        _plns.pop_back();

        if (hmod)
        {
            x3FreeLibrary(hmod);
            count++;
        }
    }

    return count;
}

/**
* @brief 加载某路径下的所有插件模块
* @param[in] folder 插件所在的文件夹名称（只有一个文件夹名称，不是绝对路径）
*/
OUTAPI int x3LoadPlugins(const char* folder)
{
    //在IPlugins接口的实现类中，找到UUID为clsidManager的类，将其实例化，获取其实例化对象的指针
    Object<IPlugins> reg(clsidManager);

    // 若reg不为空，则调用其对象的loadExtraPlugins函数加载插件，返回加载插件的数量；若reg为空，返回0
    return SafeCallIf(reg, loadExtraPlugins(folder), 0);
}

OUTAPI int x3UnloadPlugins()
{
    Object<IPlugins> reg(clsidManager);
    return SafeCallIf(reg, unloadExtraPlugins(), 0);
}

END_NAMESPACE_X3
