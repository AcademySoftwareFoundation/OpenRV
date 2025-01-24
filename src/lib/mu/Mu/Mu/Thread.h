#ifndef __Mu__Thread__h__
#define __Mu__Thread__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <setjmp.h>
#include <pthread.h>
#include <stl_ext/barray.h>
#include <Mu/Object.h>
#include <Mu/config.h>
#include <Mu/MuProcess.h>

namespace Mu
{

    class StackVariable;
    class ClassInstance;

    //
    //  class Thread
    //
    //  Holds state associated with a thread including the stack. Uses
    //  pthreads for the thread API.
    //
    //  There are two types of threads: Application and Process. An
    //  Application thread is a thread object associated with an external
    //  pthread. A process thread is a thread that "owns" its own
    //  pthread. Application threads cannot be manipulated by the user.
    //

    class Thread
    {
    public:
        MU_GC_NEW_DELETE

        //
        //	Types
        //

        typedef STLVector<Thread*>::Type ThreadPool;
        typedef STLEXTBarray<Value>::Type Stack;
        typedef void* StackValue;
        typedef StackValue* StackPointer;

        struct BackTraceFrame
        {
            BackTraceFrame(Node* n = 0, const Symbol* s = 0)
                : node(n)
                , symbol(s)
                , filename(0)
                , linenum(0)
                , charnum(0)
            {
            }

            operator Node*() const { return node; }

            MU_GC_NEW_DELETE

            Node* node;
            const Symbol* symbol;
            unsigned short linenum; // these will exist IFF debugging was on
            unsigned short charnum;
            const char* filename;
        };

        typedef STLVector<BackTraceFrame>::Type BackTrace;

        struct JumpPoint
        {
            unsigned int flags;
            jmp_buf env;
            size_t stackOffset;
            size_t stackSize;
        };

        typedef STLVector<JumpPoint>::Type JumpPoints;

        bool isApplicationThread() const { return _applicationThread; }

        //
        //	Returns the Process which spawned this thread.
        //

        Process* process() { return _process; }

        const Process* process() const { return _process; }

        //
        //  Returns the Context that owns the process
        //

        Context* context() { return _process->context(); }

        const Context* context() const { return _process->context(); }

        //
        //	Thread Control API
        //
        //	run() is called with the root node for the thread. run()
        //	returns immediately. Call isRunning() to find out if the
        //	thread has completed its task.
        //
        //	if block is true when you call run, the calling process will
        //	be blocked until the thread completes its task.
        //

        void run(const Node* root, bool block = true);

        bool isRunning() const { return _rootNode ? true : false; }

        void suspend();
        void resume();

        //
        //  Call will invoke the function using this thread
        //

        const Value call(const Function*, Function::ArgumentVector&,
                         bool returnArguments);

        //
        //  Similar to the above, but dynamically looks up func by
        //  name. the first argument to the ArgumentVector must be a
        //  ClassInstance object (the "this" arg).
        //

        const Value callMethodByName(const char* func,
                                     Function::ArgumentVector&,
                                     bool returnArguments);

        //
        //	This will block the calling thread until this thread has
        //	finished running.
        //

        void waitWhileRunning();

        //
        //	terminate() will cause the thread to die. If now is true, the
        //	thread will be killed. In either case, this function will
        //	delete this, the Thread object will be gone.
        //

        void terminate(bool now = false);

        //
        //	Upon completion, returnValue() will return the value of the
        //	top node. This is also the mechanism used when a return
        //	statement is executed in a non-native function.
        //

        const Value& returnValue() const { return _returnValue; }

        const Type* returnValueType() const { return _returnValueType; }

        //
        //	Stack management API
        //
        //	These functions are called by Nodes which manipulate the stack
        //	(pushing local variables, looking up variables, etc.)
        //

        Stack& stack() { return _stack; }

        const Stack& stack() const { return _stack; }

        size_t stackOffset() const { return _stackOffset; }

        //
        //  An instance of this class needs to be created on the stack to
        //  control stack function activation frames.
        //

        class StackRecord
        {
        public:
            StackRecord(Thread& t)
                : _thread(&t)
                , _size(t.stack().size())
                , _jumpSize(t._jumpPoints.size())
                , _offset(t.stackOffset())
            {
            }

            ~StackRecord()
            {
                popStackFrame();
                _thread->_jumpPoints.resize(_jumpSize);
            }

            //
            //  Call one or the other of these
            //

            void beginActivation(size_t);
            void newStackFrame(size_t);

            void setParameter(size_t n, const Value& v);
            void endParameters();

            void popStackFrame();

        private:
            StackRecord() {}

        private:
            Thread* _thread;
            size_t _size;
            size_t _jumpSize;
            size_t _offset;
        };

        friend class StackRecord;

