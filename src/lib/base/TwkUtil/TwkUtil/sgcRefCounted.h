// ==================================================================
//
// Copyright (c) 2017 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
// ==================================================================
//
//!
//! \file sgcRefCounted.h
//!
//! \brief RefCounting classes
//!
//!
#ifndef SGC_CORE_REFCOUNTED_H
#define SGC_CORE_REFCOUNTED_H

#include <TwkUtil/dll_defs.h>
// #include <sgc.h>

/// Forward Declarations
///
template <typename RefCounted> class SharedPtr;

//----------------------------------------------------------------------------
//! \brief Refcounting interface for managing refcounted objects
//!
//!
class TWKUTIL_EXPORT RefCountedBase
{
    template <typename RefCounted> friend class SharedPtr;

    template <typename RefCounted> friend class ConstSharedPtr;

protected:
    //! \brief Destructor
    //!
    virtual ~RefCountedBase() {}

    //! \brief Increments the reference count
    //!
    virtual void incref() const noexcept = 0;

    //! \brief Decrements the reference count
    //!
    //! \return  True only when removing the last reference
    //!
    virtual bool decref() const noexcept = 0;

    //! \brief get the current reference count
    //!
    //! \return reference count
    //!
    //! \note The value returned may no longer be valid if the object is used
    //!       by multiple threads.
    //!
    virtual int refcount() const = 0;
};

//----------------------------------------------------------------------------
//! \brief Refcounting for single threaded objects.  This class is NOT
//!        thread-safe and is intended for more performant use with single
//!        threaded objects only
//!
//!
class TWKUTIL_EXPORT RefCountedST : public RefCountedBase
{
    mutable int m_refcount{0}; //!< current refcount

protected:
    //! \brief Destructor
    //!
    virtual ~RefCountedST() {}

    //! \brief Increments the reference count (single threaded)
    //!
    virtual void incref() const noexcept { ++m_refcount; }

    //! \brief Decrements the reference count
    //!
    //! \return  True only when removing the last reference
    //!
    virtual bool decref() const noexcept { return --m_refcount == 0; }

    //! \brief get the current reference count (single threaded)
    //!
    //! \return reference count
    //!
    virtual int refcount() const { return m_refcount; }

public:
    //! \brief Constructor
    //!
    RefCountedST() {}

    //! \brief Copy Constructor
    //!
    RefCountedST(const RefCountedST&) {}

    //! \brief Move Constructor
    //!
    RefCountedST(RefCountedST&&) noexcept {}

    //! \brief Assignment operator
    //!
    RefCountedST& operator=(const RefCountedST&) { return *this; }
};

//----------------------------------------------------------------------------
//! \brief Refcounting for multi threaded objects.  This class IS thread-safe
//!
class TWKUTIL_EXPORT RefCountedMT : public RefCountedBase
{
    class Imp;
    Imp* m_imp;

protected:
    //! \brief Destructor
    //!
    virtual ~RefCountedMT();

    //! \brief Increments the reference count in a thread-safe way
    //!
    virtual void incref() const noexcept;

    //! \brief Decrements the reference count in a thread-safe way
    //!
    //! \return  True only when removing the last reference
    //!
    virtual bool decref() const noexcept;

    //! \brief get the current reference count
    //!
    //! \return reference count
    //!
    //! \note The value returned may no longer be valid if the object is used
    //!       by multiple threads.
    //!
    virtual int refcount() const;

public:
    //! \brief Constructor
    //!
    RefCountedMT();

    //! \brief Copy Constructor
    //!
    //! \param rhs object to copy
    //!
    RefCountedMT(const RefCountedMT& rhs);

    //! \brief Move Constructor
    //!
    //! \param rhs object to move
    //!
    RefCountedMT(RefCountedMT&& rhs) noexcept;

    //! \brief Assignment operator
    //!
    //! \param rhs object to copy
    //!
    RefCountedMT& operator=(const RefCountedMT& rhs);

    //! \brief Move operator
    //!
    //! \param rhs object to copy
    //!
    RefCountedMT& operator=(RefCountedMT&& rhs) noexcept;
};

#endif
