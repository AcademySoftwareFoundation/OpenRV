//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <assert.h>
#include <RvCommon/QTFrameBuffer.h>
#include <RvCommon/QTTranslator.h>
#include <iostream>

namespace Rv
{
    using namespace std;

    QTFrameBuffer::QTFrameBuffer(QGLWidget* view)
        : FrameBuffer(view->objectName().toUtf8().data())
        , m_view(view)
        , m_translator(this, view)
        , m_x(0)
        , m_y(0)
    {
        assert(view);
    }

    QTFrameBuffer::~QTFrameBuffer() {}

    void QTFrameBuffer::makeCurrent() const { FrameBuffer::makeCurrent(); }

    size_t QTFrameBuffer::width() const { return m_view->size().width(); }

    size_t QTFrameBuffer::height() const { return m_view->size().height(); }

    int QTFrameBuffer::x() const { return m_x; }

    int QTFrameBuffer::y() const { return m_y; }

    void QTFrameBuffer::redraw()
    {
        QSize s = m_view->size();
        return m_view->update();
    }

    void QTFrameBuffer::redrawImmediately()
    {
        if (m_view->isVisible())
        {
            m_view->updateGL();
        }
        else
        {
            redraw();
        }
    }

} //  End namespace Rv
