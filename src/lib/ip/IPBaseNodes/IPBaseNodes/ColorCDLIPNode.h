//******************************************************************************
// Copyright (c) 2014 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorCDLIPNode__h__
#define __IPCore__ColorCDLIPNode__h__
#include <IPCore/IPNode.h>
#include <boost/thread.hpp>

namespace IPCore
{

    class ColorCDLIPNode : public IPNode
    {
    public:
        typedef boost::mutex Mutex;
        typedef boost::lock_guard<Mutex> LockGuard;

        ColorCDLIPNode(const std::string& name, const NodeDefinition* def,
                       IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ColorCDLIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual void copyNode(const IPNode* node);
        virtual void propertyChanged(const Property* property);
        virtual void readCompleted(const std::string& type,
                                   unsigned int version);

    private:
        void updateProperties();

        IntProperty* m_active;
        StringProperty* m_colorspace;
        StringProperty* m_file;
        Vec3fProperty* m_slope;
        Vec3fProperty* m_offset;
        Vec3fProperty* m_power;
        FloatProperty* m_saturation;
        IntProperty* m_noclamp;
        Mutex m_updateMutex;
    };

} // namespace IPCore
#endif // __IPCore__ColorCDLIPNode__h__
