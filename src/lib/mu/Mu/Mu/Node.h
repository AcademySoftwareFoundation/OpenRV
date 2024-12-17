#ifndef __Mu__Node__h__
#define __Mu__Node__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/config.h>
#include <Mu/NodeFunc.h>
#include <Mu/Value.h>
#include <Mu/config.h>
#include <pthread.h>
#include <stdarg.h>
#include <stl_ext/block_alloc_arena.h>
#include <string>

namespace Mu
{

    class Symbol;
    class Type;
    class Node;
    class Thread;

    //
    //  Using these macros instead of actually calling the functions
    //  directly will allow the NodeFunc signature to change without
    //  rewriting the code.
    //

#define NODE_DECLARAION(NAME, TYPE) TYPE NAME(const Mu::Node&, Mu::Thread&)
#define NODE_DECLARATION(NAME, TYPE) TYPE NAME(const Mu::Node&, Mu::Thread&)
#define NODE_THIS node_
#define NODE_THREAD thread_
#define NODE_IMPLEMENTATION(NAME, TYPE) \
    TYPE NAME(const Mu::Node& NODE_THIS, Mu::Thread& NODE_THREAD)
#define NODE_EVAL() (NODE_THIS.eval(NODE_THREAD))
#define NODE_DATA(X) (static_cast<const Mu::DataNode&>(NODE_THIS)._data.as<X>())
// #define NODE_DATA(X) (static_cast<const Mu::DataNode&>(NODE_THIS)._data._ ##
// X)
#define NODE_EVAL_VALUE(N, T) ((N)->type()->nodeEval((N), (T)))
#define NODE_NUM_ARGS() (NODE_THIS.numArgs())

#define NODE_ARG(N, T)                                                        \
    (Mu::evalNodeFunc<T>(NODE_THIS.argNode(N)->func(), *NODE_THIS.argNode(N), \
                         NODE_THREAD))
#define NODE_ARG_OBJECT(N, T)                                                  \
    (Mu::evalNodeFunc<T*>(NODE_THIS.argNode(N)->func(), *NODE_THIS.argNode(N), \
                          NODE_THREAD))
#define NODE_RETURN(X) return X
#define NODE_ANY_TYPE_ARG(N) NODE_EVAL_VALUE(NODE_THIS.argNode(N), NODE_THREAD)

    //
    //  class Node
    //
    //  Expression trees are constructed using Nodes. Each node has a
    //  function associated with it which returns a Value (see
    //  Value.h). In addition, the Node instance indicates which symbol
    //  generated it and therefor what its return Value type will be.
    //
    //  Note-- there are no virtual functions in this class. You can't
    //  hold on to data that needs to be destroyed when the node is
    //  destroyed.
    //

    class Node
    {
    public:
        MU_GC_STUBBORN_NEW_DELETE

        Node()
            : _symbol(0)
            , _argv(0)
            , _func(0)
        {
        }

        Node(int nArgs, NodeFunc f, const Symbol* s) { init(nArgs, f, s); }

        Node(Node** args, const Symbol* s)
            : _argv(args)
            , _symbol(s)
            , _func(0)
        {
        }

        ~Node();

        void deleteSelf();

        //
        //	Evaluate this node. Returns the result as a Value
        //

#ifdef MU_FUNCTION_UNION
        const Value eval(Thread& t) const;
#else
        const Value eval(Thread& t) const { return (*_func)(*this, t); }
#endif

        //
        //	Evaluate and return the i'th argument's Value
        //

        size_t numArgs() const;

        const Node* argNode(int i) const { return _argv[i]; }

        Node** argv() const { return _argv; }

        void releaseArgv() { _argv = 0; }

#ifdef MU_FUNCTION_UNION
        Value valueArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._valueFunc)(*_argv[i], t);
        }

        float floatArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._floatFunc)(*_argv[i], t);
        }

        int intArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._intFunc)(*_argv[i], t);
        }

        short shortArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._shortFunc)(*_argv[i], t);
        }

        char charArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._charFunc)(*_argv[i], t);
        }

        bool boolArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._boolFunc)(*_argv[i], t);
        }

        Vector4f Vector4fArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._Vector4fFunc)(*_argv[i], t);
        }

        Vector3f Vector3fArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._Vector3fFunc)(*_argv[i], t);
        }

        Vector2f Vector2fArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._Vector2fFunc)(*_argv[i], t);
        }

        Pointer PointerArg(int i, Thread& t) const
        {
            return (*_argv[i]->_func._PointerFunc)(*_argv[i], t);
        }

        void voidArg(int i, Thread& t) const
        {
            (*_argv[i]->_func._voidFunc)(*_argv[i], t);
        }
#else
        const Value arg(int i, Thread& t) const
        {
            return (*_argv[i]->_func)(*_argv[i], t);
        }
#endif

        //
        //	The Type of the Node's return Value
        //

        const Type* type() const;

        //
        //	Return the symbol whichgenerated this Node
        //

        const Symbol* symbol() const { return _symbol; }

        //
        //	Returns a reference to the node function
        //

        NodeFunc func() const { return _func; }

        void set(const Symbol* s, NodeFunc f)
        {
            _symbol = s;
            _func = f;
        }

        //
        //  Unique identifier as a string
        //

        String mangledId() const;

        //
        //  Mark all in tree as unchanging stubborn objects
        //

        // PUT THIS BACK TO PROTECTED
    public:
        void init(int, NodeFunc, const Symbol*);
        // void		setArgs(Node*,...);
        void setArgs(Node**, int);
        void setArg(Node*, int);

        Node* argNode(int i) { return _argv[i]; }

        void markChangeEnd();

    private:
        const Symbol* _symbol;
        NodeFunc _func;
        Node** _argv; // always null terminated

        friend class NodeAssembler;
        friend class NodeVisitor;
        friend class NodePatch;
        friend class NodeSimplifier;
        friend class Thread;
    };

    //
    //  class AnnotatedNode
    //
    //  A Node which holds source code information for debugging. This is
    //  also the base class for ASTNode which holds pre-interpretable
    //  abstract representation of as-of-yet unresolved expressions.
    //

    class AnnotatedNode : public Node
    {
    public:
        MU_GC_NEW_DELETE
        AnnotatedNode();
        AnnotatedNode(int numArgs, NodeFunc F, const Symbol* s,
                      unsigned short linenum, unsigned short charnum,
                      Name file);

        virtual ~AnnotatedNode();

        unsigned short linenum() const { return _line; }

        unsigned short charnum() const { return _char; }

        Name sourceFileName() const { return _sourceFile; }

    private:
        unsigned short _line;
        unsigned short _char;
        Name _sourceFile;
    };

    //
    //  class DataNode
    //
    //  DataNode is a special type of Node which contans a Value. This
    //  Node type is necessary for leaf nodes in the expression tree
    //  (constants).
    //

    class DataNode : public Node
    {
    public:
        DataNode()
            : Node()
            , _data()
        {
            assert(_data._Pointer == 0);
        }

        DataNode(int numArgs, NodeFunc func, const Symbol* symbol);
        DataNode(Node** args, const Symbol* s);

        ~DataNode() {}

        //
        //	The additional data
        //

        Value _data;
    };

} // namespace Mu

#endif // __Mu__Node__h__
