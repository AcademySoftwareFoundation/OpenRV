//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__LinearizeIPNode__h__
#define __IPGraph__LinearizeIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkMath/Chromaticities.h>

namespace IPCore
{

    //
    //  LinearizeIPNode
    //
    //  Convert from file to (linear) working space.
    //

    class LinearizeIPNode : public IPNode
    {
    public:
        typedef TwkMath::Chromaticities<float> Chromaticities;

        LinearizeIPNode(const std::string& name, const NodeDefinition* def,
                        IPGraph* graph, GroupIPNode* group = 0);

        virtual ~LinearizeIPNode();

        virtual IPImage* evaluate(const Context&);

        void setTransfer(const std::string&);
        void setPrimaries(const std::string&);
        void setAlphaType(const std::string&);

        static std::string fbPrimariesToPropertyValue(const std::string&);
        static std::string fbTransferToPropertyValue(const std::string&);
        static std::string chromaticitiesToPropertyValue(const Chromaticities&);

    private:
        IntProperty* m_active;
        StringProperty* m_alphaTypeName;
        StringProperty* m_transferName;
        StringProperty* m_primariesName;
    };

} // namespace IPCore

#endif // __IPGraph__LinearizeIPNode__h__
