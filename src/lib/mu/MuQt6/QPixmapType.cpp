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
#include <MuQt6/QPixmapType.h>
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
#include <MuQt6/QSizeType.h>
#include <MuQt6/QColorType.h>
#include <MuQt6/QBitmapType.h>
#include <MuQt6/QRectType.h>
#include <MuQt6/QTransformType.h>
#include <MuQt6/QImageType.h>

namespace Mu
{
    using namespace std;

    QPixmapType::Instance::Instance(const Class* c)
        : ClassInstance(c)
    {
    }

    QPixmapType::QPixmapType(Context* c, const char* name, Class* super)
        : Class(c, name, super)
    {
    }

    QPixmapType::~QPixmapType() {}

    static NODE_IMPLEMENTATION(__allocate, Pointer)
    {
        QPixmapType::Instance* i =
            new QPixmapType::Instance((Class*)NODE_THIS.type());
        QPixmapType::registerFinalizer(i);
        NODE_RETURN(i);
    }

    void QPixmapType::registerFinalizer(void* o)
    {
        GC_register_finalizer(o, QPixmapType::finalizer, 0, 0, 0);
    }

    void QPixmapType::finalizer(void* obj, void* data)
    {
        QPixmapType::Instance* i =
            reinterpret_cast<QPixmapType::Instance*>(obj);
        delete i;
    }

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    Pointer qt_QPixmap_QPixmap_QPixmap_QPixmap(Mu::Thread& NODE_THREAD,
                                               Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        setpaintdevice(param_this, QPixmap());
        return param_this;
    }

