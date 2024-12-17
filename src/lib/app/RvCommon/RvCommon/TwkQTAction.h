//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __rv_qt__TwkQTAction__h__
#define __rv_qt__TwkQTAction__h__
#include <QtCore/QtCore>
#include <QtWidgets/QAction>
#include <TwkApp/Menu.h>
#include <TwkApp/Action.h>

namespace TwkApp
{
    class Action;
}

namespace Rv
{
    class RvDocument;

    class TwkQTAction : public QAction
    {
        Q_OBJECT

    public:
        TwkQTAction(const TwkApp::Menu::Item*, RvDocument* document,
                    QObject* parent);

        const TwkApp::Menu::Item* item() const { return m_item; }

        RvDocument* doc() const { return m_doc; }

    private:
        RvDocument* m_doc;
        const TwkApp::Menu::Item* m_item;
    };

} // namespace Rv

#endif // __rv-qt__TwkQTAction__h__
