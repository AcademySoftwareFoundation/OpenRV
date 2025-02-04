//
//  Copyright (c) 2023 Autodesk
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//

#pragma once

#include <PyMediaLibrary/PyRootNode.h>
#include <PyMediaLibrary/PyNode.h>

#include <TwkMediaLibrary/Library.h>

#include <Python.h>

#include <vector>

namespace TwkMediaLibrary
{
    class PyMediaNode;

    using MediaNodeVector = std::vector<PyMediaNode*>;

    class PyRootNode : public PyNode
    {
    public:
        PyRootNode(Library* lib);
        virtual ~PyRootNode();

        bool isLibraryMediaURL(const URL&) const;
        MediaNodeVector mediaNodesForURL(const URL&) const;
    };
} // namespace TwkMediaLibrary
