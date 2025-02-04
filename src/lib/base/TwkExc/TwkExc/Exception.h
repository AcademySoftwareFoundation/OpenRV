//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkExc__Exception__h__
#define __TwkExc__Exception__h__
#include <exception>
#include <string>
#include <sstream>
#include <iostream>
#include <TwkExc/dll_defs.h>

namespace TwkExc
{

    //
    //  class Exception
    //
    //  The TwkExc::Exception class inherits from std::exception. So its
    //  possible to catch std::exception and have it work.
    //
    //  Creating a Derived Exception:
    //
    //      TWK_DERIVED_EXCEPTION(NewException)
    //              -and-
    //      TWK_DERIVED_EXCEPTION_FROM(NewException, NewerException)
    //
    //  Typical usage without macro:
    //
    //      Exception exc;
    //      exc << "Some message " << with << " some data " << end;
    //      throw exc;
    //
    //  or with macro:
    //
    //      TWK_THROW_EXC_STREAM("Some message " << with << " some data ")
    //
    //  with derived class:
    //
    //      TWK_THROW_STREAM(DerivedException,
    //                      "Some message " << with << " some data ")
    //

    class TWKEXC_EXPORT Exception : public std::exception
    {
    public:
        Exception() throw()
            : m_stream(0)
        {
        }

        Exception(const Exception&) throw();

        Exception(const char* str) throw()
            : m_string(str)
            , m_stream(0)
        {
        }

        Exception(const std::string& str) throw()
            : m_string(str)
            , m_stream(0)
        {
        }

        Exception(const std::ostringstream& str) throw()
            : m_string(str.str())
            , m_stream(0)
        {
        }

        virtual ~Exception() throw();

        void collapseStream() const;

        virtual const char* what() const throw();
        const std::string& str() const throw();
        std::string& str() throw();

        bool hasStream() const { return m_stream != 0; }

        std::ostringstream& stream()
        {
            if (!m_stream)
                m_stream = new std::ostringstream();
            return *m_stream;
        }

    protected:
        mutable std::string m_string;
        mutable std::ostringstream* m_stream;
    };

    //
    //  I/O stream functions make exceptions look like streams
    //

    template <typename T>
    inline std::ostream& operator<<(Exception& e, const T& v)
    {
        e.stream() << v;
        return e.stream();
    }

    //
    //  Will produce "ERROR: " at the beginning of each line in the
    //  exception.
    //

    TWKEXC_EXPORT std::ostream& operator<<(std::ostream& o, const Exception& e);

    //
    //  Macros
    //

#define TWK_DERIVED_EXCEPTION_FROM(BASE_TYPE, EXC_TYPE)  \
    class EXC_TYPE : public BASE_TYPE                    \
    {                                                    \
    public:                                              \
        EXC_TYPE() throw();                              \
        virtual ~EXC_TYPE() throw();                     \
        EXC_TYPE(const char* str) throw();               \
        EXC_TYPE(const std::string& str) throw();        \
        EXC_TYPE(const std::ostringstream& str) throw(); \
    };

#define TWK_DERIVED_EXCEPTION(T) \
    TWK_DERIVED_EXCEPTION_FROM(TwkExc::Exception, T)

#define TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(BASE_TYPE, EXC_TYPE, \
                                               EXPORT_TYPE)         \
    class EXPORT_TYPE EXC_TYPE : public BASE_TYPE                   \
    {                                                               \
    public:                                                         \
        EXC_TYPE() throw();                                         \
        virtual ~EXC_TYPE() throw();                                \
        EXC_TYPE(const char* str) throw();                          \
        EXC_TYPE(const std::string& str) throw();                   \
        EXC_TYPE(const std::ostringstream& str) throw();            \
    };

#define TWK_DERIVED_EXCEPTION_WITH_EXPORT(T, EXPORT_TYPE) \
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(TwkExc::Exception, T, EXPORT_TYPE)

#define TWK_DERIVED_EXCEPTION_FROM_IMP(BASE_TYPE, EXC_TYPE)   \
    EXC_TYPE::EXC_TYPE() throw()                              \
        : BASE_TYPE()                                         \
    {                                                         \
    }                                                         \
    EXC_TYPE::EXC_TYPE(const char* str) throw()               \
        : BASE_TYPE(str)                                      \
    {                                                         \
    }                                                         \
    EXC_TYPE::EXC_TYPE(const std::string& str) throw()        \
        : BASE_TYPE(str)                                      \
    {                                                         \
    }                                                         \
    EXC_TYPE::EXC_TYPE(const std::ostringstream& str) throw() \
        : BASE_TYPE(str)                                      \
    {                                                         \
    }                                                         \
    EXC_TYPE::~EXC_TYPE() throw() {}

#define TWK_DERIVED_EXCEPTION_IMP(T) \
    TWK_DERIVED_EXCEPTION_FROM_IMP(TwkExc::Exception, T)

#define TWK_THROW_STREAM(EXC, STREAM_TOKENS) \
    {                                        \
        EXC exc("");                         \
        exc << STREAM_TOKENS;                \
        throw exc;                           \
    }

#define TWK_THROW_EXC_STREAM(STREAM_TOKENS) \
    {                                       \
        TwkExc::Exception exc;              \
        exc << STREAM_TOKENS;               \
        throw exc;                          \
    }

    //----------------------------------------------------------------------
    //  DEPRECATED API FROM HERE TO THE END OF THE FILE
    //----------------------------------------------------------------------

    //
    //  Backwards compatability with TwkExc/TwkExcException.h
    //

#define TWK_EXC_DECLARE(EXC_TYPE, BASE_TYPE, TAG) \
    class EXC_TYPE : public BASE_TYPE             \
    {                                             \
    public:                                       \
        EXC_TYPE()                                \
            : BASE_TYPE(TAG)                      \
        {                                         \
        }                                         \
        EXC_TYPE(const char*)                     \
            : BASE_TYPE(TAG)                      \
        {                                         \
        }                                         \
        EXC_TYPE(const std::string&)              \
            : BASE_TYPE(TAG)                      \
        {                                         \
        }                                         \
        EXC_TYPE(const std::ostringstream&)       \
            : BASE_TYPE(TAG)                      \
        {                                         \
        }                                         \
    };

#define TWK_EXC_THROW(EXC_TYPE) \
    do                          \
    {                           \
        EXC_TYPE exc;           \
        throw(exc);             \
    } while (0);

#define TWK_EXC_THROW_WHAT(EXC_TYPE, EXC_WHAT) \
    do                                         \
    {                                          \
        EXC_TYPE exc;                          \
        exc.str().append(EXC_WHAT);            \
        throw(exc);                            \
    } while (0);

#define TWK_EXC_APPEND(EXC, EXC_WHAT) \
    do                                \
    {                                 \
        exc.str().append(EXC_WHAT);   \
    } while (0);

} // namespace TwkExc

#endif // __TwkExc__Exception__h__
