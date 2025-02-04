//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkExc/Exception.h>

namespace TwkExc
{
    using namespace std;

    Exception::Exception(const Exception& e) throw()
        : m_stream(0)
    {
        m_string = e.str();
    }

    Exception::~Exception() throw() { delete m_stream; }

    void Exception::collapseStream() const
    {
        if (m_stream)
        {
            m_string += m_stream->str();
            delete m_stream;
            m_stream = 0;
        }
    }

    const char* Exception::what() const throw()
    {
        collapseStream();
        return m_string.c_str();
    }

    const string& Exception::str() const throw()
    {
        collapseStream();
        return m_string;
    }

    string& Exception::str() throw()
    {
        collapseStream();
        return m_string;
    }

    ostream& operator<<(ostream& o, const Exception& e)
    {
        const string& s = e.str();

        o << "ERROR: ";

        for (int i = 0; i < s.size(); i++)
        {
            if (s[i] == '\n' || s[i] == '\r')
            {
                if (i != s.size() - 1)
                    o << endl << "ERROR: ";
            }
            else
            {
                o << s[i];
            }
        }

        o << endl;
        return o;
    }

} // namespace TwkExc
