//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkFB/Attribute.h>
#include <stl_ext/replace_alloc.h>
#include <iostream>
#include <assert.h>
#include <stdlib.h>

namespace TwkFB
{
    using namespace std;

    TWK_CLASS_NEW_DELETE(FBAttribute)

    FBAttribute* DataContainerAttribute::copy() const
    {
        return new DataContainerAttribute(name(), data(), size());
    }

    FBAttribute*
    DataContainerAttribute::copyWithPrefix(const string& prefix) const
    {
        return new DataContainerAttribute(prefix + name(), data(), size());
    }

    string DataContainerAttribute::valueAsString() const
    {
        ostringstream str;
        str << "(" << size() << " bytes of data)";
        return str.str();
    }

    template <>
    TypedFBAttribute<const char*>::TypedFBAttribute(const std::string& name,
                                                    const char* value)
        : FBAttribute(name)
    {
        //  FRIENDLY REMINDER: USE A STRING ATTR INSTEAD OF CHAR*
        cout << "ERROR: TypedFBAttribute<T> where T is const char* is not "
                "allowed"
             << endl;
        abort();
    }

    template <>
    TypedFBAttribute<char*>::TypedFBAttribute(const std::string& name,
                                              char* value)
        : FBAttribute(name)
    {
        //  FRIENDLY REMINDER: USE A STRING ATTR INSTEAD OF CHAR*
        cout << "ERROR: TypedFBAttribute<T> where T is char* is not allowed"
             << endl;
        abort();
    }

} // namespace TwkFB
