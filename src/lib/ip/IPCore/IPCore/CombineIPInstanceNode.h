//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__CombineIPInstanceNode__h__
#define __IPCore__CombineIPInstanceNode__h__
#include <iostream>
#include <IPCore/IPInstanceNode.h>

namespace IPCore
{

    class CombineIPInstanceNode : public IPInstanceNode
    {
    public:
        typedef std::vector<IntProperty*> IntPropertyVector;
        typedef std::vector<StringProperty*> StringPropertyVector;

        CombineIPInstanceNode(const std::string& name,
                              const NodeDefinition* def, IPGraph* graph,
                              GroupIPNode* group = 0);

        virtual ~CombineIPInstanceNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);
        virtual void propertyDeleted(const std::string&);
        virtual void propertyChanged(const Property*);
        virtual void newPropertyCreated(const Property*);
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;
        virtual void inputImageStructureChanged(int, PropagateTarget target);
        virtual bool testInputs(const IPNodes&, std::ostringstream&) const;
        virtual void readCompleted(const std::string&, unsigned int);

    private:
        void lock() const { pthread_mutex_lock(&m_lock); }

        void unlock() const { pthread_mutex_unlock(&m_lock); }

        void contextAtInput(Context&, size_t);
        void findProps();
        void computeStructure();
        void lazyUpdateRanges() const;

    private:
        mutable bool m_structureInfoDirty;
        mutable pthread_mutex_t m_lock;
        ImageStructureInfo m_structureInfo;
        ImageRangeInfo m_rangeInfo;
        IntProperty* m_autoSize;
        IntProperty* m_outputSize;
        IntPropertyVector m_offsetProps;
        IntPropertyVector m_frameProps;
        StringPropertyVector m_layerProps;
        StringPropertyVector m_viewProps;
        StringPropertyVector m_channelProps;
        IntPropertyVector m_eyeProps;
    };

} // namespace IPCore

#endif // __IPCore__CombineIPInstanceNode__h__
