//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/ImageFBO.h>

namespace IPCore
{
    using namespace std;
    using namespace boost;

    bool ImageFBOManager::m_imageFBOLog = false;

    ImageFBO* ImageFBOManager::newImageFBO(
        size_t width, size_t height, GLenum target, GLenum internalFormat,
        GLenum format, GLenum type, size_t fullSerialNum,
        const string& identifier, size_t expectedNumUses, size_t samples)
    {
        if (samples == 0)
            samples = 1;

        //
        //  Search of available existing GLFBO. This is an FBO which is no
        //  longer be used and has the correct structure
        //

        for (size_t i = 0; i < m_imageFBOs.size(); i++)
        {
            ImageFBO* imageFBO = m_imageFBOs[i];

            if (imageFBO->available && imageFBO->type == type
                && imageFBO->format == format && imageFBO->target == target)
            {
                const GLFBO* fbo = imageFBO->fbo();

                if (fbo->primaryColorFormat() == internalFormat
                    && fbo->multiSampleSize() == samples
                    && fbo->width() == width && fbo->height() == height)
                {
                    imageFBO->available = false;
                    imageFBO->expectedUseCount = expectedNumUses;
                    imageFBO->fullSerialNum = fullSerialNum;
                    imageFBO->identifier = identifier;

                    if (m_imageFBOLog)
                    {
                        cout << "INFO: ("
                             << ") reusing GLFBO for "
                             << identifier.substr(0, 20) << endl;

                        for (size_t i = 0; i < m_imageFBOs.size(); i++)
                        {
                            ImageFBO* f = m_imageFBOs[i];
                            string s = f->identifier;
                            if (s.size() > 50)
                                s = s.substr(0, 50);

                            cout << "INFO:     " << i << ": "
                                 << f->fbo()->width() << "x"
                                 << f->fbo()->height() << ", "
                                 << (f->available ? "available" : "used") << "/"
                                 << f->fullSerialNum << ", " << s << endl;
                        }
                    }

                    return imageFBO;
                }
            }
        }

        //
        //  No available existing FBO exists -- create a new one
        //
        GLFBO* fbo = new GLFBO(width, height, internalFormat, samples);

        fbo->newColorTexture(target, format, type);
        m_imageFBOs.push_back(new ImageFBO(fbo, true, type, format, target));
        fbo->setData(m_imageFBOs.back());

        m_imageFBOs.back()->available = false;
        m_imageFBOs.back()->expectedUseCount = expectedNumUses;
        m_imageFBOs.back()->identifier = identifier;
        m_imageFBOs.back()->fullSerialNum = fullSerialNum;

        m_totalSizeInBytes += fbo->totalSizeInBytes();

        if (m_imageFBOLog)
        {
            cout << "INFO: (" << fullSerialNum << ") new FBO created for "
                 << m_imageFBOs.size() << endl;

            for (size_t i = 0; i < m_imageFBOs.size(); i++)
            {
                ImageFBO* f = m_imageFBOs[i];
                string s = f->identifier;
                if (s.size() > 50)
                    s = s.substr(0, 50);

                cout << "INFO:     " << i << ": " << f->fbo()->width() << "x"
                     << f->fbo()->height() << ", "
                     << (f->available ? "available" : "used") << "/"
                     << f->fullSerialNum << ", " << s << endl;
            }
        }

        return m_imageFBOs.back();
    }

