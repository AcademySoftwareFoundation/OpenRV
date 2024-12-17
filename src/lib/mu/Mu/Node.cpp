//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/config.h>
#include <Mu/Node.h>
#include <Mu/Symbol.h>
#include <Mu/Type.h>
#include <assert.h>
#include <iostream>
#include <stdio.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace Mu
{

    static int initialized = 0;

    void Node::deleteSelf()
    {
#if defined(MU_USE_BASE_COLLECTOR) | defined(MU_USE_NO_COLLECTOR)
        if (this)
        {
            if (symbol()->usesDataNode())
                delete (DataNode*)this;
            else
                delete this;
        }
#endif
    }

    void Node::init(int numArgs, NodeFunc func, const Symbol* symbol)
    {
        if (numArgs)
        {
#if defined(MU_USE_BASE_COLLECTOR) || defined(MU_USE_NO_COLLECTOR)
            _argv = new Node*[numArgs + 1];
#endif

#ifdef MU_USE_BOEHM_COLLECTOR
            _argv = (Node**)(MU_GC_ALLOC((numArgs + 1) * sizeof(Node*)));
#endif

            memset(_argv, 0, sizeof(Node*) * (numArgs + 1));
        }
        else
        {
            _argv = 0;
        }

        _symbol = symbol;
        _func = func;
    }

    Node::~Node()
    {
        // assert(_argv != reinterpret_cast<Node**>(MU_DELETED));

        if (_argv)
        {
            for (Node** arg = _argv; *arg; arg++)
            {
                Node* n = *arg;
                if (n->_symbol->_datanode)
                    delete (DataNode*)n;
                else
                    delete n;
                *arg = NULL;
            }

#if defined(MU_USE_BASE_COLLECTOR) || defined(MU_USE_NO_COLLECTOR)
            delete[] _argv;
#endif
        }

        //_argv = reinterpret_cast<Node**>(MU_DELETED);
        _argv = NULL;
    }

    const Type* Node::type() const
    {
        return _symbol ? _symbol->nodeReturnType(this) : 0;
    }

    size_t Node::numArgs() const
    {
        if (_argv)
        {
            int count = 0;
            for (Node** n = _argv; *n; n++, count++)
                ;
            return count;
        }
        else
        {
            return 0;
        }
    }

    String Node::mangledId() const
    {
        char temp[80];
        snprintf(temp, 80, "n%zx", size_t(this) >> 4);
        return temp;
    }

#if 0
void 
Node::setArgs(Node *arg1, ...)
{
    va_list ap;
    va_start(ap,arg1);

    if ( !(_argv[0] = arg1) ) return;
    Node *arg;
    int count=1;

    while (arg = va_arg(ap,Node*)) 
    {
	_argv[count]=arg;
	count++;
    }

    _argv[count] = 0;

    va_end(ap);
}
#endif

    void Node::setArgs(Node** args, int numArgs)
    {
        for (int i = numArgs; i--;)
            _argv[i] = args[i];
    }

    void Node::setArg(Node* arg, int index) { _argv[index] = arg; }

#ifdef MU_FUNCTION_UNION
    const Value Node::eval(Thread& t) const
    {
        return type()->nodeEval(this, t);
    }
#endif

    void Node::markChangeEnd()
    {
        if (_argv)
            for (Node** p = _argv; *p; p++)
                (*p)->markChangeEnd();
        MU_GC_END_CHANGE_STUBBORN(this);
    }

    DataNode::DataNode(int numArgs, NodeFunc func, const Symbol* symbol)
        : Node(numArgs, func, symbol)
        , _data()
    {
        assert(symbol->usesDataNode());
        assert(_data._Pointer == 0);
    }

    DataNode::DataNode(Node** args, const Symbol* s)
        : Node(args, s)
        , _data()
    {
        assert(s->usesDataNode());
        assert(_data._Pointer == 0);
    }

    AnnotatedNode::AnnotatedNode()
        : Node()
        , _line(0)
        , _char(0)
        , _sourceFile()
    {
    }

    AnnotatedNode::AnnotatedNode(int numArgs, NodeFunc F, const Symbol* s,
                                 unsigned short linenum, unsigned short charnum,
                                 Name file)

        : Node(numArgs, F, s)
        , _line(linenum)
        , _char(charnum)
        , _sourceFile(file)
    {
    }

    AnnotatedNode::~AnnotatedNode() {}

} // namespace Mu
