///*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

//==============================================================================
// EXTERNAL DECLARATIONS
//==============================================================================

#include <TwkGLF/GLSyncObject.h>

#include <TwkUtil/Macros.h>

//==============================================================================
// CLASS GLSyncObject::Imp
//==============================================================================

class GLSyncObject::Imp
{
public:
    Imp()
        : _fenceSet(false)
    {
    }

    virtual ~Imp() {}

    Imp(const Imp&) = delete;
    Imp(Imp&&) = delete;

    Imp& operator=(const Imp&) = delete;
    Imp& operator=(Imp&&) = delete;

    virtual void setFence() = 0;
    virtual void unsetFence() = 0;
    virtual void waitFence() const = 0;
    virtual bool testFence() const = 0;

    virtual bool tryTestFence() const
    {
        if (_fenceSet)
            return testFence();
        else
            return true;
    }

protected:
    mutable bool _fenceSet;
};

//==============================================================================
// CLASS GLSyncObjectARBSync
//==============================================================================

// On Mac OS X, the GLSyncObject based on GL_ARB_sync is slighty faster
// than the one based on GL_APPLE_fence.
//
// On Linux, the GLSyncObject based on GL_ARB_sync has the same
// performance than the one based on GL_NV_fence.
//
// Since GL_ARB_sync is the ARB synchronization method and since it's
// faster on Mac OS X we would ideally use it. However, GL_ARB_sync is
// not supported on Mac OS X 10.6.

typedef struct __GLsync* GLsync;
#ifndef __linux
typedef long long GLint64;
typedef unsigned long long GLuint64;
#endif

#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#define GL_ALREADY_SIGNALED 0x911A
#define GL_TIMEOUT_EXPIRED 0x911B
#define GL_CONDITION_SATISFIED 0x911C
#define GL_WAIT_FAILED 0x911D

//------------------------------------------------------------------------------
//
class GLSyncObjectARBSync : public GLSyncObject::Imp
{
public:
    GLSyncObjectARBSync();
    virtual ~GLSyncObjectARBSync();

    virtual void setFence();
    virtual void unsetFence();
    virtual void waitFence() const;
    virtual bool testFence() const;

private:
    mutable GLsync _sync;
};

//------------------------------------------------------------------------------
//
GLSyncObjectARBSync::GLSyncObjectARBSync()
    : Imp()
    , _sync(NULL)
{
}

//------------------------------------------------------------------------------
//
GLSyncObjectARBSync::~GLSyncObjectARBSync()
{
    if (_sync)
    {
        glDeleteSync(_sync);
        TWK_GLDEBUG
    }
}

//------------------------------------------------------------------------------
//
void GLSyncObjectARBSync::setFence()
{
    RV_ASSERT_INTERNAL(!_fenceSet);
    _sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    TWK_GLDEBUG
    _fenceSet = true;
    RV_ASSERT_INTERNAL(_sync != 0);
}

//------------------------------------------------------------------------------
//
void GLSyncObjectARBSync::unsetFence()
{
    RV_ASSERT_INTERNAL(_fenceSet);
    glDeleteSync(_sync);
    TWK_GLDEBUG
    _sync = NULL;
    _fenceSet = false;
}

//------------------------------------------------------------------------------
//
void GLSyncObjectARBSync::waitFence() const
{
    RV_ASSERT_INTERNAL(_fenceSet && _sync != 0);
    GLenum ret = GL_WAIT_FAILED;
    while ((ret = glClientWaitSync(_sync, GL_SYNC_FLUSH_COMMANDS_BIT, ~0ULL))
           == GL_TIMEOUT_EXPIRED)
        ;
    assert(ret == GL_CONDITION_SATISFIED || ret == GL_ALREADY_SIGNALED);
    glDeleteSync(_sync);
    TWK_GLDEBUG
    _sync = NULL;
    _fenceSet = false;
}

