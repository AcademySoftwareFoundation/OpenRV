//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__OutputGroupIPNode__h__
#define __IPCore__OutputGroupIPNode__h__
#include <IPCore/DisplayGroupIPNode.h>
#include <iostream>

namespace IPCore
{

    ///
    /// The OutputGroupIPNode is a DisplayGroupIPNode which indicates
    /// output via main memory. In other words, the resulting image of an
    /// OutputGroupIPNode is expected to be placed in a CPU accessible
    /// memory buffer for output to some other devices or file.
    //

    class OutputGroupIPNode : public DisplayGroupIPNode
    {
    public:
        OutputGroupIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0);

        virtual ~OutputGroupIPNode();

        bool isActive() const;

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
        IntProperty* m_width;
        IntProperty* m_height;
        StringProperty* m_dataType;
        FloatProperty* m_pixelAspect;
        IntProperty* m_passThrough;
    };

} // namespace IPCore

#endif // __IPCore__OutputGroupIPNode__h__
