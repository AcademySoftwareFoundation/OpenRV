//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkGLF/GLState.h>

namespace TwkGLF
{

    using namespace std;
    using namespace TwkMath;

    //----------------------------------------------------------------------
    // Memory management related funcs
    //

    GLuint GLState::createGLTexture(size_t s)
    {
        GLuint id;
        glGenTextures(1, &id);
        TWK_GLDEBUG;

        std::map<size_t, size_t>::iterator it = m_activeTextures.find(id);
        if (it != m_activeTextures.end())
        {
            // this really shouldn't happen. because deleteGLTextures should
            // have taken care of it
            m_totalTextureMemory += s - m_activeTextures[id];
        }
        else
        {
            m_totalTextureMemory += s;
        }
        m_activeTextures[id] = s;

        return id;
    }

    void GLState::deleteGLTexture(const GLuint& id)
    {
        std::map<size_t, size_t>::iterator it = m_activeTextures.find(id);
        if (it != m_activeTextures.end())
        {
            glDeleteTextures(1, &id);
            TWK_GLDEBUG;

            m_totalTextureMemory -= m_activeTextures[id];
            m_activeTextures.erase(id);
        }
    }

    //----------------------------------------------------------------------
    // create a new pipeline for this glProgram if none exists in m_state.
    // otherwise make the one in m_state active
    //

    GLPipeline* GLState::useGLProgram(const GLProgram* p)
    {
        // find
        PipelineMap::iterator it;
        int pid = p->programId();
        it = m_state.find(pid);
        if (it == m_state.end())
        {
            // create one
            std::pair<PipelineMap::iterator, bool> result;
            result = m_state.insert(
                std::pair<unsigned int, GLPipeline>(pid, GLPipeline(p)));
            it = result.first;
        }
        // use
        it->second.use();
        m_activeProgram = p;
        return &(m_state[pid]);
    }

    GLPipeline* GLState::activeGLPipeline()
    {
        // return the currently active pipeline
        return &(m_state[m_activeProgram->programId()]);
    }

    const GLProgram* GLState::activeGLProgram() const
    {
        return m_activeProgram;
    }

    // note that the [] operator will insert an element with key p if none
    // exists
    GLPipeline* GLState::pipeline(const GLuint p) { return &(m_state[p]); }

    void GLState::clearState()
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        TWK_GLDEBUG;
        m_state.clear();
    }

} // namespace TwkGLF
