//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__RvViewerWidget__h__
#define __RvCommon__RvViewerWidget__h__
#include <QtGui/QToolBar>
#include <iostream>
#include <RvCommon/GLView.h>

namespace Rv
{

    class RvViewerWidget : public QWidget
    {
    public:
        RvViewerWidget();

    private:
        QToolBar* m_topToolBar;
        QToolBar* m_bottomToolBar;
        GLView* m_glView;
    }

} // namespace Rv

#endif // __RvCommon__RvViewerWidget__h__
