//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

static Pointer findChild_Object_string(Thread& NODE_THREAD, Pointer obj, Pointer p)
{
    ClassInstance* self = reinterpret_cast<ClassInstance*>(obj);
    StringType::String* name = reinterpret_cast<StringType::String*>(p);
    QObject* o = object<QObject>(self);
    ClassInstance* rval = ClassInstance::allocate(self->classType());
    setobject(rval, o->findChild<QObject*>(name->c_str()));
    return rval;
}

Pointer qt_QObject_tr_string_QObject_string_string_int(Mu::Thread& NODE_THREAD, Pointer sourceText, Pointer disambiguation, int n)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QString arg0 = qstring(sourceText);
    QString arg1 = qstring(disambiguation);
    int arg2 = n;
    return makestring(c, QObject::tr(arg0.toStdString().c_str(), arg1.toStdString().c_str(), arg2));
}

static NODE_IMPLEMENTATION(findChild, Pointer)
{
    NODE_RETURN(findChild_Object_string(NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_tr0, Pointer)
{
    NODE_RETURN(
        qt_QObject_tr_string_QObject_string_string_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, int)));
}

#include <MuQt6/QVariantType.h>

static bool setProperty_bool_Object_string_qt_QVariant(Thread& NODE_THREAD, Pointer obj, Pointer n, Pointer v)
{
    ClassInstance* self = reinterpret_cast<ClassInstance*>(obj);
    StringType::String* name = reinterpret_cast<StringType::String*>(n);
    const QVariant value = getqtype<QVariantType>(v);
    QObject* o = object<QObject>(self);
    return o->setProperty(name->c_str(), value);
}

static NODE_IMPLEMENTATION(setProperty, bool)
{
    NODE_RETURN(setProperty_bool_Object_string_qt_QVariant(NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, Pointer)));
}
