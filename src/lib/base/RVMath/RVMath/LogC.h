//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __base__RVMath__LogC
#define __base__RVMath__LogC

class Param;

namespace RVMath
{

    struct Param
    {
        float cutPoint;
        float nominalSpeed;

        float midGraySignal;
        float blackSignal;

        float encodingBlack;
        float encodingOffset;
        float encodingGain;
    };

    class LogC
    {
    public:
        explicit LogC(const Param& param, float asa);

        float value(float x) const;

        float getExposureGain() { return m_ExposureGain; }

        float getEncodingGain() { return m_EncodingGain; }

        float getEncodingOffset() { return m_EncodingOffset; }

        float getLinearSlope() { return m_LinSlope; }

        float getLinearOffset() { return m_LinOffset; }

        float getBlackOffset() { return m_BlackOffset; }

        float getGraySignal() { return m_GraySignal; }

    private:
        bool setAsa(float asa);
        float unconstrainedValue(float x) const;

        float m_ExposureGain;
        float m_EncodingGain;
        float m_EncodingOffset;

        float m_LinSlope;
        float m_LinOffset;

        float m_BlackOffset;
        float m_GraySignal;

        float m_MaxValue;

        Param m_Param;
    };
} // namespace RVMath

#endif
