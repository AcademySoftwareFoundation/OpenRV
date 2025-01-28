//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__CacheIPNode__h__
#define __IPCore__CacheIPNode__h__
#include <IPCore/IPNode.h>
#include <pthread.h>

namespace IPCore
{

    class CacheIPNode : public IPNode
    {
    public:
        CacheIPNode(const std::string& name, const NodeDefinition* def,
                    IPGraph* graph, GroupIPNode* group = 0);

        CacheIPNode(const std::string& name, const NodeDefinition* def,
                    const IPNode* sourceNode, // used in IPImage constructor
                    IPGraph* graph, GroupIPNode* group = 0);

        virtual ~CacheIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void flushAllCaches(const FlushContext&);

        void setActive(bool active) { m_active = active; }

        void lock() { pthread_mutex_lock(&m_mutex); }

        void unlock() { pthread_mutex_unlock(&m_mutex); }

        struct EvalLock
        {
            EvalLock(CacheIPNode* p)
            {
                node = p;
                node->lock();
            }

            ~EvalLock() { node->unlock(); }

            CacheIPNode* node;
        };

        void setSourceNode(const IPNode* node) { m_sourceNode = node; }

        const IPNode* sourceNode() const { return m_sourceNode; }

    private:
        void init();

    private:
        bool m_active;
        pthread_mutex_t m_mutex;
        const IPNode* m_sourceNode;
        IntProperty* m_performDownSample;
    };

} // namespace IPCore

#endif // __IPCore__CacheIPNode__h__