    ImageFBO* ImageFBOManager::newImageFBO(const IPImage* image,
                                           size_t fullSerialNum,
                                           const string& identifier)
    {
        GLenum type;
        GLenum iformat;

        switch (image->dataType)
        {
        case IPImage::HalfDataType:
            type = GL_HALF_FLOAT_ARB;
            iformat = GL_RGBA16F_ARB;
            break;
        default:
        case IPImage::FloatDataType:
            type = GL_FLOAT;
            iformat = GL_RGBA32F_ARB;
            break;
        case IPImage::UInt8DataType:
            type = GL_UNSIGNED_BYTE;
            iformat = GL_RGBA8;
            break;
        case IPImage::UInt16DataType:
            type = GL_UNSIGNED_SHORT;
            iformat = GL_RGBA16;
            break;
        case IPImage::UInt10A2DataType:
            type = GL_UNSIGNED_INT_10_10_10_2;
            iformat = GL_RGB10;
            break;
        case IPImage::UInt10A2RevDataType:
            type = GL_UNSIGNED_INT_2_10_10_10_REV;
            iformat = GL_RGB10;
            break;
        }

        return newImageFBO(image->width, image->height,
                           image->samplerType == IPImage::Rect2DSampler
                               ? GL_TEXTURE_RECTANGLE_ARB
                               : GL_TEXTURE_2D,
                           iformat, GL_RGBA, type, fullSerialNum, identifier,
                           image->node->outputs().size(), 0);
    }

    ImageFBO* ImageFBOManager::newImageFBO(const GLFBO* fbo,
                                           size_t fullSerialNum,
                                           const string& identifier)
    {
        return newImageFBO(fbo->width(), fbo->height(),
                           fbo->primaryColorTarget(), fbo->primaryColorFormat(),
                           GL_RGBA, fbo->primaryColorType(), fullSerialNum,
                           identifier);
    }

    // find existing fbo if available, and mark it unavailable.
    ImageFBO* ImageFBOManager::findExistingPaintFBO(const GLFBO* fbo,
                                                    const string& identifier,
                                                    bool& found,
                                                    size_t& lastCmdNum,
                                                    size_t fullSerialNum)
    {
        for (size_t i = 0; i < m_imageFBOs.size(); i++)
        {
            ImageFBO* imageFBO = m_imageFBOs[i];

            size_t loc = imageFBO->identifier.find("paintCmdNo");
            if (loc != string::npos)
            {
                if (strncmp(imageFBO->identifier.c_str(), identifier.c_str(),
                            loc)
                    == 0)
                {
                    // this means we found a fbo containing the IPImage and a
                    // number of commands (could be zero) we need to find out
                    // how many commands had been painted by this fbo. and
                    // continue to paint the rest
                    if (m_imageFBOLog)
                    {
                        ImageFBO* i = imageFBO;
                        cout << "INFO: found paint imageFBO "
                             << i->fbo()->width() << "x" << i->fbo()->height()
                             << ", " << (i->available ? "available" : "used")
                             << "/" << i->fullSerialNum << ", " << i->identifier
                             << endl;
                    }

                    // the identfifier finishes with "paintCmdNo" and the number
                    // of commands painted in this imageFBO
                    string n =
                        imageFBO->identifier.substr(loc + strlen("paintCmdNo"));
                    lastCmdNum = atoi(n.c_str());

                    imageFBO->available = false;
                    imageFBO->fullSerialNum = fullSerialNum;
                    imageFBO->identifier = identifier;
                    found = true;
                    return imageFBO;
                }
            }
        }

        found = false;
        lastCmdNum = 0;
        return 0;
    }

    ImageFBO* ImageFBOManager::findExistingImageFBO(const string& identifier,
                                                    size_t fullSerialNum)
    {
        for (size_t j = 0; j < m_imageFBOs.size(); j++)
        {
            ImageFBO* imageFBO = m_imageFBOs[j];

            if (imageFBO->identifier == identifier)
            {
                if (m_imageFBOLog)
                {
                    ImageFBO* i = imageFBO;
                    // string s = i->identifier;
                    // if (s.size() > 50) s = s.substr(0, 50);
                    cout << "INFO: (" << fullSerialNum << ") found imageFBO "
                         << i->fbo()->width() << "x" << i->fbo()->height()
                         << ", " << (i->available ? "available" : "used") << "/"
                         << i->fullSerialNum << ", " << i->identifier << endl;
                }

                imageFBO->available = false;
                imageFBO->fullSerialNum = fullSerialNum;
                return imageFBO;
            }
        }

        return 0;
    }

