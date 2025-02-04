//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <OCIONodes/OCIO3DLUT.h>
#include <TwkExc/Exception.h>

namespace IPCore
{

    OCIO3DLUT::OCIO3DLUT(OCIO::GpuShaderDescRcPtr& shaderDesc, unsigned int idx,
                         const std::string& shaderCacheID)
    {
        // Get the information of the 3D LUT.
        const char* textureName = nullptr;
        const char* samplerName = nullptr;
        unsigned edgelen = 0;
        OCIO::Interpolation interpolation = OCIO::INTERP_BEST;
        shaderDesc->get3DTexture(idx, textureName, samplerName, edgelen,
                                 interpolation);
        if (!samplerName || !*samplerName || edgelen == 0)
        {
            TWK_THROW_EXC_STREAM("The OCIO 3D LUT texture data is corrupted");
        }
        m_samplerName = samplerName;

        m_lutfb = new TwkFB::FrameBuffer(
            TwkFB::FrameBuffer::NormalizedCoordinates, edgelen, edgelen,
            edgelen, 3 /*channelSize*/, TwkFB::FrameBuffer::FLOAT,
            nullptr /*data*/, nullptr /*channelNames are optional*/,
            TwkFB::FrameBuffer::BOTTOMLEFT, true /*deleteOnDestruction*/);

        m_lutfb->staticRef();
        m_lutfb->setIdentifier(shaderCacheID + "|" + m_samplerName);

        const float* values = nullptr;
        shaderDesc->get3DTextureValues(idx, values);
        if (!values)
        {
            TWK_THROW_EXC_STREAM("The OCIO 3D LUT texture values are missing");
        }

        memcpy(m_lutfb->pixels<float>(), values,
               3 * sizeof(float) * edgelen * edgelen * edgelen);
    }

} // namespace IPCore
