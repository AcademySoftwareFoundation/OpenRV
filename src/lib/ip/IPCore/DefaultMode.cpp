//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/DefaultMode.h>
#include <TwkApp/Menu.h>
#include <TwkApp/SelectionType.h>
#include <TwkApp/EventTable.h>

namespace IPCore
{
    using namespace TwkApp;

    DefaultMode::DefaultMode(TwkApp::Document* doc)
        : TwkApp::MajorMode("default", doc)
        , m_menu(0)
    {
        EventTable* global = new EventTable("global");
        addEventTable(global);

        //
        //	Create menu
        //

        m_menu = new Menu("Main");
    }

    DefaultMode::~DefaultMode() {}

    TwkApp::Menu* DefaultMode::menu() { return m_menu; }

} // namespace IPCore
