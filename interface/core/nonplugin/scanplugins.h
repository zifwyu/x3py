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

static HMODULE  s_modules[PLUGINS_MAXCOUNT] = { NULL }; // ��̬���飬�洢DLL�ľ��
static int      s_nmods = 0; //��̬��������ʾs_modules�����е�Ԫ�ظ���

/**
* @brief ����DLL
* @param[in] filename Ҫ����DLL�ľ���·��
* @param[in] ext ��DLL����׺��У�飬ֻ�к�׺��ext��ͬ�ż���
* @return ����DLL���������Ԫ���Ƿ�������������������false��δ������true
*/
static bool loadfilter(const char* filename, const char* ext)
{
    // _stricmp �����ִ�Сд�Ƚ������ַ���������򷵻�0
    // PathFindFileNameA ���ļ�·������ļ���
    // GetModuleHandleA ����ָ��ģ��ľ��
    if (_stricmp(ext, ".pln") == 0
        && GetModuleHandleA(PathFindFileNameA(filename)) == NULL)
    {
        // ���ģ����ΪNULL������ʽ���ش�ģ��
        s_modules[s_nmods] = x3LoadLibrary(filename);

        // �����سɹ�������������Ԫ��������1
        if (s_modules[s_nmods])
            s_nmods++;
    }
    return s_nmods < PLUGINS_MAXCOUNT;
}

int loadScanPlugins(const char* folder = "plugins")
{
    char path[MAX_PATH];

    //��ȡexe��ִ���ļ��ľ���·��
    GetModuleFileNameA(NULL, path, MAX_PATH);

    //��·����ɾ��β���ļ����ͷ�б�ܣ�������ڣ�
    PathRemoveFileSpecA(path);

    //�ַ���ƴ�ӣ���folderƴ�ӵ�path����
    PathAppendA(path, folder);

    // Load x3manager before others, so others can find it in x3InitPlugin().
    // �ʼҪ�ȼ���x3manager���������ܹ���������

    // ��ȡģ��������ľ����������ΪNULL���ͼ���ģ�������
    
    //GetModuleHandleA ����ָ��ģ��ľ��
    if (!GetModuleHandleA("x3manager.pln"))
    {
        //�ַ���ƴ�ӣ���"x3manager.pln"ƴ�ӵ�path����
        PathAppendA(path, "x3manager.pln");
        loadfilter(path, ".pln");

        //��·����ɾ��β���ļ����ͷ�б�ܣ�������ڣ�
        PathRemoveFileSpecA(path);
    }

    // ����һ������ָ��
    using LOADF = int (*)(const char*);

    // GetProcAddress ����DLL����ͺ������ƣ��ҵ�DLL�е��������ĺ�����ַ
    LOADF fload = (LOADF)GetProcAddress(GetModuleHandleA("x3manager.pln"), "x3LoadPlugins");

    // ������ָ�벻Ϊ�գ������иú�����������ģ��������е�x3LoadPlugins����������Ϊ����ģ��DLL���ڵ��ļ���·��
    int extcount = fload ? fload(folder) : 0;
    
    if (!fload) // ���� x3manager �����Σ������ز����
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
* @brief �����ӿڵ�ʵ����Ķ���
* @param[in] clsid ʵ�����UUID
* @param[in] iid �ӿڵ�Ψһ��ʶ
* @param[out] ��������ʵ�����ָ���ָ��
*/
LOCALAPI bool createObject(const char* clsid, long iid, IObject** p)
{
    using F = bool (*)(const char*, long, IObject**);

    // Q s_modules[0]Ӧ��һֱ�ǲ���������ľ����
    // �����ǵ��ò����������"x3CreateObject"������
    F f = (F)GetProcAddress(s_modules[0], "x3CreateObject");
    return f && f(clsid, iid, p);
}
HMODULE getManagerModule() { return s_modules[0]; }
#endif // CREATEOBJECTIMPL

} // x3
#endif