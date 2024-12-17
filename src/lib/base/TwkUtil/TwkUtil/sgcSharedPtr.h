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
//! \file sgcSharedPtr.h
//!
//! \brief Smart Pointer
//!
//
#ifndef SGC_SHARED_POINTER_H
#define SGC_SHARED_POINTER_H

// #include <sgc.h>
#include <TwkUtil/sgcRefCounted.h>

// SGC_NAMESPACE_BEGIN

template <typename RefCounted> class ConstSharedPtr;

//------------------------------------------------------------------------------------------------
//! \brief Generic smart pointer.
//!
//! Class that manages sharing of class instances.  Implements a shared
//! pointer where only
//! the last instance holding a specific instance will delete it.
//!
template <typename RefCounted> class SharedPtr
{
    friend class ConstSharedPtr<RefCounted>;

    RefCountedBase* _p;

    //! \brief  Acquire ownership of the pointer
    //!
    void _acquire() noexcept
    {
        if (_p)
            _p->incref(); // NOLINT
    }

    //! \brief  Release ownership of the pointer, destroy if its no longer used
    //!
    void _release() noexcept
    {
        if (_p && _p->decref())
            delete _p; // NOLINT
        _p = 0;
    } // NOLINT

public:
    //! \brief Constructor
    //!
    SharedPtr(RefCounted* p = 0)
        : _p(p)
    {
        _acquire();
    }

    //! \brief Copy Constructor
    //!
    SharedPtr(const SharedPtr& r)
        : _p(r._p)
    {
        _acquire();
    }

    //! \brief Move Constructor
    //!
    SharedPtr(SharedPtr&& r) noexcept
        : _p(r._p)
    {
        _acquire();
        r._release();
    }

    //! \brief Assignment operator
    //!
    SharedPtr& operator=(RefCounted* p)
    {
        reset(p);
        return *this;
    }

    //! \brief Assignment operator
    //!
    SharedPtr& operator=(const SharedPtr& r) { return *this = r.get(); }

    //! \brief Move operator
    //!
    SharedPtr& operator=(SharedPtr&& r) noexcept
    {
        *this = r.get();
        r._release();
        return *this;
    }

    //! \brief Destructor
    //!
    ~SharedPtr() { _release(); }

    //! \brief Reset the pointer to a new instance. Release the current pointer
    //
    void reset(RefCounted* p = 0)
    {
        if (_p != p)
        {
            _release();
            _p = p;
            _acquire();
        }
    }

    //! \brief Dereferencing operator.
    //!
    //! \return Pointer to the underlying instance.
    //!
    RefCounted* operator->() const { return get(); }

    //! \brief Dereferencing operator.
    //!
    //! \return Pointer to the underlying instance.
    //!
    RefCounted& operator*() { return *get(); }

    //! \brief Dereferencing operator (const).
    //!
    //! \return Pointer to the underlying instance.
    //!
    const RefCounted& operator*() const { return *get(); }

    //! \brief Check the underlying pointer for null
    //!
    //! \return false if the underlying pointer is null, otherwise true
    //!
    operator bool() const { return get() != 0x0; }

    //! \brief Count of references to the underlying instance.
    //!
    //! \return Number of SharedPtr instances with the same underlying pointer.
    //!
    int use_count() const { return _p ? _p->refcount() : 0; }

    //! \brief Basic comparison operators.
    //!
    bool operator==(const RefCounted* p) const { return get() == p; }

    bool operator!=(const RefCounted* p) const { return get() != p; }

    bool operator<(const RefCounted* p) const { return get() < p; }

    bool operator>(const RefCounted* p) const { return get() > p; }

    bool operator==(const SharedPtr& r) const { return get() == r.get(); }

    bool operator!=(const SharedPtr& r) const { return get() != r.get(); }

    bool operator<(const SharedPtr& r) const { return get() < r.get(); }

    bool operator>(const SharedPtr& r) const { return get() > r.get(); }

    //! \brief Get the instance pointer.
    //!
    //! \return Pointer to the underlying instance.
    //!
    RefCounted* get() const { return static_cast<RefCounted*>(_p); }

    //! \brief Cast the shared pointer to parent shared pointer
    //!
    //! \return shared pointer to the parent object
    //!
    template <typename RefCountedParent>
    SharedPtr<RefCountedParent> upCast() const
    {
        return SharedPtr<RefCountedParent>(
            static_cast<RefCountedParent*>(get()));
    }

    //! \brief Cast the shared pointer to child shared pointer
    //!
    //! \return shared pointer to the child object
    //!
    template <typename RefCountedChild>
    SharedPtr<RefCountedChild> downCast() const
    {
        return SharedPtr<RefCountedChild>(
            dynamic_cast<RefCountedChild*>(get()));
    }
};

