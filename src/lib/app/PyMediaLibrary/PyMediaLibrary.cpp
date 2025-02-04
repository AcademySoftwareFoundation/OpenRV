//
//  Copyright (c) 2023 Autodesk, Inc. All Rights Reserved.
//  SPDX-License-Identifier: Apache-2.0
//
#include <PyMediaLibrary/PyMediaLibrary.h>
#include <PyMediaLibrary/PyNode.h>
#include <PyMediaLibrary/PyMediaNode.h>
#include <PyMediaLibrary/PyRootNode.h>

namespace TwkMediaLibrary
{
    PyMediaLibrary::PyMediaLibrary(const std::string& appName)
        : Library("py-media-library", appName)
        , m_root(new PyRootNode(this))
    {
    }

    PyMediaLibrary::~PyMediaLibrary() { delete m_root; }

    const Node* PyMediaLibrary::rootNode() const { return m_root; }

    size_t PyMediaLibrary::numNodeTypeNames() const { return 2; }

    std::string PyMediaLibrary::nodeTypeName(size_t i) const
    {
        return nameFromPyNodeType(PyNodeType(PyNoType + i + 1));
    }

    const NodeAPI* PyMediaLibrary::nodeAPI(const Node* innode) const
    {
        if (const auto* node = dynamic_cast<const NodeAPI*>(innode))
        {
            return node;
        }

        return nullptr;
    }

    bool PyMediaLibrary::isLibraryMediaURL(const URL& inURL) const
    {
        if (const auto* root = dynamic_cast<const PyRootNode*>(rootNode()))
        {
            return root->isLibraryMediaURL(inURL);
        }

        return false;
    }

    NodeVector PyMediaLibrary::associatedNodes(const URL& inURL) const
    {
        NodeVector nodes;

        if (const auto* root = dynamic_cast<const PyRootNode*>(rootNode()))
        {
            const MediaNodeVector mediaNodes = root->mediaNodesForURL(inURL);

            for (const auto* mediaNode : mediaNodes)
            {
                nodes.push_back(mediaNode);
            }
        }

        return nodes;
    }

} // namespace TwkMediaLibrary