    ImageFBO* ImageFBOManager::findExistingImageFBO(GLuint textureID)
    {
        for (size_t i = 0; i < m_imageFBOs.size(); i++)
        {
            ImageFBO* imageFBO = m_imageFBOs[i];
            assert(imageFBO->fbo()->hasColorAttachment());
            if (imageFBO->fbo()->colorID(0) == textureID)
                return imageFBO;
        }

        return 0;
    }

    void ImageFBOManager::gcImageFBOs(size_t fullSerialNum)
    {
        //
        //  This is run after each main render -- FBOs all become
        //  available and those that were untouched from last time are
        //  freed (age > 1). This is a naively simple caching strategy,
        //  but seems to be adequate in practice.
        //

        for (size_t q = 0; q < m_imageFBOs.size(); q++)
        {
            ImageFBO* i = m_imageFBOs[q];
            const size_t age = fullSerialNum - i->fullSerialNum;

            i->available = true;

            size_t loc = i->identifier.find("paintCmdNo");
            if (age > 5 && loc == string::npos) // do not release the paint FBOs
            {
                if (m_imageFBOLog)
                {
                    string s = i->identifier;
                    if (s.size() > 50)
                        s = s.substr(0, 50);

                    cout << "INFO: gc (" << fullSerialNum << ") "
                         << i->fbo()->width() << "x" << i->fbo()->height()
                         << ", " << (i->available ? "available" : "used") << "/"
                         << i->fullSerialNum << ", " << s << endl;
                }

                m_totalSizeInBytes -= i->fbo()->totalSizeInBytes();

                deleteFBOFence(i->fbo());
                delete i->fbo();
                delete i;
                m_imageFBOs[q] = m_imageFBOs.back();
                m_imageFBOs.pop_back();
                q--;
            }
        }
    }

    void ImageFBOManager::insertFBOFence(const GLFBO* fbo)
    {
        ScopedLock lock(m_fboFenceMutex);
        fbo->insertFence();
        m_fboSet.insert(fbo);
        // delete m_fboFenceMap[fbo];
        // GLFence* fence = new GLFence();
        // fence->set();
        // m_fboFenceMap[fbo] = fence;
    }

    void ImageFBOManager::waitForFBOFence(const GLFBO* fbo)
    {
        ScopedLock lock(m_fboFenceMutex);
        if (fbo->state() == GLFBO::FenceInserted)
            fbo->waitForFence();
        m_fboSet.erase(fbo);

        // if (m_fboFenceMap.empty()) return;

        // GLFenceFBOMap::iterator it = m_fboFenceMap.find(fbo);

        // if (it != m_fboFenceMap.end())
        // {
        //     GLFence* f = it->second;
        //     if (!f) return;

        //     TwkUtil::Timer *timer = m_profilingState.profilingTimer;
        //     double start = (timer) ? timer->elapsed() : 0.0;

        //     m_fboFenceMap[fbo] = NULL;
        //     f->wait();
        //     delete f;

        //     if (timer) m_profilingState.fenceWaitTime += timer->elapsed() -
        //     start;
        // }
    }

    void ImageFBOManager::deleteFBOFence(const GLFBO* fbo)
    {
        ScopedLock lock(m_fboFenceMutex);
        if (fbo->state() == GLFBO::FenceInserted)
            fbo->waitForFence();
        m_fboSet.erase(fbo);

        // GLFenceFBOMap::iterator it = m_fboFenceMap.find(fbo);

        // if (it != m_fboFenceMap.end())
        // {
        //     GLFence* f = it->second;
        //     waitForFBOFence(fbo);
        //     m_fboFenceMap.erase(fbo);
        // }
    }

    void ImageFBOManager::insertTextureFence(const size_t id)
    {
        ScopedLock lock(m_textureFenceMutex);
        GLFenceTextureMap::iterator it = m_textureFenceMap.find(id);
        if (it != m_textureFenceMap.end())
        {
            if (m_imageFBOLog)
            {
                cout << "Func insertTextureFence: Fence already exists. This "
                        "should never happen."
                     << endl;
            }
            return;
        }
        GLFence* fence = new GLFence();
        m_textureFenceMap[id] = fence;
        fence->set();
    }

