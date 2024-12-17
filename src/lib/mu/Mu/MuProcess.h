#ifndef __Mu__Process__h__
#define __Mu__Process__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/config.h>
#include <Mu/Value.h>
#include <Mu/Function.h>
#include <pthread.h>

namespace Mu
{

    class Object;
    class Node;
    class Type;
    class Context;

    //
    //  In some applications, some state may need to be intialized
    //  before a callback can be invoked. Sub-classing this object and
    //  calling setCallEnv() with your custom CallEnvironment allows
    //  the callback site to invoke your code prior to the call back.
    //

    class CallEnvironment
    {
    public:
        CallEnvironment() {}

        virtual ~CallEnvironment() {}

        virtual const Value call(const Function*,
                                 Function::ArgumentVector&) const = 0;
        virtual const Value
        callMethodByName(const char* name, Function::ArgumentVector&) const = 0;
        virtual const Context* context() const = 0;
    };

    //
    //  class Process
    //
    //  Process holds the code, heap, and static memory segments for a
    //  Language.
    //
    //  See NodeAssembler for API to build one of these from
    //  scratch. Normally you would use that class to actually fill in the
    //  Process state.
    //

    class Process
    {
    public:
        MU_GC_NEW_DELETE

        //
        //  types
        //

        typedef STLMap<const Symbol*, Object*>::Type SymbolDocumentation;
        typedef STLVector<Value>::Type Heap;
        typedef STLVector<Object*>::Type ObjectHeap;
        typedef STLVector<Thread*>::Type Threads;
        typedef STLVector<Process*>::Type Processes;
        typedef STLVector<CallEnvironment*>::Type CallEnvStack;

        //
        //  Constructor / Destructor
        //

        //
        //  Expects an empty (no parsing/loading done) context
        //

        explicit Process(Context*);

        //
        //  Expects a process that's the result of a parse/load. This
        //  duplicates the process (including its global space) but shares
        //  the context.
        //

        explicit Process(Process*);

        ~Process();

        static const Processes& processes() { return _processes; }

        //
        //	Returns the Context passed in to the constructor
        //

        Context* context() const { return _context; }

        //
        //	Execute the Process root. This will return the return Value
        //	upon completion. returnType() tells you what the return value
        //	of the root node is. You can call returnValue() after
        //	evaluate() to get the value of the last evaluation.
        //

        const Value evaluate(Thread*);

        //
        //	Call a function in the context using this process.
        //
        //	If you pass returnArguments as true, your argument vector will
        //	reflect the state of the function's parameters after the call
        //	returns. So if the function modified one of its parameters,
        //	your argumenet vector will reflect that change. So don't put
        //	precious data into the argument vector if you're using
        //	returnArguments.
        //

        const Value call(Thread* t, const Function* f,
                         Function::ArgumentVector& args,
                         bool returnArguments = false);

        //
        //  The CallEnvironment can be set prior to execution and may be
        //  retrieved by library code that will generate callbacks. The
        //  library should use the call() function on the CallEnvironment
        //  to if it exists instead of the above call() or Thread::call().
        //
        //  By default callEnv() will return 0. Its necessary to manually
        //  initialize this. You can use BasicCallEnvironment below if
        //  there is no special state required. The CallEnvironment is
        //  owned by the CALLER not the process.
        //

        void setCallEnv(const CallEnvironment* cb);
        const CallEnvironment* callEnv() const;

        //
        //	Thread control
        //
        //  A new thread can be created. While a thread is running, its
        //  owned by a Process. The main thread is owned by the Process
        //  until the Process dies. Secondary threads are owned only as
        //  long as they are needed.
        //
        //  If you get an application thread you *must* release it when
        //  you're done.
        //

        Thread* newProcessThread();
        Thread* newApplicationThread();
        void releaseApplicationThread(Thread*);

        const Threads& threads() const { return _threads; }

        void suspendAll();
        void resumeAll();

        //
        //	Global variables statically allocated
        //

        Heap& globals() { return _globals; }

        const Heap& globals() const { return _globals; }

        //
        //  Documentation
        //
        //  If documentation is loaded, it is stored here by symbol
        //

        Object* documentSymbol(const Symbol*);

        //
        //	The root of the expression tree
        //

        Node* rootNode() { return _rootNode; }

        const Node* rootNode() const { return _rootNode; }

        //
        //	Varying size: these are process-wide parameters for all
        //	varying types.
        //

        void setVaryingSize(size_t s0, size_t s1, size_t s2);

        void setVaryingSizeDimension(size_t d, size_t s)
        {
            _varyingSize[d] = s;
        }

        size_t varyingSize(size_t d) { return _varyingSize[d]; }

        //
        //  Documentation
        //

        void addDocumentation(const Symbol* s, Object* o)
        {
            _symbolDocs[s] = o;
        }

    private:
        void clearNodes();
        void removeThread(Thread*);

    private:
        Context* _context;
        pthread_mutex_t _threadNewMutex;
        Threads _threads;
        Threads _applicationThreads;
        Threads _processThreads;
        Heap _globals;
        Node* _rootNode;
        size_t _varyingSize[3];
        SymbolDocumentation _symbolDocs;
        static Processes _processes;
        mutable Value _returnValue;
        const CallEnvironment* _cbEnv;

        friend class Thread;
        friend class NodeAssembler;
    };

    inline void Process::setVaryingSize(size_t s0, size_t s1, size_t s2)
    {
        _varyingSize[0] = s0;
        _varyingSize[1] = s1;
        _varyingSize[2] = s2;
    }

    //
    //  BasicCallEnvironment impements the simplest call environment. If
    //  you pass it a thread it will always use that thread otherwise it
    //  will create/release a new application thread on each invocation.
    //

    class BasicCallEnvironment : public CallEnvironment
    {
    public:
        BasicCallEnvironment(Process* p, Thread* t = 0)
            : _process(p)
            , _thread(0)
        {
        }

        virtual ~BasicCallEnvironment() {}

        virtual const Value call(const Function*,
                                 Function::ArgumentVector&) const;
        virtual const Value callMethodByName(const char* name,
                                             Function::ArgumentVector&) const;

        virtual const Context* context() const { return _process->context(); }

        Process* _process;
        Thread* _thread;
    };

} // namespace Mu

#endif // __Mu__Process__h__
