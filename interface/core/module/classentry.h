// x3py framework: https://github.com/rhcad/x3py
#ifndef X3_CORE_CLASSENTRY_H
#define X3_CORE_CLASSENTRY_H

#include "iobject.h"

BEGIN_NAMESPACE_X3

using ObjectCreator = IObject* (*)(long iid);
using HASIID = bool (*)(long iid);
enum { MIN_SINGLETON_TYPE = 10 };

class ClassEntry
{
public:
    int             type;
    const char*     className;
    const char*     clsid;
    ObjectCreator   creator;
    HASIID          hasiid;

    ClassEntry(int _type, const char* _className,
        const char* _clsid, ObjectCreator _creator, HASIID _hasiid)
        : type(_type), className(_className), clsid(_clsid)
        , creator(_creator), hasiid(_hasiid)
    {
    }

    ClassEntry() : type(0), className(""), clsid(""), creator(NULL), hasiid(NULL)
    {
    }

    // see XEND_DEFINE_MODULE, XEND_DEFINE_MODULE_LIB
    // 类的静态成员变量，是一个静态数组，数组中的元素是ClassEntry*
    // 第一个const说明数组中的成员不能变更，第二个const说明数组本身不能变更
    static const ClassEntry* const  classes[];
};

END_NAMESPACE_X3

// If don't need plugininc.h or portability/*.h on Windows:
#if !defined(OUTAPI) && defined(_WIN32)
#ifndef _WINDEF_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#define OUTAPI      extern "C" __declspec(dllexport)
#define LOCALAPI
#define x3FreeLibrary(h) FreeLibrary(h)
#define x3LoadLibrary(f) LoadLibraryExA(f, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)
#endif // OUTAPI

#endif