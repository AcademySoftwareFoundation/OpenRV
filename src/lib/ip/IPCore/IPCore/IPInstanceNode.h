//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__IPInstanceNode__h__
#define __IPCore__IPInstanceNode__h__
#include <iostream>
#include <IPCore/IPNode.h>

namespace IPCore
{
    class NodeDefinition;
    class IPGraph;
    class GroupIPNode;

    namespace Shader
    {
        class Symbol;
        class BoundSymbol;
        class Expression;
    } // namespace Shader

    /// IPInstanceNode is an IPNode which implements a NodeDefinition

    ///
    /// IPInstanceNode is an instance of a NodeDefinition. The NodeDefinition
    /// completely determines the behavior of the node
    ///

    class IPInstanceNode : public IPNode
    {
    public:
        typedef std::vector<TwkFB::FrameBuffer*> FrameBufferVector;
        typedef std::vector<Shader::Expression*> ExprVector;

        IPInstanceNode(const std::string& name,
                       const NodeDefinition* definition, IPGraph* graph,
                       GroupIPNode* group = 0);

        virtual ~IPInstanceNode();

        //
        //  Fails if the number does not exactly match the number of
        //  inputImage parameters in the function definition.
        //

        virtual bool testInputs(const IPNodes&, std::ostringstream&) const;

        Shader::Expression* bind(IPImage*, Shader::Expression*,
                                 const Context&) const;

        Shader::Expression* bind(IPImage*, const ExprVector&,
                                 const Context&) const;

        Shader::BoundSymbol* boundSymbolFromSymbol(IPImage*,
                                                   const Shader::Symbol*,
                                                   const Context&) const;

        void addFillerInputs(IPImageVector& inImages);

        bool isActive() const;

    protected:
        void init();

        void lock() const { pthread_mutex_lock(&m_lock); }

        void unlock() const { pthread_mutex_unlock(&m_lock); }

        size_t numInputs() const { return m_numInputs; }

    private:
        IntProperty* m_activeProperty;
        bool m_intermediate;
        size_t m_numInputs;
        mutable pthread_mutex_t m_lock;
        size_t m_numFetches;
        size_t m_numBuffers;
        size_t m_numCoords;
    };

} // namespace IPCore

#endif // __IPCore__IPInstanceNode__h__