    void ImageFBOManager::waitForTextureFence(const size_t id)
    {
        ScopedLock lock(m_textureFenceMutex);
        if (m_textureFenceMap.empty())
            return;

        GLFenceTextureMap::iterator it = m_textureFenceMap.find(id);

        if (it != m_textureFenceMap.end())
        {
            GLFence* f = it->second;
            if (!f)
            {
                m_textureFenceMap.erase(it->first);
                return;
            }

            m_textureFenceMap[id] = NULL;
            f->wait();
            delete f;
            m_textureFenceMap.erase(it->first);
        }
    }

    void ImageFBOManager::clearAllFences()
    {
        //
        //  waitForFBOFence will remove set members while we loop over
        //  them.
        //

        while (!m_fboSet.empty())
        {
            waitForFBOFence(*m_fboSet.begin());
        }

        // for (GLFenceFBOMap::iterator i = m_fboFenceMap.begin();
        //      i != m_fboFenceMap.end();
        //      ++i)
        // {
        //     waitForFBOFence(i->first);
        // }

        ScopedLock lock2(m_fboFenceMutex);
        // m_fboFenceMap.clear();
        m_fboSet.clear();

        ScopedLock lock1(m_textureFenceMutex);
        for (GLFenceTextureMap::iterator i = m_textureFenceMap.begin();
             i != m_textureFenceMap.end(); ++i)
        {
            GLFence* f = i->second;
            if (f)
                f->wait();
            delete f;
        }

        m_textureFenceMap.clear();
    }

    void ImageFBOManager::releaseImageFBO(const GLFBO* fbo)
    {
        ImageFBO* target = fbo->data<ImageFBO>();

        if (m_imageFBOLog)
        {
            ImageFBO* i = target;
            string s = i->identifier;
            if (s.size() > 50)
                s = s.substr(0, 50);
            cout << "INFO: release " << i->fbo()->width() << "x"
                 << i->fbo()->height() << ", "
                 << (i->available ? "available" : "used") << "/"
                 << i->fullSerialNum << ", " << s << endl;
        }

        deleteFBOFence(fbo);
        target->available = true;
    }

    void ImageFBOManager::flushImageFBOs()
    {
        for (size_t i = 0; i < m_outputImageFBOs.size(); i++)
        {
            ImageFBO* t = m_outputImageFBOs[i];
            deleteFBOFence(t->fbo());
            if (t)
            {
                m_totalSizeInBytes -= t->fbo()->totalSizeInBytes();
                delete t->fbo();
            }
            delete t;
        }

        for (size_t i = 0; i < m_imageFBOs.size(); i++)
        {
            ImageFBO* t = m_imageFBOs[i];
            deleteFBOFence(t->fbo());
            if (t)
            {
                m_totalSizeInBytes -= t->fbo()->totalSizeInBytes();
                delete t->fbo();
            }
            delete t;
        }

        if (m_imageFBOLog)
        {
            cout << "INFO: flushed " << m_imageFBOs.size() << " FBOs" << endl;
        }

        m_outputImageFBOs.clear();
        m_imageFBOs.clear();
    }

    ImageFBO* ImageFBOManager::newOutputOnlyImageFBO(GLenum format, size_t w,
                                                     size_t h, size_t samples)
    {
        if (samples == 0)
            samples = 1;

        for (size_t i = 0; i < m_outputImageFBOs.size(); i++)
        {
            ImageFBO* target = m_outputImageFBOs[i];

            if (target->available)
            {
                const GLFBO* fbo = target->fbo();

                if (fbo->primaryColorFormat() == format
                    && fbo->multiSampleSize() == samples)
                {
                    target->available = false;
                    return target;
                }
            }
        }

        GLFBO* fbo = new GLFBO(w, h, format, samples);

        fbo->newColorRenderBuffer();

        m_totalSizeInBytes += fbo->totalSizeInBytes();

        ImageFBO* t = new ImageFBO(fbo);
        m_outputImageFBOs.push_back(t);

        fbo->setData(m_outputImageFBOs.back());
        return m_outputImageFBOs.back();
    }

} // namespace IPCore
