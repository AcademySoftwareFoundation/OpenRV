//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPBaseNodes__FileOutputGroupIPNode__h__
#define __IPBaseNodes__FileOutputGroupIPNode__h__
#include <iostream>
#include <IPCore/OutputGroupIPNode.h>

namespace IPCore
{
    class RetimeIPNode;

    class FileOutputGroupIPNode : public OutputGroupIPNode
    {
    public:
        FileOutputGroupIPNode(const std::string& name,
                              const NodeDefinition* def, IPGraph* graph,
                              GroupIPNode* group = 0);

        virtual ~FileOutputGroupIPNode();

        virtual IPImage* evaluate(const Context&);

        virtual void propertyChanged(const Property*);

    protected:
        StringProperty* m_filename;
        StringProperty* m_channels;
        FloatProperty* m_fps;
        StringProperty* m_timeRange;
        RetimeIPNode* m_retimeNode;
    };

} // namespace IPCore

#endif // __IPBaseNodes__FileOutputGroupIPNode__h__
