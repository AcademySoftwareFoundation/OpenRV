//
//  Copyright (c) 2023 Autodesk
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//

#pragma once

#include <TwkMediaLibrary/Library.h>

namespace TwkMediaLibrary
{

    class PyMediaLibrary;

    enum PyNodeType
    {
        PyNoType = 0,
        PyRootType,
        PyMediaType
    };

    std::string nameFromPyNodeType(PyNodeType t);

    class PyNode : public Node
    {
    public:
        using PyNodeVector = std::vector<PyNode*>;
        using NodeAPIVector = std::vector<NodeAPI*>;

        const Node* parent() const override;
        size_t numChildren() const override;
        const Node* child(size_t index) const override;

        std::string name() const override;
        std::string typeName() const override;

    protected:
        explicit PyNode(Library*, PyNode* parent = nullptr,
                        std::string name = "", PyNodeType type = PyNoType);
        virtual ~PyNode() = default;

        void addChild(PyNode*);
        void setName(const std::string&) override;

        friend class PyMediaLibrary;

    private:
        PyNodeType m_type;
        std::string m_name;
        PyNode* m_parent;
        PyNodeVector m_children;
        NodeAPIVector m_apiVector;
        URL m_mediaURL;
    };

} // namespace TwkMediaLibrary
