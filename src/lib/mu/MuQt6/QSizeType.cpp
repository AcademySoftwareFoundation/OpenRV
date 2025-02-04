//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

// IMPORTANT: This file (not the template) is auto-generated by qt2mu.py script.
//            The prefered way to do a fix is to handrolled it or modify the
//            qt2mu.py script. If it is not possible, manual editing is ok but
//            it could be lost in future generations.

#include <MuQt6/qtUtils.h>
#include <MuQt6/QSizeType.h>
#include <MuQt6/QActionType.h>
#include <MuQt6/QWidgetType.h>
#include <Mu/Alias.h>
#include <Mu/BaseFunctions.h>
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/Value.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtSvg/QtSvg>
#include <QSvgWidget>
#include <QtNetwork/QtNetwork>
#include <MuQt6/QMarginsType.h>

namespace Mu
{
    using namespace std;

    QSizeType::Instance::Instance(const Class* c)
        : ClassInstance(c)
    {
    }

    QSizeType::QSizeType(Context* c, const char* name, Class* super)
        : Class(c, name, super)
    {
    }

    QSizeType::~QSizeType() {}

    static NODE_IMPLEMENTATION(__allocate, Pointer)
    {
        QSizeType::Instance* i =
            new QSizeType::Instance((Class*)NODE_THIS.type());
        QSizeType::registerFinalizer(i);
        NODE_RETURN(i);
    }

    void QSizeType::registerFinalizer(void* o)
    {
        GC_register_finalizer(o, QSizeType::finalizer, 0, 0, 0);
    }

    void QSizeType::finalizer(void* obj, void* data)
    {
        QSizeType::Instance* i = reinterpret_cast<QSizeType::Instance*>(obj);
        delete i;
    }

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    Pointer qt_QSize_QSize_QSize_QSize(Mu::Thread& NODE_THREAD,
                                       Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        setqtype<QSizeType>(param_this, QSize());
        return param_this;
    }

