//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorTemperatureIPNode__h__
#define __IPCore__ColorTemperatureIPNode__h__
#include <IPCore/IPNode.h>

namespace IPCore
{

    //
    //  Operating in gamma space
    //

    class ColorTemperatureIPNode : public IPNode
    {
    public:
        ColorTemperatureIPNode(const std::string& name,
                               const NodeDefinition* def, IPGraph* graph,
                               GroupIPNode* group = 0);

        virtual ~ColorTemperatureIPNode();

        virtual IPImage* evaluate(const Context&);

        enum
        {
            BRADFORD,
            TANNER,
            DANIELLE
        };

    private:
        FloatProperty* m_inTemperature;
        FloatProperty* m_outTemperature;
        Vec2fProperty* m_inWhitePrimary;
        IntProperty* m_active;
        IntProperty* m_method;
    };

} // namespace IPCore
#endif // __IPCore__ColorTemperatureIPNode__h__
