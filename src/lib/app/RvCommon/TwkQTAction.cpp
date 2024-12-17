//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <RvCommon/TwkQTAction.h>

namespace Rv
{

    TwkQTAction::TwkQTAction(const TwkApp::Menu::Item* i, RvDocument* doc,
                             QObject* parent)
        : QAction(parent)
        , m_item(i)
        , m_doc(doc)
    {
    }

} // namespace Rv
