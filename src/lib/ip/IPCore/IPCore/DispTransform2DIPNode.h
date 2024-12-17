//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__DispTransform2DIPNode__h__
#define __IPCore__DispTransform2DIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkMovie/Movie.h>
#include <algorithm>

namespace IPCore
{

    //
    //  class DispTransform2DIPNode
    //
    //  Converts inputs into formats that can be used by the ImageRenderer
    //

    class DispTransform2DIPNode : public IPNode
    {
    public:
        DispTransform2DIPNode(const std::string& name,
                              const NodeDefinition* def, IPGraph*,
                              GroupIPNode* group = 0);

        virtual ~DispTransform2DIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual void propertyChanged(const Property*);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual Matrix localMatrix(const Context&) const;
        virtual void readCompleted(const std::string&, unsigned int);

        static size_t transformHash() { return m_transformHash; }

    protected:
        virtual void outputDisconnect(IPNode*);

    private:
        void updateTransformHash();

        static size_t m_transformHash;

        Vec2fProperty* m_translate;
        Vec2fProperty* m_scale;
    };

} // namespace IPCore

#endif // __IPCore__DispTransform2DIPNode__h__
