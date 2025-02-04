//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __mu_usf__QTFrameBuffer__h__
#define __mu_usf__QTFrameBuffer__h__
#include <QtGui/QWidget>
#include <QtOpenGL/QGLWidget>
#include <TwkGLF/FrameBuffer.h>
#include <RvCommon/QTTranslator.h>

namespace Rv
{

    class QTFrameBuffer : public TwkGLF::FrameBuffer
    {
    public:
        QTFrameBuffer(QGLWidget* view);
        ~QTFrameBuffer();

        virtual size_t width() const;
        virtual size_t height() const;

        virtual int x() const;
        virtual int y() const;

        virtual void makeCurrent() const;

        const QTTranslator& translator() const { return m_translator; }

        virtual void redraw();
        virtual void redrawImmediately();

        void setAbsolutePosition(int x, int y)
        {
            m_x = x;
            m_y = y;
        }

    private:
        int m_x;
        int m_y;
        QGLWidget* m_view;
        QTTranslator m_translator;
    };

} // namespace Rv

#endif // __mu_usf__QTFrameBuffer__h__
