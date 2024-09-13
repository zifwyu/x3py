#include <module/plugininc.h>
#include <module/pluginimpl.h>
#define XUSE_LIB_PLUGIN
#include <module/modulemacro.h>

#include "libspln.h"

//XBEGIN_DEFINE_MODULE()
//    XDEFINE_CLASSMAP_ENTRY(CSimple3)
//XEND_DEFINE_MODULE_DLL()
static const x3::ClassEntry s_classes[] = {
    x3::ClassEntry(1, "NormalObject<" "CSimple3" ">", CSimple3::_getClassID(), (x3::ObjectCreator)(&x3::NormalObject<CSimple3>::create), (x3::HASIID)(&x3::NormalObject<CSimple3>::hasInterface)),
    x3::ClassEntry() }; 

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

#ifdef _MSC_VER
#pragma comment(lib, "libpln1.lib")
#pragma comment(lib, "libpln2.lib")
#endif

extern const x3::ClassEntry* const classes_libpln1;
extern const x3::ClassEntry* const classes_libpln2;

const x3::ClassEntry* const x3::ClassEntry::classes[] = {
        s_classes, classes_libpln1, classes_libpln2, NULL
    };

OUTAPI bool x3InitializePlugin()
{
    return true;
}

OUTAPI void x3UninitializePlugin()
{
}
