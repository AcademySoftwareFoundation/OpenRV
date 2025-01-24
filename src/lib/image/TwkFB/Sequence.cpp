//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkFB/Sequence.h>
#include <TwkUtil/File.h>

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;

    // *****************************************************************************
    FBSequence::FBSequence(string sequencePattern)
        : m_sequencePattern(sequencePattern)
        , m_fs(NULL)
        , m_plugin(NULL)
    {
        m_fs = new FileSequence(sequencePattern);
        m_startFrame = m_fs->first();
        m_endFrame = m_fs->last();
        m_frameIncrement = m_fs->increment();

        m_plugin = GenericIO::findByExtension(extension(sequencePattern));
        if (!m_plugin)
        {
            string err = "No plugin found can read " + sequencePattern;
            TWK_EXC_THROW_WHAT(Exception, err);
        }

        m_imgInfo = m_plugin->getImageInfo(m_fs->firstFile());
    }

    // *****************************************************************************
    FBSequence::~FBSequence()
    {
        delete m_fs;
        delete m_plugin;
    }

    // *****************************************************************************
    FrameBuffer* FBSequence::frame(int frame)
    {
        assert(m_fs);
        assert(m_plugin);

        return m_plugin->readImage(m_fs->file(frame));
    }

} //  End namespace TwkFB
