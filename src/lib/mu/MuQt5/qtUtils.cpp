//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuQt5/qtUtils.h>
#include <Mu/Thread.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/ExceptionType.h>
#include <Mu/ParameterVariable.h>
#include <Mu/Type.h>
#include <iostream>
#include <sstream>
#include <typeinfo>

namespace Mu
{
    using namespace std;

    Pointer assertNotNil(Thread& thread, const Node& node, Pointer p, size_t n)
    {
        if (!p)
        {
            MuLangContext* context = (MuLangContext*)thread.context();
            const Symbol* s = node.symbol();
            const Function* f = dynamic_cast<const Function*>(s);

            ostringstream str;
            str << "Argument " << n;

            if (f && f->hasParameters())
            {
                str << " (" << f->parameter(n)->name() << " of type "
                    << f->parameter(n)->storageClass()->fullyQualifiedName()
                    << ")";
            }

            str << " in function " << s->fullyQualifiedName()
                << ": unexpected nil argument";

            ExceptionType::Exception* e =
                new ExceptionType::Exception(context->exceptionType());
            e->string() += str.str().c_str();
            thread.setException(e);
            ProgramException exc(thread);
            exc.message() = str.str().c_str();
            throw exc;
        }

        return p;
    }

    Class::ClassVector vectorOf2(Class* a, Class* b)
    {
        Class::ClassVector array(2);
        array[0] = a;
        array[1] = b;
        return array;
    }

    bool isMuQtObject(QObject* o)
    {
        //
        //  This is a pretty poor way to do this, but it works
        //

        return o ? strstr(typeid(*o).name(), "MuQt") != 0 : false;
    }

    bool isMuQtLayoutItem(QLayoutItem* o)
    {
        return o ? strstr(typeid(*o).name(), "MuQt") != 0 : false;
    }

    bool isMuQtPaintDevice(QPaintDevice* o)
    {
        return o ? strstr(typeid(*o).name(), "MuQt") != 0 : false;
    }

#if 0
static MuQtQObjectFinalizer(void* obj, void* data)
{
    ClassInstance* i = reinterpret_cast<ClassInstance*>(obj);
    Pointer* pp = i->data<Pointer>();
    if (*pp == this) *pp = 0;
}


NODE_IMPLEMENTATION(__allocate_register_gc_qobject, Pointer)
{
    const Class* c = static_cast<const Class*>(NODE_THIS.symbol()->scope());
    ClassInstance* i = ClassInstance::allocate(c);
    GC_register_finalizer(o, MuQtQObjectFinalizer, 0, 0, 0);
}
#endif

} // namespace Mu
