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

static std::vector<HMODULE> _plns; //��̬vector�������洢�Ѿ����صĲ��ģ��ľ��
static long                 _loading = 0;

///////////// ��չ��////////////
//XBEGIN_DEFINE_MODULE()
//    XDEFINE_CLASSMAP_ENTRY_Singleton(CManager)
//XEND_DEFINE_MODULE_DLL()
///////////// ��չ��////////////
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
///////////// ��չ��////////////

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
* @brief ���ز��DLL
* @param[in] filename ���DLL�ľ���·��
* @param[in] ext ��չ��У�飬ֻ��extΪ".pln"���Ż����
* @return һֱΪtrue
* @todo ��������ʱ���Ƿ�Ӧ����false
* @note 
*/
static bool loadfilter(const char* filename, const char* ext)
{
    // _stricmp(ext, ".pln") �ַ����Ƚϣ����ַ�����ȷ���0�������Ƕ���չ����У��
    // PathFindFileNameA(filename) �����ļ�����·������ȡ�ļ���
    // GetModuleHandleA ��ȡĳ����ľ��������NULL˵��û�м��ظò��
    if (_stricmp(ext, ".pln") == 0
        && GetModuleHandleA(PathFindFileNameA(filename)) == NULL)
    {
        HMODULE hmod = x3LoadLibrary(filename);

        // GetProcAddress ����DLL����ͺ������ƣ��ҵ�DLL�е��������ĺ�����ַ        
        if (hmod && GetProcAddress(hmod, "x3InitializePlugin"))
        {
            // ���DLL�д��ڵ�������"x3InitializePlugin"���Ż����
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
* @brief ���ع�����ģ��·��/folder/�µ����в��
* @param[in] folder �������·��
* @return ���ؼ��ز��������
*/
int CManager::loadExtraPlugins(const char* folder)
{
    char path[MAX_PATH];
    LockCount locker(&_loading);
    int from = (int)_plns.size();
    int ret = 0;

    //·�������õ���������ļ��еľ���·��
    GetModuleFileNameA(getModuleHandle(), path, MAX_PATH);
    PathRemoveFileSpecA(path);
    PathRemoveFileSpecA(path);
    PathAppendA(path, folder);

    x3::scanfiles(loadfilter, path, true);
    locker.Unlock();

    //�����ո��¼��صĲ��ģ����
    for (int i = from; i < (int)_plns.size(); i++)
    {
        //��ȡ����е�������"x3InitializePlugin"�ĺ���ָ��
        using INITF = bool (*)();
        INITF init = (INITF)GetProcAddress(_plns[i], "x3InitializePlugin");

        //����ָ����� �ҵ��øú�������false���ͽ�����ͷ�
        // ����ÿ������ձ����أ��ͻ��һ����"x3InitializePlugin"����
        if (init && !init())
        {
            x3FreeLibrary(_plns[i]);
            _plns[i] = NULL;
        }
        else
        {
            // Q ����������ѽ��������ָ�벻���ڣ�Ҳ��������߼���
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
* @brief ����ĳ·���µ����в��ģ��
* @param[in] folder ������ڵ��ļ������ƣ�ֻ��һ���ļ������ƣ����Ǿ���·����
*/
OUTAPI int x3LoadPlugins(const char* folder)
{
    //��IPlugins�ӿڵ�ʵ�����У��ҵ�UUIDΪclsidManager���࣬����ʵ��������ȡ��ʵ���������ָ��
    Object<IPlugins> reg(clsidManager);

    // ��reg��Ϊ�գ������������loadExtraPlugins�������ز�������ؼ��ز������������regΪ�գ�����0
    return SafeCallIf(reg, loadExtraPlugins(folder), 0);
}

OUTAPI int x3UnloadPlugins()
{
    Object<IPlugins> reg(clsidManager);
    return SafeCallIf(reg, unloadExtraPlugins(), 0);
}

END_NAMESPACE_X3
