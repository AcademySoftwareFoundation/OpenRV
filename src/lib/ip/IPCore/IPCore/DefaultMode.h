//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__DefaultMode__h__
#define __IPCore__DefaultMode__h__
#include <TwkApp/Mode.h>

namespace IPCore
{

    //
    //  Rv's default mode
    //

    class DefaultMode : public TwkApp::MajorMode
    {
    public:
        DefaultMode(TwkApp::Document*);
        virtual ~DefaultMode();

        //
        //  TwkApp::Mode API
        //

        virtual TwkApp::Menu* menu();

    private:
        TwkApp::Menu* m_menu;
    };

} // namespace IPCore

#endif // __IPCore__DefaultMode__h__
