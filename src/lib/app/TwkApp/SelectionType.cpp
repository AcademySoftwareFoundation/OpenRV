//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkApp/SelectionType.h>
#include <stl_ext/stl_ext_algo.h>

namespace TwkApp
{
    using namespace std;

    SelectionType::TypeVector SelectionType::m_allTypes;

    SelectionType::SelectionType(const string& name)
        : m_name(name)
    {
        m_allTypes.push_back(this);
    }

    SelectionType::~SelectionType() { stl_ext::remove(m_allTypes, this); }

    SelectionType* SelectionType::findByName(const std::string& name)
    {
        for (int i = 0; i < m_allTypes.size(); i++)
        {
            if (m_allTypes[i]->name() == name)
            {
                return m_allTypes[i];
            }
        }

        return 0;
    }

} // namespace TwkApp
