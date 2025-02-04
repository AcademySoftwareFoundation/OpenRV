//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <OCIONodes/OCIO1DLUT.h>
#include <TwkExc/Exception.h>

namespace IPCore
{

    OCIO1DLUT::OCIO1DLUT(OCIO::GpuShaderDescRcPtr& shaderDesc, unsigned int idx,
                         const std::string& shaderCacheID)
    {
        // Get the information of the 1D LUT.
        const char* textureName = nullptr;
        const char* samplerName = nullptr;
        unsigned width = 0;
        unsigned height = 0;
        OCIO::GpuShaderDesc::TextureType textureType =
            OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL;
        OCIO::Interpolation interpolation = OCIO::INTERP_BEST;
#if defined(RV_VFX_CY2023)
        // Older (pre OCIOv2 2.3.x) getTexture() function signature
        shaderDesc->getTexture(idx, textureName, samplerName, width, height,
                               textureType, interpolation);
#else
        OCIO::GpuShaderDesc::TextureDimensions _textureDim =
            OCIO::GpuShaderDesc::TextureDimensions::TEXTURE_1D;
        shaderDesc->getTexture(idx, textureName, samplerName, width, height,
                               textureType, _textureDim, interpolation);
#endif
        if (!samplerName || !*samplerName || width == 0)
        {
            TWK_THROW_EXC_STREAM("The OCIO 1D LUT texture data is corrupted");
        }
        const int channelSize =
            (textureType == OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL) ? 3 : 1;
        m_samplerName = samplerName;
        m_height = height;

        m_lutfb = new TwkFB::FrameBuffer(
            TwkFB::FrameBuffer::NormalizedCoordinates, width, height,
            1 /*depth*/, channelSize, TwkFB::FrameBuffer::FLOAT,
            nullptr /*data*/, nullptr /*channelNames are optional*/,
            TwkFB::FrameBuffer::BOTTOMLEFT, true /*deleteOnDestruction*/);

        m_lutfb->staticRef();
        m_lutfb->setIdentifier(shaderCacheID + "|" + m_samplerName);

        const float* values = nullptr;
        shaderDesc->getTextureValues(idx, values);
        if (!values)
        {
            TWK_THROW_EXC_STREAM("The OCIO 1D LUT texture values are missing");
        }

        memcpy(m_lutfb->pixels<float>(), values,
               channelSize * sizeof(float) * width * height);
    }

} // namespace IPCore
