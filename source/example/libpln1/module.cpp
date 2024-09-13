#include <module/plugininc.h>
#include <module/pluginimpl.h>
#include <module/modulemacro.h>

#include "libpln1.h"

XBEGIN_DEFINE_MODULE()
x3::ClassEntry(1, "NormalObject<" "CSimpleA" ">", CSimpleA::_getClassID(), (x3::ObjectCreator)(&x3::NormalObject<CSimpleA>::create), (x3::HASIID)(&x3::NormalObject<CSimpleA>::hasInterface)),
XEND_DEFINE_MODULE_LIB(libpln1)