        //
        //	Used for "break" and "continue" like functions. The "flags"
        //	argument is a bit-or'd JumpReturnCode.
        //

        void jumpPointBegin(unsigned int flags);
        jmp_buf& jumpPoint();
        void jumpPointRestore();
        void jumpPointEnd();
        void jump(int returnValue, int nPoints = 1, Value v = Value());

        struct JumpRecord
        {
            JumpRecord(Thread& t, unsigned int flags)
                : thread(t)
            {
                t.jumpPointBegin(flags);
            }

            ~JumpRecord() { thread.jumpPointEnd(); }

            Thread& thread;
        };

        //
        //  The continuation is currently used when reclaiming stack space
        //  via tail call optimization.
        //

        void setContinuation(const Node* n) { _continuation = n; }

        const Node* continuation() const { return _continuation; }

        //
        //	For method invokation, the class object pointer is cached in
        //	the thread.
        //

        void setInstanceCache(ClassInstance* i) { _classInstance = i; }

        ClassInstance* instanceCache() const { return _classInstance; }

        //
        //	When an exception object is thrown, the object is cached in
        //	the Thread in addition to possibly being throw with the
        //	Mu::Exception. It can be retrieved later.
        //

        void setException(Object* o) { _exception = o; }

        Object* exception() const { return _exception; }

        //
        //	Called if the thread catches an exception that should have been
        //	caught by the node tree
        //

        bool uncaughtException() const { return _caughtException; }

        //
        //  Stack crawling.
        //

        void backtrace(BackTrace&) const;

        //
        //	These functions are related to the operation of varying
        //	types. A Boolean VaryingObject is stored here to mask varying
        //	operations. Its questionable whether the varying types should
        //	really be properly defined in the Mu library instead of
        //	MuLang.
        //

        Object* varyingMask() const { return _varyingMask; }

        void setVaryingMask(Object* o) { _varyingMask = o; }

        bool varyingMaskSense() const { return _varyingMaskSense; }

        void setVaryingMaskSense(bool b) { _varyingMaskSense = b; }

        //
        //  Garbage collection / Continuation related
        //

        StackPointer bottomOfStack() const { return _bottomOfStack; }

        pthread_t pthread_id() const { return _id; }

        void setApplicationThreadId(pthread_t p) { _id = p; }

        //
        //  When searching for objects on the stack, these functions are
        //  called to determine what each pointer really is
        //

        bool isSymbol(const void*) const;
        bool isNode(const void*, int depth = 0) const;

    private:
        //
        //	Returns an offset which should be passed to popStackFrame() to
        //	restore the previous stack frame. Note that it will remove all
        //	frames above specified offset.
        //

        void newStackFrame(size_t size);

        void popStackFrame(size_t offset, size_t size)
        {
            _stackOffset = offset;
            _stack.resize(size);
        }

        void beginActivation(size_t size);

        void setParameter(size_t n, const Value& v) { _stack[n] = v; }

        void endParameters(size_t r) { _stackOffset = r; }

    private:
        Thread(Process*, bool appthread = false);
        ~Thread();

        //
        //	The trampoline() function is passed to pthread_create(). It
        //	merely calls main() to get the thread into the object.
        //

        static void* trampoline(void*);
        void main();
        void go();

        void setProcess(Process* p) { _process = p; }

        void* threadState();
        size_t threadStateSize();

    private:
        Process* _process;
        const Node* _rootNode;
        size_t _stackCapacity;
        Stack _stack;
        size_t _stackOffset;
        bool _alive;
        Value _returnValue;
        const Type* _returnValueType;
        JumpPoints _jumpPoints;
        ClassInstance* _classInstance;
        Object* _exception;
        bool _caughtException;
        Object* _varyingMask;
        bool _varyingMaskSense;
        StackPointer _bottomOfStack;
        bool _applicationThread;
        bool _suspended;
        size_t* _threadState;
        const Node* _continuation;

        pthread_mutex_t _controlMutex;
        pthread_cond_t _controlCond;
        pthread_mutex_t _waitMutex;
        pthread_cond_t _waitCond;
        pthread_t _id;
        pthread_attr_t _threadAttrs;

        friend class Process;
    };

    //----------------------------------------------------------------------

    inline void Thread::StackRecord::beginActivation(size_t s)
    {
        _thread->beginActivation(s);
    }

    inline void Thread::StackRecord::newStackFrame(size_t s)
    {
        _thread->newStackFrame(s);
    }

    inline void Thread::StackRecord::endParameters()
    {
        _thread->endParameters(_size);
    }

    inline void Thread::StackRecord::setParameter(size_t n, const Value& v)
    {
        _thread->setParameter(_size + n, v);
    }

    inline void Thread::StackRecord::popStackFrame()
    {
        _thread->popStackFrame(_offset, _size);
    }

} // namespace Mu

#endif // __Mu__Thread__h__
