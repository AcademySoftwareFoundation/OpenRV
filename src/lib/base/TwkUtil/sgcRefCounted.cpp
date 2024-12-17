// ==================================================================
//
// Copyright (c) 2017 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
// ==================================================================
//

#include <TwkUtil/sgcRefCounted.h>

#include <atomic>

//------------------------------------------------------------------------------
//
// CLASS RefCountedMT::Imp
//
class RefCountedMT::Imp
{
    mutable std::atomic<int> m_refcount{0};

public:
    //----------------------------------------------------------------------------
    //
    Imp() {}

    //----------------------------------------------------------------------------
    //
    explicit Imp(const Imp&) {}

    //----------------------------------------------------------------------------
    //
    Imp& operator=(const Imp&) { return *this; }

    //----------------------------------------------------------------------------
    //
    virtual ~Imp() {}

    //----------------------------------------------------------------------------
    //
    void incref() const noexcept { ++m_refcount; }

    //----------------------------------------------------------------------------
    //
    bool decref() const noexcept { return --m_refcount == 0; }

    //----------------------------------------------------------------------------
    //
    int refcount() const { return m_refcount; }
};

//------------------------------------------------------------------------------
//
// CLASS RefCountedMT
//
RefCountedMT::RefCountedMT()
    : m_imp(new Imp())
{
}

//------------------------------------------------------------------------------
//
RefCountedMT::RefCountedMT(const RefCountedMT& rhs)
    : m_imp(new Imp(*rhs.m_imp))
{
}

//------------------------------------------------------------------------------
//
RefCountedMT::RefCountedMT(RefCountedMT&& rhs) noexcept
    : m_imp(rhs.m_imp)
{
    rhs.m_imp = nullptr;
}

//------------------------------------------------------------------------------
//
RefCountedMT& RefCountedMT::operator=(const RefCountedMT& rhs)
{
    if (this != &rhs)
        *m_imp = *rhs.m_imp;
    return *this;
}

//------------------------------------------------------------------------------
//
RefCountedMT& RefCountedMT::operator=(RefCountedMT&& rhs) noexcept
{
    if (this != &rhs)
    {
        delete m_imp;
        m_imp = rhs.m_imp;
        rhs.m_imp = nullptr;
    }
    return *this;
}

//------------------------------------------------------------------------------
//
RefCountedMT::~RefCountedMT() { delete m_imp; }

//------------------------------------------------------------------------------
//
void RefCountedMT::incref() const noexcept { m_imp->incref(); }

//------------------------------------------------------------------------------
//
bool RefCountedMT::decref() const noexcept { return m_imp->decref(); }

//------------------------------------------------------------------------------
//
int RefCountedMT::refcount() const { return m_imp->refcount(); }