    Pointer qt_QPixmap_QPixmap_QPixmap_QPixmap_int_int(Mu::Thread& NODE_THREAD,
                                                       Pointer param_this,
                                                       int param_width,
                                                       int param_height)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        int arg1 = (int)(param_width);
        int arg2 = (int)(param_height);
        setpaintdevice(param_this, QPixmap(arg1, arg2));
        return param_this;
    }

    Pointer qt_QPixmap_QPixmap_QPixmap_QPixmap_QSize(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this,
                                                     Pointer param_size)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QSize arg1 = getqtype<QSizeType>(param_size);
        setpaintdevice(param_this, QPixmap(arg1));
        return param_this;
    }

    int64 qt_QPixmap_cacheKey_int64_QPixmap(Mu::Thread& NODE_THREAD,
                                            Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.cacheKey();
    }

    bool qt_QPixmap_convertFromImage_bool_QPixmap_QImage_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_image,
        int param_flags)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        const QImage arg1 = getqtype<QImageType>(param_image);
        Qt::ImageConversionFlags arg2 = (Qt::ImageConversionFlags)(param_flags);
        return arg0.convertFromImage(arg1, arg2);
    }

    Pointer qt_QPixmap_copy_QPixmap_QPixmap_QRect(Mu::Thread& NODE_THREAD,
                                                  Pointer param_this,
                                                  Pointer param_rectangle)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        const QRect arg1 = getqtype<QRectType>(param_rectangle);
        return makeqtype<QPixmapType>(c, arg0.copy(arg1), "qt.QPixmap");
    }

    Pointer qt_QPixmap_copy_QPixmap_QPixmap_int_int_int_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_x, int param_y,
        int param_width, int param_height)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        int arg1 = (int)(param_x);
        int arg2 = (int)(param_y);
        int arg3 = (int)(param_width);
        int arg4 = (int)(param_height);
        return makeqtype<QPixmapType>(c, arg0.copy(arg1, arg2, arg3, arg4),
                                      "qt.QPixmap");
    }

    Pointer qt_QPixmap_createHeuristicMask_QBitmap_QPixmap_bool(
        Mu::Thread& NODE_THREAD, Pointer param_this, bool param_clipTight)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        bool arg1 = (bool)(param_clipTight);
        return makeqtype<QBitmapType>(c, arg0.createHeuristicMask(arg1),
                                      "qt.QBitmap");
    }

    Pointer qt_QPixmap_createMaskFromColor_QBitmap_QPixmap_QColor_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_maskColor,
        int param_mode)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        const QColor arg1 = getqtype<QColorType>(param_maskColor);
        Qt::MaskMode arg2 = (Qt::MaskMode)(param_mode);
        return makeqtype<QBitmapType>(c, arg0.createMaskFromColor(arg1, arg2),
                                      "qt.QBitmap");
    }

    int qt_QPixmap_depth_int_QPixmap(Mu::Thread& NODE_THREAD,
                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.depth();
    }

    void qt_QPixmap_detach_void_QPixmap(Mu::Thread& NODE_THREAD,
                                        Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        arg0.detach();
    }

    double qt_QPixmap_devicePixelRatio_double_QPixmap(Mu::Thread& NODE_THREAD,
                                                      Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.devicePixelRatio();
    }

    void qt_QPixmap_fill_void_QPixmap_QColor(Mu::Thread& NODE_THREAD,
                                             Pointer param_this,
                                             Pointer param_color)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        const QColor arg1 = getqtype<QColorType>(param_color);
        arg0.fill(arg1);
    }

    bool qt_QPixmap_hasAlpha_bool_QPixmap(Mu::Thread& NODE_THREAD,
                                          Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.hasAlpha();
    }

    bool qt_QPixmap_hasAlphaChannel_bool_QPixmap(Mu::Thread& NODE_THREAD,
                                                 Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.hasAlphaChannel();
    }

    int qt_QPixmap_height_int_QPixmap(Mu::Thread& NODE_THREAD,
                                      Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.height();
    }

    bool qt_QPixmap_isNull_bool_QPixmap(Mu::Thread& NODE_THREAD,
                                        Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.isNull();
    }

    bool qt_QPixmap_isQBitmap_bool_QPixmap(Mu::Thread& NODE_THREAD,
                                           Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.isQBitmap();
    }

    Pointer qt_QPixmap_mask_QBitmap_QPixmap(Mu::Thread& NODE_THREAD,
                                            Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return makeqtype<QBitmapType>(c, arg0.mask(), "qt.QBitmap");
    }

    Pointer qt_QPixmap_rect_QRect_QPixmap(Mu::Thread& NODE_THREAD,
                                          Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return makeqtype<QRectType>(c, arg0.rect(), "qt.QRect");
    }

    Pointer qt_QPixmap_scaled_QPixmap_QPixmap_QSize_int_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_size,
        int param_aspectRatioMode, int param_transformMode)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        const QSize arg1 = getqtype<QSizeType>(param_size);
        Qt::AspectRatioMode arg2 = (Qt::AspectRatioMode)(param_aspectRatioMode);
        Qt::TransformationMode arg3 =
            (Qt::TransformationMode)(param_transformMode);
        return makeqtype<QPixmapType>(c, arg0.scaled(arg1, arg2, arg3),
                                      "qt.QPixmap");
    }

    Pointer qt_QPixmap_scaled_QPixmap_QPixmap_int_int_int_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_width,
        int param_height, int param_aspectRatioMode, int param_transformMode)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        int arg1 = (int)(param_width);
        int arg2 = (int)(param_height);
        Qt::AspectRatioMode arg3 = (Qt::AspectRatioMode)(param_aspectRatioMode);
        Qt::TransformationMode arg4 =
            (Qt::TransformationMode)(param_transformMode);
        return makeqtype<QPixmapType>(c, arg0.scaled(arg1, arg2, arg3, arg4),
                                      "qt.QPixmap");
    }

    Pointer qt_QPixmap_scaledToHeight_QPixmap_QPixmap_int_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_height,
        int param_mode)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        int arg1 = (int)(param_height);
        Qt::TransformationMode arg2 = (Qt::TransformationMode)(param_mode);
        return makeqtype<QPixmapType>(c, arg0.scaledToHeight(arg1, arg2),
                                      "qt.QPixmap");
    }

    Pointer qt_QPixmap_scaledToWidth_QPixmap_QPixmap_int_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_width,
        int param_mode)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        int arg1 = (int)(param_width);
        Qt::TransformationMode arg2 = (Qt::TransformationMode)(param_mode);
        return makeqtype<QPixmapType>(c, arg0.scaledToWidth(arg1, arg2),
                                      "qt.QPixmap");
    }

    void qt_QPixmap_setDevicePixelRatio_void_QPixmap_double(
        Mu::Thread& NODE_THREAD, Pointer param_this, double param_scaleFactor)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        qreal arg1 = (double)(param_scaleFactor);
        arg0.setDevicePixelRatio(arg1);
    }

    void qt_QPixmap_setMask_void_QPixmap_QBitmap(Mu::Thread& NODE_THREAD,
                                                 Pointer param_this,
                                                 Pointer param_mask)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        const QBitmap arg1 = getqtype<QBitmapType>(param_mask);
        arg0.setMask(arg1);
    }

    Pointer qt_QPixmap_size_QSize_QPixmap(Mu::Thread& NODE_THREAD,
                                          Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return makeqtype<QSizeType>(c, arg0.size(), "qt.QSize");
    }

    void qt_QPixmap_swap_void_QPixmap_QPixmap(Mu::Thread& NODE_THREAD,
                                              Pointer param_this,
                                              Pointer param_other)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        QPixmap arg1 = getqtype<QPixmapType>(param_other);
        arg0.swap(arg1);
    }

    Pointer qt_QPixmap_toImage_QImage_QPixmap(Mu::Thread& NODE_THREAD,
                                              Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return makeqtype<QImageType>(c, arg0.toImage(), "qt.QImage");
    }

    Pointer qt_QPixmap_transformed_QPixmap_QPixmap_QTransform_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_transform,
        int param_mode)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        const QTransform arg1 = getqtype<QTransformType>(param_transform);
        Qt::TransformationMode arg2 = (Qt::TransformationMode)(param_mode);
        return makeqtype<QPixmapType>(c, arg0.transformed(arg1, arg2),
                                      "qt.QPixmap");
    }

    int qt_QPixmap_width_int_QPixmap(Mu::Thread& NODE_THREAD,
                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.width();
    }

    bool qt_QPixmap_operatorBang__bool_QPixmap(Mu::Thread& NODE_THREAD,
                                               Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QPixmap& arg0 = getqtype<QPixmapType>(param_this);
        return arg0.operator!();
    }

    int qt_QPixmap_defaultDepth_int(Mu::Thread& NODE_THREAD)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        return QPixmap::defaultDepth();
    }

    Pointer qt_QPixmap_fromImage_QPixmap_QImage_int(Mu::Thread& NODE_THREAD,
                                                    Pointer param_image,
                                                    int param_flags)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QImage arg0 = getqtype<QImageType>(param_image);
        Qt::ImageConversionFlags arg1 = (Qt::ImageConversionFlags)(param_flags);
        return makeqtype<QPixmapType>(c, QPixmap::fromImage(arg0, arg1),
                                      "qt.QPixmap");
    }

    Pointer qt_QPixmap_trueMatrix_QTransform_QTransform_int_int(
        Mu::Thread& NODE_THREAD, Pointer param_matrix, int param_width,
        int param_height)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QTransform arg0 = getqtype<QTransformType>(param_matrix);
        int arg1 = (int)(param_width);
        int arg2 = (int)(param_height);
        return makeqtype<QTransformType>(
            c, QPixmap::trueMatrix(arg0, arg1, arg2), "qt.QTransform");
    }

    static NODE_IMPLEMENTATION(_n_QPixmap0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_QPixmap_QPixmap_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_QPixmap1, Pointer)
    {
        NODE_RETURN(qt_QPixmap_QPixmap_QPixmap_QPixmap_int_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int)));
    }

    static NODE_IMPLEMENTATION(_n_QPixmap2, Pointer)
    {
        NODE_RETURN(qt_QPixmap_QPixmap_QPixmap_QPixmap_QSize(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_cacheKey0, int64)
    {
        NODE_RETURN(qt_QPixmap_cacheKey_int64_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_convertFromImage0, bool)
    {
        NODE_RETURN(qt_QPixmap_convertFromImage_bool_QPixmap_QImage_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
            NODE_ARG(2, int)));
    }

    static NODE_IMPLEMENTATION(_n_copy0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_copy_QPixmap_QPixmap_QRect(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_copy1, Pointer)
    {
        NODE_RETURN(qt_QPixmap_copy_QPixmap_QPixmap_int_int_int_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int), NODE_ARG(3, int), NODE_ARG(4, int)));
    }

    static NODE_IMPLEMENTATION(_n_createHeuristicMask0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_createHeuristicMask_QBitmap_QPixmap_bool(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, bool)));
    }

    static NODE_IMPLEMENTATION(_n_createMaskFromColor0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_createMaskFromColor_QBitmap_QPixmap_QColor_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
            NODE_ARG(2, int)));
    }

    static NODE_IMPLEMENTATION(_n_depth0, int)
    {
        NODE_RETURN(qt_QPixmap_depth_int_QPixmap(NODE_THREAD,
                                                 NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_detach0, void)
    {
        qt_QPixmap_detach_void_QPixmap(NODE_THREAD,
                                       NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_devicePixelRatio0, double)
    {
        NODE_RETURN(qt_QPixmap_devicePixelRatio_double_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_fill0, void)
    {
        qt_QPixmap_fill_void_QPixmap_QColor(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_hasAlpha0, bool)
    {
        NODE_RETURN(qt_QPixmap_hasAlpha_bool_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_hasAlphaChannel0, bool)
    {
        NODE_RETURN(qt_QPixmap_hasAlphaChannel_bool_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_height0, int)
    {
        NODE_RETURN(qt_QPixmap_height_int_QPixmap(NODE_THREAD,
                                                  NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_isNull0, bool)
    {
        NODE_RETURN(qt_QPixmap_isNull_bool_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_isQBitmap0, bool)
    {
        NODE_RETURN(qt_QPixmap_isQBitmap_bool_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_mask0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_mask_QBitmap_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_rect0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_rect_QRect_QPixmap(NODE_THREAD,
                                                  NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_scaled0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_scaled_QPixmap_QPixmap_QSize_int_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
            NODE_ARG(2, int), NODE_ARG(3, int)));
    }

    static NODE_IMPLEMENTATION(_n_scaled1, Pointer)
    {
        NODE_RETURN(qt_QPixmap_scaled_QPixmap_QPixmap_int_int_int_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int), NODE_ARG(3, int), NODE_ARG(4, int)));
    }

    static NODE_IMPLEMENTATION(_n_scaledToHeight0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_scaledToHeight_QPixmap_QPixmap_int_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int)));
    }

    static NODE_IMPLEMENTATION(_n_scaledToWidth0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_scaledToWidth_QPixmap_QPixmap_int_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int)));
    }

    static NODE_IMPLEMENTATION(_n_setDevicePixelRatio0, void)
    {
        qt_QPixmap_setDevicePixelRatio_void_QPixmap_double(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, double));
    }

    static NODE_IMPLEMENTATION(_n_setMask0, void)
    {
        qt_QPixmap_setMask_void_QPixmap_QBitmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_size0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_size_QSize_QPixmap(NODE_THREAD,
                                                  NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_swap0, void)
    {
        qt_QPixmap_swap_void_QPixmap_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_toImage0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_toImage_QImage_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_transformed0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_transformed_QPixmap_QPixmap_QTransform_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
            NODE_ARG(2, int)));
    }

    static NODE_IMPLEMENTATION(_n_width0, int)
    {
        NODE_RETURN(qt_QPixmap_width_int_QPixmap(NODE_THREAD,
                                                 NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_operatorBang_0, bool)
    {
        NODE_RETURN(qt_QPixmap_operatorBang__bool_QPixmap(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_defaultDepth0, int)
    {
        NODE_RETURN(qt_QPixmap_defaultDepth_int(NODE_THREAD));
    }

    static NODE_IMPLEMENTATION(_n_fromImage0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_fromImage_QPixmap_QImage_int(
            NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, int)));
    }

    static NODE_IMPLEMENTATION(_n_trueMatrix0, Pointer)
    {
        NODE_RETURN(qt_QPixmap_trueMatrix_QTransform_QTransform_int_int(
            NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int)));
    }

    void QPixmapType::load()
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
            new Function(c, "QPixmap", _n_QPixmap0, None, Compiled,
                         qt_QPixmap_QPixmap_QPixmap_QPixmap, Return,
                         "qt.QPixmap", Parameters,
                         new Param(c, "this", "qt.QPixmap"), End),
            new Function(c, "QPixmap", _n_QPixmap1, None, Compiled,
                         qt_QPixmap_QPixmap_QPixmap_QPixmap_int_int, Return,
                         "qt.QPixmap", Parameters,
                         new Param(c, "this", "qt.QPixmap"),
                         new Param(c, "width", "int"),
                         new Param(c, "height", "int"), End),
            new Function(c, "QPixmap", _n_QPixmap2, None, Compiled,
                         qt_QPixmap_QPixmap_QPixmap_QPixmap_QSize, Return,
                         "qt.QPixmap", Parameters,
                         new Param(c, "this", "qt.QPixmap"),
                         new Param(c, "size", "qt.QSize"), End),
            // MISSING: QPixmap (QPixmap; QPixmap this, string fileName, "const
            // char *" format, flags Qt::ImageConversionFlags flags) MISSING:
            // QPixmap (QPixmap; QPixmap this, "const char * const[]" xpm)
            // MISSING: QPixmap (QPixmap; QPixmap this, QPixmap pixmap)
            // MISSING: QPixmap (QPixmap; QPixmap this, "QPixmap & &" other)
            new Function(c, "cacheKey", _n_cacheKey0, None, Compiled,
                         qt_QPixmap_cacheKey_int64_QPixmap, Return, "int64",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            new Function(
                c, "convertFromImage", _n_convertFromImage0, None, Compiled,
                qt_QPixmap_convertFromImage_bool_QPixmap_QImage_int, Return,
                "bool", Parameters, new Param(c, "this", "qt.QPixmap"),
                new Param(c, "image", "qt.QImage"),
                new Param(c, "flags", "int", Value((int)Qt::AutoColor)), End),
            new Function(c, "copy", _n_copy0, None, Compiled,
                         qt_QPixmap_copy_QPixmap_QPixmap_QRect, Return,
                         "qt.QPixmap", Parameters,
                         new Param(c, "this", "qt.QPixmap"),
                         new Param(c, "rectangle", "qt.QRect"), End),
            new Function(c, "copy", _n_copy1, None, Compiled,
                         qt_QPixmap_copy_QPixmap_QPixmap_int_int_int_int,
                         Return, "qt.QPixmap", Parameters,
                         new Param(c, "this", "qt.QPixmap"),
                         new Param(c, "x", "int"), new Param(c, "y", "int"),
                         new Param(c, "width", "int"),
                         new Param(c, "height", "int"), End),
            new Function(c, "createHeuristicMask", _n_createHeuristicMask0,
                         None, Compiled,
                         qt_QPixmap_createHeuristicMask_QBitmap_QPixmap_bool,
                         Return, "qt.QBitmap", Parameters,
                         new Param(c, "this", "qt.QPixmap"),
                         new Param(c, "clipTight", "bool"), End),
            new Function(
                c, "createMaskFromColor", _n_createMaskFromColor0, None,
                Compiled,
                qt_QPixmap_createMaskFromColor_QBitmap_QPixmap_QColor_int,
                Return, "qt.QBitmap", Parameters,
                new Param(c, "this", "qt.QPixmap"),
                new Param(c, "maskColor", "qt.QColor"),
                new Param(c, "mode", "int", Value((int)Qt::MaskInColor)), End),
            new Function(c, "depth", _n_depth0, None, Compiled,
                         qt_QPixmap_depth_int_QPixmap, Return, "int",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            new Function(c, "detach", _n_detach0, None, Compiled,
                         qt_QPixmap_detach_void_QPixmap, Return, "void",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            // MISSING: deviceIndependentSize ("QSizeF"; QPixmap this)
            new Function(c, "devicePixelRatio", _n_devicePixelRatio0, None,
                         Compiled, qt_QPixmap_devicePixelRatio_double_QPixmap,
                         Return, "double", Parameters,
                         new Param(c, "this", "qt.QPixmap"), End),
            new Function(c, "fill", _n_fill0, None, Compiled,
                         qt_QPixmap_fill_void_QPixmap_QColor, Return, "void",
                         Parameters, new Param(c, "this", "qt.QPixmap"),
                         new Param(c, "color", "qt.QColor"), End),
            new Function(c, "hasAlpha", _n_hasAlpha0, None, Compiled,
                         qt_QPixmap_hasAlpha_bool_QPixmap, Return, "bool",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            new Function(c, "hasAlphaChannel", _n_hasAlphaChannel0, None,
                         Compiled, qt_QPixmap_hasAlphaChannel_bool_QPixmap,
                         Return, "bool", Parameters,
                         new Param(c, "this", "qt.QPixmap"), End),
            new Function(c, "height", _n_height0, None, Compiled,
                         qt_QPixmap_height_int_QPixmap, Return, "int",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            new Function(c, "isNull", _n_isNull0, None, Compiled,
                         qt_QPixmap_isNull_bool_QPixmap, Return, "bool",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            new Function(c, "isQBitmap", _n_isQBitmap0, None, Compiled,
                         qt_QPixmap_isQBitmap_bool_QPixmap, Return, "bool",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            // MISSING: load (bool; QPixmap this, string fileName, "const char
            // *" format, flags Qt::ImageConversionFlags flags) MISSING:
            // loadFromData (bool; QPixmap this, "const uchar *" data, int len,
            // "const char *" format, flags Qt::ImageConversionFlags flags)
            // MISSING: loadFromData (bool; QPixmap this, QByteArray data,
            // "const char *" format, flags Qt::ImageConversionFlags flags)
            new Function(c, "mask", _n_mask0, None, Compiled,
                         qt_QPixmap_mask_QBitmap_QPixmap, Return, "qt.QBitmap",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            new Function(c, "rect", _n_rect0, None, Compiled,
                         qt_QPixmap_rect_QRect_QPixmap, Return, "qt.QRect",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            // MISSING: save (bool; QPixmap this, string fileName, "const char
            // *" format, int quality) MISSING: save (bool; QPixmap this,
            // QIODevice device, "const char *" format, int quality)
            new Function(c, "scaled", _n_scaled0, None, Compiled,
                         qt_QPixmap_scaled_QPixmap_QPixmap_QSize_int_int,
                         Return, "qt.QPixmap", Parameters,
                         new Param(c, "this", "qt.QPixmap"),
                         new Param(c, "size", "qt.QSize"),
                         new Param(c, "aspectRatioMode", "int",
                                   Value((int)Qt::IgnoreAspectRatio)),
                         new Param(c, "transformMode", "int",
                                   Value((int)Qt::FastTransformation)),
                         End),
            new Function(
                c, "scaled", _n_scaled1, None, Compiled,
                qt_QPixmap_scaled_QPixmap_QPixmap_int_int_int_int, Return,
                "qt.QPixmap", Parameters, new Param(c, "this", "qt.QPixmap"),
                new Param(c, "width", "int"), new Param(c, "height", "int"),
                new Param(c, "aspectRatioMode", "int",
                          Value((int)Qt::IgnoreAspectRatio)),
                new Param(c, "transformMode", "int",
                          Value((int)Qt::FastTransformation)),
                End),
            new Function(
                c, "scaledToHeight", _n_scaledToHeight0, None, Compiled,
                qt_QPixmap_scaledToHeight_QPixmap_QPixmap_int_int, Return,
                "qt.QPixmap", Parameters, new Param(c, "this", "qt.QPixmap"),
                new Param(c, "height", "int"),
                new Param(c, "mode", "int", Value((int)Qt::FastTransformation)),
                End),
            new Function(
                c, "scaledToWidth", _n_scaledToWidth0, None, Compiled,
                qt_QPixmap_scaledToWidth_QPixmap_QPixmap_int_int, Return,
                "qt.QPixmap", Parameters, new Param(c, "this", "qt.QPixmap"),
                new Param(c, "width", "int"),
                new Param(c, "mode", "int", Value((int)Qt::FastTransformation)),
                End),
            new Function(
                c, "setDevicePixelRatio", _n_setDevicePixelRatio0, None,
                Compiled, qt_QPixmap_setDevicePixelRatio_void_QPixmap_double,
                Return, "void", Parameters, new Param(c, "this", "qt.QPixmap"),
                new Param(c, "scaleFactor", "double"), End),
            new Function(c, "setMask", _n_setMask0, None, Compiled,
                         qt_QPixmap_setMask_void_QPixmap_QBitmap, Return,
                         "void", Parameters, new Param(c, "this", "qt.QPixmap"),
                         new Param(c, "mask", "qt.QBitmap"), End),
            new Function(c, "size", _n_size0, None, Compiled,
                         qt_QPixmap_size_QSize_QPixmap, Return, "qt.QSize",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            new Function(c, "swap", _n_swap0, None, Compiled,
                         qt_QPixmap_swap_void_QPixmap_QPixmap, Return, "void",
                         Parameters, new Param(c, "this", "qt.QPixmap"),
                         new Param(c, "other", "qt.QPixmap"), End),
            new Function(c, "toImage", _n_toImage0, None, Compiled,
                         qt_QPixmap_toImage_QImage_QPixmap, Return, "qt.QImage",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            new Function(
                c, "transformed", _n_transformed0, None, Compiled,
                qt_QPixmap_transformed_QPixmap_QPixmap_QTransform_int, Return,
                "qt.QPixmap", Parameters, new Param(c, "this", "qt.QPixmap"),
                new Param(c, "transform", "qt.QTransform"),
                new Param(c, "mode", "int", Value((int)Qt::FastTransformation)),
                End),
            new Function(c, "width", _n_width0, None, Compiled,
                         qt_QPixmap_width_int_QPixmap, Return, "int",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            // MISSING: QVariant ("QVariant operator"; QPixmap this)
            // static functions
            new Function(c, "defaultDepth", _n_defaultDepth0, None, Compiled,
                         qt_QPixmap_defaultDepth_int, Return, "int", End),
            new Function(
                c, "fromImage", _n_fromImage0, None, Compiled,
                qt_QPixmap_fromImage_QPixmap_QImage_int, Return, "qt.QPixmap",
                Parameters, new Param(c, "image", "qt.QImage"),
                new Param(c, "flags", "int", Value((int)Qt::AutoColor)), End),
            // MISSING: fromImageReader (QPixmap; "QImageReader *" imageReader,
            // flags Qt::ImageConversionFlags flags)
            new Function(c, "trueMatrix", _n_trueMatrix0, None, Compiled,
                         qt_QPixmap_trueMatrix_QTransform_QTransform_int_int,
                         Return, "qt.QTransform", Parameters,
                         new Param(c, "matrix", "qt.QTransform"),
                         new Param(c, "width", "int"),
                         new Param(c, "height", "int"), End),
            EndArguments);
        globalScope()->addSymbols(
            new Function(c, "!", _n_operatorBang_0, Op, Compiled,
                         qt_QPixmap_operatorBang__bool_QPixmap, Return, "bool",
                         Parameters, new Param(c, "this", "qt.QPixmap"), End),
            // MISSING: = (QPixmap; QPixmap this, QPixmap pixmap)
            // MISSING: = (QPixmap; QPixmap this, "QPixmap & &" other)
            EndArguments);
        scope()->addSymbols(EndArguments);
    }

} // namespace Mu
