//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

//
//  NOT the correct mangling here
//

struct StringBoolPair
{
    Pointer s;
    bool b;
};

static Pointer qt_QInputDialog_getText(Thread& NODE_THREAD,
                                       Pointer param_parent,
                                       Pointer param_title, Pointer param_label,
                                       int param_echoMode, Pointer param_text,
                                       int param_flags)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());

    QWidget* parent =
        object<QWidget>(reinterpret_cast<ClassInstance*>(param_parent));
    QString title = qstring(param_title);
    QString label = qstring(param_label);
    QLineEdit::EchoMode mode = (QLineEdit::EchoMode)param_echoMode;
    Qt::WindowFlags flags = (Qt::WindowFlags)param_flags;
    QString text = qstring(param_text);

    bool ok = false;
    QString result =
        QInputDialog::getText(parent, title, label, mode, text, &ok, flags);

    Context::TypeVector tt(2);
    tt[0] = c->stringType();
    tt[1] = c->boolType();

    ClassInstance* rval = ClassInstance::allocate(c->tupleType(tt));
    StringBoolPair* p = rval->data<StringBoolPair>();
    p->s = makestring(c, result);
    p->b = ok;

    return rval;
}

static NODE_IMPLEMENTATION(getText, Pointer)
{
    NODE_RETURN(qt_QInputDialog_getText(
        NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
        NODE_ARG(2, Pointer), NODE_ARG(3, int), NODE_ARG(4, Pointer),
        NODE_ARG(5, int)));
}
