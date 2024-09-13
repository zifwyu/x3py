#ifndef X3_EXAMPLE_SIMPLE_IMPL_H
#define X3_EXAMPLE_SIMPLE_IMPL_H

#include <module/classmacro.h>
#include <plsimple/isimple.h>

class CSimpleA : public ISimple
{
    //X3BEGIN_CLASS_DECLARE(CSimpleA, clsidSimple)
    //    X3DEFINE_INTERFACE_ENTRY(ISimple)
    //X3END_CLASS_DECLARE()
    //////////// 宏展开 //////////////
public: 
    static const char* _getClassID() 
    {
        return clsidSimple;
    } 
    static const char* _getClassName() 
    {
        return "CSimpleA";
    } 
    static bool _queryObject(const CSimpleA* self, long iid, x3::IObject** p) 
    {
        if (iid == ISimple::getIID())
        {
            if (self) 
            {
                *p = (x3::IObject*)(ISimple*)(self); 
                (*p)->retainObject();
            } 
            return true;
        }
        return false;
    } 
    protected:
    //////////// 宏展开 结束//////////////
protected:
    CSimpleA();
    virtual ~CSimpleA();

private:
    virtual int add(int a, int b) const;
    virtual int subtract(int a, int b) const;
};

#endif