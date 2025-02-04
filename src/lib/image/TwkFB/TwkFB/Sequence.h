//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __TWKFBSEQUENCE_H__
#define __TWKFBSEQUENCE_H__

#include <TwkExc/TwkExcException.h>
#include <TwkUtil/FileSequence.h>
#include <TwkFB/IO.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/dll_defs.h>

namespace TwkFB
{

    class TWKFB_EXPORT FBSequence
    {
    public:
        TWK_EXC_DECLARE(Exception, TwkExc::Exception,
                        "FBSequence::Exception: ");

        FBSequence(std::string sequencePattern);
        virtual ~FBSequence();

        virtual FrameBuffer* frame(int frame);

        int width() const { return m_imgInfo.width; }

        int height() const { return m_imgInfo.height; }

        int numChannels() const { return m_imgInfo.numChannels; }

        FrameBuffer::DataType dataType() const { return m_imgInfo.dataType; }

        int startFrame() const { return m_startFrame; }

        int endFrame() const { return m_endFrame; }

        int frameIncrement() const { return m_frameIncrement; }

        int numFrames() const { return m_endFrame - m_startFrame + 1; }

        float frameRate() const { return m_frameRate; }

        std::string sequencePattern() const { return m_sequencePattern; }

    private:
        std::string m_sequencePattern;
        TwkUtil::FileSequence* m_fs;
        FrameBufferIO* m_plugin;

        FBInfo m_imgInfo;

        int m_startFrame;
        int m_endFrame;
        int m_frameIncrement;
        float m_frameRate;

    private:
        //
        // Don't allow copy or assignment at this time
        //
        FBSequence() {}

        FBSequence(const FBSequence& copy) {}

        void operator=(const FBSequence& copy) {};
    };

} //  End namespace TwkFB

#endif // End #ifdef __TWKFBSEQUENCE_H__
