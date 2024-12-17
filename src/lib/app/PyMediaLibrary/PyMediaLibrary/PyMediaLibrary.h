//
//  Copyright (c) 2023 Autodesk
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <TwkMediaLibrary/Library.h>

#include <iostream>
#include <map>

namespace TwkMediaLibrary
{
    class PyRootNode;
    class PyMediaNode;

    class PyMediaLibrary : public Library
    {
    public:
        using StreamingURLNodeMap = std::map<URL, PyMediaNode*>;

        explicit PyMediaLibrary(const std::string& appName);
        virtual ~PyMediaLibrary();

        size_t numNodeTypeNames() const override;
        std::string nodeTypeName(size_t) const override;

        bool isLibraryMediaURL(const URL&) const override;

        const Node* rootNode() const override;

        NodeVector associatedNodes(const URL& url) const override;
        const NodeAPI* nodeAPI(const Node*) const override;

    private:
        PyRootNode* m_root;
    };

} // namespace TwkMediaLibrary