    Pointer qt_QSize_QSize_QSize_QSize_int_int(Mu::Thread& NODE_THREAD,
                                               Pointer param_this,
                                               int param_width,
                                               int param_height)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        int arg1 = (int)(param_width);
        int arg2 = (int)(param_height);
        setqtype<QSizeType>(param_this, QSize(arg1, arg2));
        return param_this;
    }

    Pointer qt_QSize_boundedTo_QSize_QSize_QSize(Mu::Thread& NODE_THREAD,
                                                 Pointer param_this,
                                                 Pointer param_otherSize)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        const QSize arg1 = getqtype<QSizeType>(param_otherSize);
        return makeqtype<QSizeType>(c, arg0.boundedTo(arg1), "qt.QSize");
    }

    Pointer qt_QSize_expandedTo_QSize_QSize_QSize(Mu::Thread& NODE_THREAD,
                                                  Pointer param_this,
                                                  Pointer param_otherSize)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        const QSize arg1 = getqtype<QSizeType>(param_otherSize);
        return makeqtype<QSizeType>(c, arg0.expandedTo(arg1), "qt.QSize");
    }

    Pointer qt_QSize_grownBy_QSize_QSize_QMargins(Mu::Thread& NODE_THREAD,
                                                  Pointer param_this,
                                                  Pointer param_margins)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        QMargins arg1 = getqtype<QMarginsType>(param_margins);
        return makeqtype<QSizeType>(c, arg0.grownBy(arg1), "qt.QSize");
    }

    int qt_QSize_height_int_QSize(Mu::Thread& NODE_THREAD, Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        return arg0.height();
    }

    bool qt_QSize_isEmpty_bool_QSize(Mu::Thread& NODE_THREAD,
                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        return arg0.isEmpty();
    }

    bool qt_QSize_isNull_bool_QSize(Mu::Thread& NODE_THREAD, Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        return arg0.isNull();
    }

    bool qt_QSize_isValid_bool_QSize(Mu::Thread& NODE_THREAD,
                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        return arg0.isValid();
    }

    void qt_QSize_scale_void_QSize_int_int_int(Mu::Thread& NODE_THREAD,
                                               Pointer param_this,
                                               int param_width,
                                               int param_height, int param_mode)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        int arg1 = (int)(param_width);
        int arg2 = (int)(param_height);
        Qt::AspectRatioMode arg3 = (Qt::AspectRatioMode)(param_mode);
        arg0.scale(arg1, arg2, arg3);
        setqtype<QSizeType>(param_this, arg0);
    }

    void qt_QSize_scale_void_QSize_QSize_int(Mu::Thread& NODE_THREAD,
                                             Pointer param_this,
                                             Pointer param_size, int param_mode)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        const QSize arg1 = getqtype<QSizeType>(param_size);
        Qt::AspectRatioMode arg2 = (Qt::AspectRatioMode)(param_mode);
        arg0.scale(arg1, arg2);
        setqtype<QSizeType>(param_this, arg0);
    }

    void qt_QSize_setHeight_void_QSize_int(Mu::Thread& NODE_THREAD,
                                           Pointer param_this, int param_height)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        int arg1 = (int)(param_height);
        arg0.setHeight(arg1);
        setqtype<QSizeType>(param_this, arg0);
    }

    void qt_QSize_setWidth_void_QSize_int(Mu::Thread& NODE_THREAD,
                                          Pointer param_this, int param_width)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        int arg1 = (int)(param_width);
        arg0.setWidth(arg1);
        setqtype<QSizeType>(param_this, arg0);
    }

    Pointer qt_QSize_shrunkBy_QSize_QSize_QMargins(Mu::Thread& NODE_THREAD,
                                                   Pointer param_this,
                                                   Pointer param_margins)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        QMargins arg1 = getqtype<QMarginsType>(param_margins);
        return makeqtype<QSizeType>(c, arg0.shrunkBy(arg1), "qt.QSize");
    }

    void qt_QSize_transpose_void_QSize(Mu::Thread& NODE_THREAD,
                                       Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        arg0.transpose();
        setqtype<QSizeType>(param_this, arg0);
    }

    int qt_QSize_width_int_QSize(Mu::Thread& NODE_THREAD, Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        return arg0.width();
    }

    Pointer qt_QSize_operatorPlus_EQ__QSize_QSize_QSize(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this,
                                                        Pointer param_size)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        const QSize arg1 = getqtype<QSizeType>(param_size);
        return makeqtype<QSizeType>(c, arg0.operator+=(arg1), "qt.QSize");
    }

    Pointer qt_QSize_operatorMinus_EQ__QSize_QSize_QSize(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_size)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        const QSize arg1 = getqtype<QSizeType>(param_size);
        return makeqtype<QSizeType>(c, arg0.operator-=(arg1), "qt.QSize");
    }

    Pointer qt_QSize_operatorSlash_EQ__QSize_QSize_double(
        Mu::Thread& NODE_THREAD, Pointer param_this, double param_divisor)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QSize arg0 = getqtype<QSizeType>(param_this);
        qreal arg1 = (double)(param_divisor);
        return makeqtype<QSizeType>(c, arg0.operator/=(arg1), "qt.QSize");
    }

    static NODE_IMPLEMENTATION(_n_QSize0, Pointer)
    {
        NODE_RETURN(qt_QSize_QSize_QSize_QSize(NODE_THREAD,
                                               NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_QSize1, Pointer)
    {
        NODE_RETURN(qt_QSize_QSize_QSize_QSize_int_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int)));
    }

    static NODE_IMPLEMENTATION(_n_boundedTo0, Pointer)
    {
        NODE_RETURN(qt_QSize_boundedTo_QSize_QSize_QSize(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_expandedTo0, Pointer)
    {
        NODE_RETURN(qt_QSize_expandedTo_QSize_QSize_QSize(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_grownBy0, Pointer)
    {
        NODE_RETURN(qt_QSize_grownBy_QSize_QSize_QMargins(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_height0, int)
    {
        NODE_RETURN(qt_QSize_height_int_QSize(NODE_THREAD,
                                              NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_isEmpty0, bool)
    {
        NODE_RETURN(qt_QSize_isEmpty_bool_QSize(NODE_THREAD,
                                                NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_isNull0, bool)
    {
        NODE_RETURN(qt_QSize_isNull_bool_QSize(NODE_THREAD,
                                               NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_isValid0, bool)
    {
        NODE_RETURN(qt_QSize_isValid_bool_QSize(NODE_THREAD,
                                                NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_scale0, void)
    {
        qt_QSize_scale_void_QSize_int_int_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int), NODE_ARG(3, int));
    }

    static NODE_IMPLEMENTATION(_n_scale1, void)
    {
        qt_QSize_scale_void_QSize_QSize_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
            NODE_ARG(2, int));
    }

    static NODE_IMPLEMENTATION(_n_setHeight0, void)
    {
        qt_QSize_setHeight_void_QSize_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int));
    }

    static NODE_IMPLEMENTATION(_n_setWidth0, void)
    {
        qt_QSize_setWidth_void_QSize_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int));
    }

    static NODE_IMPLEMENTATION(_n_shrunkBy0, Pointer)
    {
        NODE_RETURN(qt_QSize_shrunkBy_QSize_QSize_QMargins(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_transpose0, void)
    {
        qt_QSize_transpose_void_QSize(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_width0, int)
    {
        NODE_RETURN(
            qt_QSize_width_int_QSize(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_operatorPlus_EQ_0, Pointer)
    {
        NODE_RETURN(qt_QSize_operatorPlus_EQ__QSize_QSize_QSize(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_operatorMinus_EQ_0, Pointer)
    {
        NODE_RETURN(qt_QSize_operatorMinus_EQ__QSize_QSize_QSize(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_operatorSlash_EQ_0, Pointer)
    {
        NODE_RETURN(qt_QSize_operatorSlash_EQ__QSize_QSize_double(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, double)));
    }

    void QSizeType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;
        MuLangContext* c = static_cast<MuLangContext*>(context());
        Module* global = globalModule();

        const string typeName = name();
        const string refTypeName = typeName + "&";
        const string fullTypeName = fullyQualifiedName();
        const string fullRefTypeName = fullTypeName + "&";
        const char* tn = typeName.c_str();
        const char* ftn = fullTypeName.c_str();
        const char* rtn = refTypeName.c_str();
        const char* frtn = fullRefTypeName.c_str();

        scope()->addSymbols(new ReferenceType(c, rtn, this),

                            new Function(c, tn, BaseFunctions::dereference,
                                         Cast, Return, ftn, Args, frtn, End),

                            EndArguments);

        addSymbols(
            new Function(c, "__allocate", __allocate, None, Return, ftn, End),

            EndArguments);

        addSymbols(EndArguments);

        addSymbols(
            // enums
            // member functions
            new Function(c, "QSize", _n_QSize0, None, Compiled,
                         qt_QSize_QSize_QSize_QSize, Return, "qt.QSize",
                         Parameters, new Param(c, "this", "qt.QSize"), End),
            new Function(c, "QSize", _n_QSize1, None, Compiled,
                         qt_QSize_QSize_QSize_QSize_int_int, Return, "qt.QSize",
                         Parameters, new Param(c, "this", "qt.QSize"),
                         new Param(c, "width", "int"),
                         new Param(c, "height", "int"), End),
            new Function(c, "boundedTo", _n_boundedTo0, None, Compiled,
                         qt_QSize_boundedTo_QSize_QSize_QSize, Return,
                         "qt.QSize", Parameters,
                         new Param(c, "this", "qt.QSize"),
                         new Param(c, "otherSize", "qt.QSize"), End),
            new Function(c, "expandedTo", _n_expandedTo0, None, Compiled,
                         qt_QSize_expandedTo_QSize_QSize_QSize, Return,
                         "qt.QSize", Parameters,
                         new Param(c, "this", "qt.QSize"),
                         new Param(c, "otherSize", "qt.QSize"), End),
            new Function(c, "grownBy", _n_grownBy0, None, Compiled,
                         qt_QSize_grownBy_QSize_QSize_QMargins, Return,
                         "qt.QSize", Parameters,
                         new Param(c, "this", "qt.QSize"),
                         new Param(c, "margins", "qt.QMargins"), End),
            new Function(c, "height", _n_height0, None, Compiled,
                         qt_QSize_height_int_QSize, Return, "int", Parameters,
                         new Param(c, "this", "qt.QSize"), End),
            new Function(c, "isEmpty", _n_isEmpty0, None, Compiled,
                         qt_QSize_isEmpty_bool_QSize, Return, "bool",
                         Parameters, new Param(c, "this", "qt.QSize"), End),
            new Function(c, "isNull", _n_isNull0, None, Compiled,
                         qt_QSize_isNull_bool_QSize, Return, "bool", Parameters,
                         new Param(c, "this", "qt.QSize"), End),
            new Function(c, "isValid", _n_isValid0, None, Compiled,
                         qt_QSize_isValid_bool_QSize, Return, "bool",
                         Parameters, new Param(c, "this", "qt.QSize"), End),
            // MISSING: rheight ("int &"; QSize this)
            // MISSING: rwidth ("int &"; QSize this)
            new Function(c, "scale", _n_scale0, None, Compiled,
                         qt_QSize_scale_void_QSize_int_int_int, Return, "void",
                         Parameters, new Param(c, "this", "qt.QSize"),
                         new Param(c, "width", "int"),
                         new Param(c, "height", "int"),
                         new Param(c, "mode", "int"), End),
            new Function(c, "scale", _n_scale1, None, Compiled,
                         qt_QSize_scale_void_QSize_QSize_int, Return, "void",
                         Parameters, new Param(c, "this", "qt.QSize"),
                         new Param(c, "size", "qt.QSize"),
                         new Param(c, "mode", "int"), End),
            new Function(c, "setHeight", _n_setHeight0, None, Compiled,
                         qt_QSize_setHeight_void_QSize_int, Return, "void",
                         Parameters, new Param(c, "this", "qt.QSize"),
                         new Param(c, "height", "int"), End),
            new Function(c, "setWidth", _n_setWidth0, None, Compiled,
                         qt_QSize_setWidth_void_QSize_int, Return, "void",
                         Parameters, new Param(c, "this", "qt.QSize"),
                         new Param(c, "width", "int"), End),
            new Function(c, "shrunkBy", _n_shrunkBy0, None, Compiled,
                         qt_QSize_shrunkBy_QSize_QSize_QMargins, Return,
                         "qt.QSize", Parameters,
                         new Param(c, "this", "qt.QSize"),
                         new Param(c, "margins", "qt.QMargins"), End),
            // MISSING: toCGSize ("CGSize"; QSize this)
            // MISSING: toSizeF ("QSizeF"; QSize this)
            new Function(c, "transpose", _n_transpose0, None, Compiled,
                         qt_QSize_transpose_void_QSize, Return, "void",
                         Parameters, new Param(c, "this", "qt.QSize"), End),
            new Function(c, "width", _n_width0, None, Compiled,
                         qt_QSize_width_int_QSize, Return, "int", Parameters,
                         new Param(c, "this", "qt.QSize"), End),
            // MISSING: = ("QSize & operator*"; QSize this, double factor)
            // static functions
            EndArguments);
        globalScope()->addSymbols(
            new Function(c, "+=", _n_operatorPlus_EQ_0, Op, Compiled,
                         qt_QSize_operatorPlus_EQ__QSize_QSize_QSize, Return,
                         "qt.QSize", Parameters,
                         new Param(c, "this", "qt.QSize"),
                         new Param(c, "size", "qt.QSize"), End),
            new Function(c, "-=", _n_operatorMinus_EQ_0, Op, Compiled,
                         qt_QSize_operatorMinus_EQ__QSize_QSize_QSize, Return,
                         "qt.QSize", Parameters,
                         new Param(c, "this", "qt.QSize"),
                         new Param(c, "size", "qt.QSize"), End),
            new Function(c, "/=", _n_operatorSlash_EQ_0, Op, Compiled,
                         qt_QSize_operatorSlash_EQ__QSize_QSize_double, Return,
                         "qt.QSize", Parameters,
                         new Param(c, "this", "qt.QSize"),
                         new Param(c, "divisor", "double"), End),
            EndArguments);
        scope()->addSymbols(EndArguments);
    }

} // namespace Mu
