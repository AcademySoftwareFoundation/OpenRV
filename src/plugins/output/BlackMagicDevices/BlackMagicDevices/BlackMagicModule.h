//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <TwkApp/VideoModule.h>
#include <TwkGLF/GLVideoDevice.h>
#include <DeckLinkAPI.h>
#include <DeckLinkAPIVersion.h>
#include <iostream>

namespace BlackMagicDevices
{

    class BlackMagicModule : public TwkApp::VideoModule
    {
    public:
        BlackMagicModule(NativeDisplayPtr);
        virtual ~BlackMagicModule();

        virtual std::string name() const;
        virtual std::string SDKIdentifier() const;
        virtual std::string SDKInfo() const;
        virtual void open();
        virtual void close();
        virtual bool isOpen() const;
    };

} // namespace BlackMagicDevices