//------------------------------------------------------------------------------------------------
//! \brief Generic smart pointer for const pointers.
//!
//! Class that manages sharing of class instances.  Implements a shared
//! pointer where only
//! the last instance holding a specific instance will delete it.
//!
template <typename RefCounted> class ConstSharedPtr
{
    const RefCountedBase* _p;

    //! \see SharedPtr._acquire
    //!
    void _acquire() noexcept
    {
        if (_p)
            _p->incref();
    }

    //! \see SharedPtr._release
    //!
    void _release() noexcept
    {
        if (_p && _p->decref())
            delete _p; // NOLINT
        _p = 0;
    }

public:
    //! \brief constructor
    //!
    //! \param p const pointer
    //!
    ConstSharedPtr(const RefCounted* p = 0)
        : _p(p)
    {
        _acquire();
    }

    //! \brief copy constructor
    //!
    //! \param r const shared pointer to copy
    //!
    ConstSharedPtr(const ConstSharedPtr& r)
        : _p(r._p)
    {
        _acquire();
    }

    //! \brief copy constructor
    //!
    //! \param r const shared pointer to copy
    //!
    ConstSharedPtr(ConstSharedPtr&& r) noexcept
        : _p(r._p)
    {
        _acquire();
        r._release();
    }

    //! \brief constructor from SharedPtr
    //!
    //! \param r non-const shared pointer to copy
    //!
    ConstSharedPtr(const SharedPtr<RefCounted>& r)
        : _p(r._p)
    {
        _acquire();
    }

    //! \brief destructor.  Deletes the pointer if its the last instance
    //!
    ~ConstSharedPtr() { _release(); }

    //! \brief Reset the pointer to a new instance. Release the current pointer
    //
    void reset(const RefCounted* p = 0)
    {
        if (_p != p)
        {
            _release();
            _p = p;
            _acquire();
        }
    }

    //! \see SharedPtr::operator=( const RefCounted* )
    //!
    ConstSharedPtr& operator=(const RefCounted* p)
    {
        reset(p);
        return *this;
    }

    //! \see SharedPtr:;operator=
    //!
    ConstSharedPtr& operator=(const ConstSharedPtr& r)
    {
        return *this = r.get();
    }

    //! \see SharedPtr::operator=
    //!
    ConstSharedPtr& operator=(ConstSharedPtr&& r) noexcept
    {
        *this = r.get();
        r._release();
        return *this;
    }

    //! \see SharedPtr::operator->
    //!
    const RefCounted* operator->() const { return get(); }

    //! \see SharedPtr::operator*
    //!
    const RefCounted& operator*() const { return *get(); }

    //! \see SharedPtr::operator bool
    //!
    operator bool() const { return get() != 0x0; }

    //! \see SharedPtr::operator==( const Refconted* )
    //!
    bool operator==(const RefCounted* r) const { return get() == r; }

    //! \see SharedPtr::operator!=
    //!
    bool operator!=(const RefCounted* r) const { return get() != r; }

    //! \see SharedPtr::operator<
    //!
    bool operator<(const RefCounted* r) const { return get() < r; }

    //! \see SharedPtr::operator>
    //!
    bool operator>(const RefCounted* r) const { return get() > r; }

    //! \see SharedPtr::operator==( const SharedPtr& )
    //!
    bool operator==(const ConstSharedPtr& r) const { return get() == r.get(); }

    //! \see SharedPtr::operator!=( const SharedPtr &)
    //!
    bool operator!=(const ConstSharedPtr& r) const { return get() != r.get(); }

    //! \see SharedPtr::operator<( const SharedPtr &)
    //!
    bool operator<(const ConstSharedPtr& r) const { return get() < r.get(); }

    //! \see SharedPtr::operator>( const SharedPtr &)
    //!
    bool operator>(const ConstSharedPtr& r) const { return get() > r.get(); }

    //! \see SharedPtr::operator==( const SharedPtr &)
    //!
    bool operator==(const SharedPtr<RefCounted>& r) const
    {
        return get() == r.get();
    }

    //! \see SharedPtr::operator!=( const SharedPtr &)
    //!
    bool operator!=(const SharedPtr<RefCounted>& r) const
    {
        return get() != r.get();
    }

    //! \see SharedPtr::operator<( const SharedPtr &)
    //!
    bool operator<(const SharedPtr<RefCounted>& r) const
    {
        return get() < r.get();
    }

    //! \see SharedPtr::operator>( const SharedPtr &)
    //!
    bool operator>(const SharedPtr<RefCounted>& r) const
    {
        return get() > r.get();
    }

    //! \see SharedPtr::get
    //!
    const RefCounted* get() const { return static_cast<const RefCounted*>(_p); }

    //! \see SharedPtr::upCast
    //!
    template <typename RefCountedParent>
    ConstSharedPtr<RefCountedParent> upCast() const
    {
        return ConstSharedPtr<RefCountedParent>(
            static_cast<const RefCountedParent*>(get()));
    }

    //! \see SharedPtr::downCast
    //!
    template <typename RefCountedChild>
    ConstSharedPtr<RefCountedChild> downCast() const
    {
        return ConstSharedPtr<RefCountedChild>(
            dynamic_cast<const RefCountedChild*>(get()));
    }
};

#endif
