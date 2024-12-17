//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__ChannelMapIPNode__h__
#define __IPGraph__ChannelMapIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkMovie/Movie.h>
#include <algorithm>

namespace IPCore
{

    //
    //  class ChannelMapIPNode
    //
    //  Maps input image channels to output image channels.
    //

    class ChannelMapIPNode : public IPNode
    {
    public:
        ChannelMapIPNode(const std::string& name, const NodeDefinition* def,
                         IPGraph* graph, GroupIPNode* group = 0);
        virtual ~ChannelMapIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void propertyChanged(const Property*);

        void setChannelMap(const std::vector<std::string>& chmap);

    protected:
        virtual void outputDisconnect(IPNode*);
        void clear();

    private:
        bool m_showErrorMessageInEvaluate;
        StringProperty* m_channels;
    };

} // namespace IPCore

#endif // __IPGraph__ChannelMapIPNode__h__
