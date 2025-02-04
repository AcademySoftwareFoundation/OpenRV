//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <ALSAAudioModule/ALSAAudioRenderer.h>

extern "C"
{

    IPCore::AudioRenderer*
    CreateAudioModule(const IPCore::AudioRenderer::RendererParameters& p)
    {
        return new IPCore::ALSAAudioRenderer(p);
    }
}
