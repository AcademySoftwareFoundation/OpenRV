//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__FolderGroupIPNode__h__
#define __IPGraph__FolderGroupIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    class SwitchIPNode;
    class LayoutGroupIPNode;
    class StackGroupIPNode;
    class CompositeIPNode;
    class PaintIPNode;

    /// FolderGroupIPNode manages a sub-graph that includes a switch node at the
    /// root

    ///
    /// The sub-graph contains one switch node as the root
    ///

    class FolderGroupIPNode : public GroupIPNode
    {
    public:
        struct InternalNodeEntry
        {
            InternalNodeEntry(const std::string& t = "",
                              const std::string& v = "")
                : value(v)
                , type(t)
                , node(0)
            {
            }

            std::string type;
            std::string value;
            IPNode* node;
        };

        typedef std::vector<InternalNodeEntry> InternalNodeEntryVector;

        FolderGroupIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0);

        virtual ~FolderGroupIPNode();

        virtual void setInputs(const IPNodes&);
        virtual IPNode* newSubGraphForInput(size_t, const IPNodes&);
        virtual void propertyChanged(const Property*);

        void rebuild();
        void updateViewType();

    private:
        InternalNodeEntryVector m_internalNodes;
        StringProperty* m_viewType;
    };

} // namespace IPCore

#endif // __IPGraph__FolderGroupIPNode__h__
