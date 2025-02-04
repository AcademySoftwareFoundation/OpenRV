#ifndef __Mu__NodePrinter__h__
#define __Mu__NodePrinter__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/NodeVisitor.h>
#include <Mu/Type.h>
#include <iosfwd>

namespace Mu
{

    class Context;

    //
    //  class NodePrinter
    //
    //  A NodeVisitor that prints out the nodes in a tree to the output
    //  stream.
    //

    class NodePrinter : public NodeVisitor
    {
    public:
        enum Style
        {
            Tree,
            Lispy
        };

        NodePrinter(Node*, std::ostream&, Style = Tree);
        NodePrinter(Node*, std::ostream&, Type::ValueOutputState&,
                    Style = Tree);
        ~NodePrinter();

    protected:
        virtual void preOrderVisit(Node*, int depth);
        virtual void postOrderVisit(Node*, int depth);
        virtual void childVisit(Node*, Node*, size_t);

        std::ostream& out() { return *_ostream; }

    private:
        Style _style;
        Type::ValueOutputState* _state;
        std::ostream* _ostream;
    };

} // namespace Mu

#endif // __Mu__NodePrinter__h__