//------------------------------------------------------------------------------
//
bool GLSyncObjectARBSync::testFence() const
{
    RV_ASSERT_INTERNAL(_fenceSet && _sync != 0);
    GLenum ret = glClientWaitSync(_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    TWK_GLDEBUG
    if (ret == GL_CONDITION_SATISFIED || ret == GL_ALREADY_SIGNALED)
    {
        glDeleteSync(_sync);
        TWK_GLDEBUG
        _sync = NULL;
        _fenceSet = false;
        return true;
    }
    else
    {
        return false;
    }
}

//==============================================================================
// CLASS SyncObjectFenceNV
//==============================================================================

#if defined __linux

//------------------------------------------------------------------------------
//
class SyncObjectFenceNV : public GLSyncObject::Imp
{
public:
    SyncObjectFenceNV();
    virtual ~SyncObjectFenceNV();

    virtual void setFence();
    virtual void unsetFence();
    virtual void waitFence() const;
    virtual bool testFence() const;

private:
    GLuint _fence;
};

//------------------------------------------------------------------------------
//
SyncObjectFenceNV::SyncObjectFenceNV()
    : Imp()
    , _fence(0)
{
}

//------------------------------------------------------------------------------
//
SyncObjectFenceNV::~SyncObjectFenceNV()
{
    if (_fence)
        glDeleteFencesNV(1, &_fence);
    TWK_GLDEBUG
}

//------------------------------------------------------------------------------
//
void SyncObjectFenceNV::setFence()
{
    RV_ASSERT_INTERNAL(!_fenceSet);
    if (!_fence)
    {
        glGenFencesNV(1, &_fence);
        TWK_GLDEBUG
    }
    glSetFenceNV(_fence, GL_ALL_COMPLETED_NV);
    TWK_GLDEBUG
    _fenceSet = true;
}

//------------------------------------------------------------------------------
//
void SyncObjectFenceNV::unsetFence()
{
    RV_ASSERT_INTERNAL(_fenceSet);
    glDeleteFencesNV(1, &_fence);
    _fence = 0;
    _fenceSet = false;
}

//------------------------------------------------------------------------------
//
void SyncObjectFenceNV::waitFence() const
{
    RV_ASSERT_INTERNAL(_fenceSet);
    glFinishFenceNV(_fence);
    TWK_GLDEBUG
    _fenceSet = false;
}

//------------------------------------------------------------------------------
//
bool SyncObjectFenceNV::testFence() const
{
    RV_ASSERT_INTERNAL(_fenceSet);
    bool done = glTestFenceNV(_fence);
    TWK_GLDEBUG
    if (done)
        _fenceSet = false;
    return done;
}

//==============================================================================
// CLASS SyncObjectFenceAPPLE
//==============================================================================

#elif defined __APPLE__

//------------------------------------------------------------------------------
//
class SyncObjectFenceAPPLE : public GLSyncObject::Imp
{
public:
    SyncObjectFenceAPPLE();
    virtual ~SyncObjectFenceAPPLE();

    virtual void setFence();
    virtual void unsetFence();
    virtual void waitFence() const;
    virtual bool testFence() const;

private:
    GLuint _fence;
};

//------------------------------------------------------------------------------
//
SyncObjectFenceAPPLE::SyncObjectFenceAPPLE()
    : Imp()
    , _fence(0)
{
}

//------------------------------------------------------------------------------
//
SyncObjectFenceAPPLE::~SyncObjectFenceAPPLE()
{
    if (_fence)
    {
        glDeleteFencesAPPLE(1, &_fence);
        TWK_GLDEBUG
    }
}

//------------------------------------------------------------------------------
//
void SyncObjectFenceAPPLE::setFence()
{
    RV_ASSERT_INTERNAL(!_fenceSet);
    if (!_fence)
    {
        glGenFencesAPPLE(1, &_fence);
        TWK_GLDEBUG
    }
    glSetFenceAPPLE(_fence);
    TWK_GLDEBUG
    _fenceSet = true;
}

//------------------------------------------------------------------------------
//
void SyncObjectFenceAPPLE::unsetFence()
{
    RV_ASSERT_INTERNAL(_fenceSet);
    glDeleteFencesAPPLE(1, &_fence);
    TWK_GLDEBUG
    _fence = 0;
    _fenceSet = false;
}

//------------------------------------------------------------------------------
//
void SyncObjectFenceAPPLE::waitFence() const
{
    RV_ASSERT_INTERNAL(_fenceSet);
    glFinishFenceAPPLE(_fence);
    TWK_GLDEBUG
    _fenceSet = false;
}

//------------------------------------------------------------------------------
//
bool SyncObjectFenceAPPLE::testFence() const
{
    RV_ASSERT_INTERNAL(_fenceSet);
    bool done = glTestFenceAPPLE(_fence);
    TWK_GLDEBUG
    if (done)
        _fenceSet = false;
    return done;
}

#endif

//==============================================================================
// CLASS SyncObjectStub
//==============================================================================

//------------------------------------------------------------------------------
//
class SyncObjectStub : public GLSyncObject::Imp
{
public:
    SyncObjectStub();
    virtual ~SyncObjectStub();

    virtual void setFence();
    virtual void unsetFence();
    virtual void waitFence() const;
    virtual bool testFence() const;
};

//------------------------------------------------------------------------------
//
SyncObjectStub::SyncObjectStub()
    : Imp()
{
    assert("Platform not supported");
}

//------------------------------------------------------------------------------
//
SyncObjectStub::~SyncObjectStub() {}

//------------------------------------------------------------------------------
//
void SyncObjectStub::setFence() {}

//------------------------------------------------------------------------------
//
void SyncObjectStub::unsetFence() {}

//------------------------------------------------------------------------------
//
void SyncObjectStub::waitFence() const {}

//------------------------------------------------------------------------------
//
bool SyncObjectStub::testFence() const { return false; }

//==============================================================================
// CLASS GLSyncObject
//==============================================================================

//------------------------------------------------------------------------------
//
GLSyncObject::GLSyncObject()
    : _imp(NULL)
{
    static bool supportARBSync = TWK_GL_SUPPORTS("GL_ARB_sync");
    if (supportARBSync)
    {
        _imp = new GLSyncObjectARBSync;
    }
    else
    {
#if defined __linux
        _imp = new SyncObjectFenceNV;
#elif defined __APPLE__
        _imp = new SyncObjectFenceAPPLE;
#else
        _imp = new SyncObjectStub;
#endif
    }
}

//------------------------------------------------------------------------------
//
GLSyncObject::GLSyncObject(GLSyncObject&& rhs)
    : _imp(rhs._imp)
{
    rhs._imp = nullptr;
}

//------------------------------------------------------------------------------
//
GLSyncObject& GLSyncObject::operator=(GLSyncObject&& rhs)
{
    if (this != &rhs)
    {
        delete _imp;
        _imp = rhs._imp;
        rhs._imp = nullptr;
    }
    return *this;
}

//------------------------------------------------------------------------------
//
GLSyncObject::~GLSyncObject() { delete _imp; }

//------------------------------------------------------------------------------
//
void GLSyncObject::setFence() { _imp->setFence(); }

//------------------------------------------------------------------------------
//
void GLSyncObject::waitFence() const { _imp->waitFence(); }

//------------------------------------------------------------------------------
//
bool GLSyncObject::testFence() const { return _imp->testFence(); }

//------------------------------------------------------------------------------
//
bool GLSyncObject::tryTestFence() const { return _imp->tryTestFence(); }

//------------------------------------------------------------------------------
//
void GLSyncObject::wait()
{
    setFence();
    waitFence();
}
