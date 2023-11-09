//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
#include "RVMath/LogC.h"

#include "RVMath/hermite.h"

#include <cmath>

namespace RVMath
{
    LogC::LogC(const Param& param, float asa):
        m_Param(param),
        m_ExposureGain(0.0f)
    {
        m_LinSlope = 1 / (param.cutPoint * log(10.0f));
        m_LinOffset = log10(param.cutPoint) - m_LinSlope * param.cutPoint;
        if (asa != 0.0f) setAsa(asa);
        else setAsa(param.nominalSpeed);
    }

    bool LogC::setAsa(float asa)
    {
        if (asa < 1.0f)
        {
            return false;
        }

        float gain = asa / m_Param.nominalSpeed;
        if (gain == m_ExposureGain)
        {
            return false;
        }

        m_ExposureGain = gain;
        m_GraySignal = m_Param.midGraySignal / m_ExposureGain;
        m_EncodingGain = (log(m_ExposureGain)/log(2.0f) * (0.89f - 1.0f) / 3.0f + 1.0f) * m_Param.encodingGain;
        m_EncodingOffset = m_Param.encodingOffset;
        for (int ioff = 0; ioff < 3; ++ioff)
        {
            m_BlackOffset = ((m_Param.encodingBlack - m_EncodingOffset) / m_EncodingGain - m_LinOffset) / m_LinSlope;
            m_EncodingOffset = m_Param.encodingOffset - log10(1 + m_BlackOffset) * m_EncodingGain;
        }
        m_MaxValue = unconstrainedValue(1.0);
        return(true);
    }

    float LogC::unconstrainedValue(float x) const
    {
        float xr = (x - m_Param.blackSignal ) / m_GraySignal + m_BlackOffset;
        float y = 0.0f;
        if(xr > m_Param.cutPoint)
        {
            y = log10(xr);
        }
        else
        {
            y = xr + m_LinSlope + m_LinOffset;
        }

        return y * m_EncodingGain + m_EncodingOffset;
    }

    float LogC::value(float x) const
    {
        float y = unconstrainedValue(x);
        const float yceil = 0.8f;
        if(y < 0.0f)
        {
            y = 0.0f;
        }
        else if(y > yceil && m_MaxValue > 1.0f)
        {
            float w[4];
            hermite(y, yceil, m_MaxValue, w);
            const float d = 0.2f / (m_MaxValue - yceil);
            const float powD = pow(d, 2.0f);
            y = w[0] * yceil + w[1] + w[2] + w[3] * powD;
        }
        return y;
    }
}
