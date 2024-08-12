//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0 
// 

#include <Mu/Context.h>
#include <Mu/Exception.h>
#include <Mu/MuProcess.h>
#include <Mu/Thread.h>
#include <Mu/MachineRep.h>
#include <Mu/SymbolTable.h>
#include <Mu/MemberFunction.h>
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/GarbageCollector.h>
#include <algorithm>
#include <iostream>
#include <stl_ext/stl_ext_algo.h>
#include <string.h>
#include <sys/types.h>
#include <Mu/config.h>

#define MU_GC_PTR void*

#ifdef PLATFORM_DARWIN

#include <mach/thread_act.h>
#if defined(RV_ARCH_X86_64)
#include <mach/thread_status.h>
#elif defined(RV_ARCH_ARM64)
#include <mach/arm/thread_state.h>
#include <mach/arm/thread_status.h>
#endif

#ifdef ARCH_PPC32
#define darwin_thread_state ppc_thread_state
#define DARWIN_THREAD_STATE_COUNT PPC_THREAD_STATE_COUNT
#define DARWIN_THREAD_STATE PPC_THREAD_STATE
#endif

#if defined(ARCH_IA32) || defined(ARCH_IA32_64)
#define darwin_thread_state i386_thread_state
#define DARWIN_THREAD_STATE_COUNT i386_THREAD_STATE_COUNT
#define DARWIN_THREAD_STATE i386_THREAD_STATE
#endif

#if defined(RV_ARCH_ARM64)
#define darwin_thread_state arm_thread_state64_t
#define DARWIN_THREAD_STATE_COUNT ARM_THREAD_STATE64_COUNT
#define DARWIN_THREAD_STATE ARM_THREAD_STATE64
#endif

#endif

#ifdef PLATFORM_LINUX
#include <ucontext.h>
#endif

namespace Mu {
using namespace std;

#define STACK_CAPACTIY 4096

Thread::Thread(Process* p, bool appthread) : 
    _process(p), 
    _rootNode(0),
    _alive(true),
    _suspended(false),
    _stackOffset(0),
    _classInstance(0),
    _caughtException(false),
    _exception(0),
    _stackCapacity(STACK_CAPACTIY),
    _bottomOfStack(0),
    _varyingMask(0),
    _varyingMaskSense(true),
    _continuation(0),
    _applicationThread(appthread),
    _returnValueType(0),
    _threadState(0)
{
    GarbageCollector::init();
    _stack.reserve(_stackCapacity);

#ifdef PLATFORM_DARWIN
    _threadState = new size_t[sizeof(DARWIN_THREAD_STATE)];
#endif

#ifdef PLATFORM_LINUX
    _threadState = (size_t*)(new ucontext_t);
#endif

    if (isApplicationThread())
    {
        _id = pthread_self();
    }
    else
    {
        pthread_mutex_init(&_controlMutex,0);
        pthread_cond_init(&_controlCond,0);
        pthread_mutex_init(&_waitMutex,0);
        pthread_cond_init(&_waitCond,0);

        size_t stacksize;
        pthread_attr_init(&_threadAttrs);
        pthread_attr_getstacksize(&_threadAttrs, &stacksize);
        pthread_attr_setstacksize(&_threadAttrs, stacksize * 4);

        if (int err = pthread_create(&_id, &_threadAttrs, 
                                     &Thread::trampoline, this))
        {
            cerr << "Error: trying to create thread: "
                 << strerror(err) << endl << flush;
            abort();
        }
    }
}

Thread::~Thread()
{
    if (!isApplicationThread()) 
    {
        pthread_mutex_destroy(&_controlMutex);
        pthread_cond_destroy(&_controlCond);
        pthread_mutex_destroy(&_waitMutex);
        pthread_cond_destroy(&_waitCond);
        pthread_attr_destroy(&_threadAttrs);
    }

    _process->removeThread(this);
    _process = 0;

#ifdef PLATFORM_DARWIN
    delete [] _threadState;
#endif

#ifdef PLATFORM_LINUX
    delete _threadState;
#endif
}

void*
Thread::trampoline(void *arg)
{
    Thread *thread = static_cast<Thread*>(arg);
    thread->main();
    return 0;
}

void
Thread::go()
{
    void* bottomOfStack = (void*)0xf0000001;
    void** bsaddr = reinterpret_cast<void**>(&bottomOfStack);
    
    //
    //  The garbage collector has to be completely paranoid.  If this
    //  call address is higher in memory (smaller stack size) adjust
    //  the bottom of stack to compensate.
    //
    //  This will not account for doing sensationally wicked things
    //  like allocating objects in main and not declaring them
    //  external. It should suffice for "regular" usage since the
    //  collector will be conservative if this number is unnecessarily
    //  large.
    //

    if (bsaddr > _bottomOfStack)
    {
        _bottomOfStack = bsaddr;
    }

    try
    {
	_returnValue = _rootNode->eval(*this);
    }
    catch (Mu::Exception &exc)
    {
        if (!_exception) _exception = context()->exceptionObject(exc);

        _returnValue = Value(_exception);
        _returnValueType = _exception ? _exception->type() : 0;
	_caughtException = true;
    }
    catch (std::exception &exc)
    {
        _returnValue = Value(_exception);
        _returnValueType = _exception ? _exception->type() : 0;
        _caughtException = true;
    }
    catch (...)
    {
        _returnValue = Value(_exception);
        _returnValueType = _exception ? _exception->type() : 0;
	_caughtException = true;
    }
}

void
Thread::main()
{
    //
    //	Entry point for thread.
    //

    _stack.reserve(_stackCapacity);

    while (_alive)
    {
        assert(!isApplicationThread());

	if (!_rootNode)
	{
	    pthread_mutex_lock(&_controlMutex);
	    pthread_cond_wait(&_controlCond,&_controlMutex);
	    pthread_mutex_unlock(&_controlMutex);
	}

	if (_rootNode)
	{
	    go();

	    _rootNode = 0;

	    //
	    //  Inform any waiting threads
	    //

	    pthread_mutex_lock(&_waitMutex);
	    pthread_cond_signal(&_waitCond);
	    pthread_mutex_unlock(&_waitMutex);
	}
   }

    //
    //	When the thread exists the while() loop, its destined for
    //	hell.
    //

    delete this;
}

void
Thread::run(const Node *node, bool wait)
{
    if (!isApplicationThread())
    {
	assert(!isRunning());
	pthread_mutex_lock(&_controlMutex);
	if (wait) pthread_mutex_lock(&_waitMutex);
    }

    _rootNode        = node;
    _returnValueType = node->type();
    _exception       = 0;
    _returnValue     = Value();
    _caughtException = false;

    if (!isApplicationThread())
    {
	pthread_cond_signal(&_controlCond);
	pthread_mutex_unlock(&_controlMutex);

	if (wait)
	{
	    pthread_cond_wait(&_waitCond,&_waitMutex);
	    pthread_mutex_unlock(&_waitMutex);
	    _stack.clear();
	}
    }
    else
    {
        //
        // Why is an application thread required to be
        //  called by the same thread every invocation?
        //
//         if (!pthread_equal(_id, pthread_self()))
//         {
//             throw ThreadMismatchException();
//         }

	if (_rootNode) 
	{
	    _id = pthread_self();
	    go();
	}
    }

    _rootNode = 0;
}

void
Thread::waitWhileRunning()
{
    if (!isApplicationThread())
    {
	pthread_mutex_lock(&_waitMutex);
	pthread_cond_wait(&_waitCond,&_waitMutex);
	pthread_mutex_unlock(&_waitMutex);
    }

    if (_exception)
    {
	throw ProgramException(_exception);
    }
}

const Value         
Thread::callMethodByName(const char* funcName,
                         Function::ArgumentVector& args,
                         bool returnArguments)
{
    const Context* c = context();

    if (Name n = c->lookupName(funcName))
    {
        ClassInstance* obj = reinterpret_cast<ClassInstance*>(args[0].as<Pointer>());
        if (!obj || args.empty()) throw NilMethodInvocationException();

        const Class* t = obj->classType();

        if (const MemberFunction* F = t->findSymbolOfType<MemberFunction>(n))
        {
            return call(F, args, returnArguments);
        }
    }

    throw UnresolvedFunctionException();
    return Value();
}

const Value
Thread::call(const Function* f,
             Function::ArgumentVector& args,
             bool returnArguments)
{
    Value v;

    if (f->body())
    {
        size_t s = f->stackSize();
        Thread::StackRecord record(*this);
        record.beginActivation(s);

        for (int i=0, s=args.size(); i < s; i++)
        {
            record.setParameter(i, args[i]);
        }

        record.endParameters();
        const Node *body = f->body();

        if (NodeFunc func = body->func())
        {
            jumpPointBegin(JumpReturnCode::Return);
            int rv = SETJMP(jumpPoint());

            if (rv == JumpReturnCode::NoJump)
            {
                run(body, true);
                //v = body->eval(*thread);
            }
            else
            {
                jumpPointRestore();
            }

            v = returnValue();
            jumpPointEnd();

            if (returnArguments)
            {
                for (int i=0, s=args.size(); i < s; i++)
                {
                    args[i] = stack()[i];
                }
            }
        }
        else
        {
            throw NilNodeFuncException();
        }
    }
    else
    {
        if (returnArguments) throw UnimplementedFeatureException();

        size_t n = f->numArgs();
        Node* body = new Node(n, f->func(), f);

        for (int i=0; i < n; i++)
        {
            const Type* t = f->argType(i);
            DataNode* dn = new DataNode(0, t->machineRep()->constantFunc(), t);
            dn->_data = args[i];
            body->argv()[i] = dn;
        }

        v = body->eval(*this);
        body->deleteSelf();
    }

    return v;
}

void
Thread::terminate(bool now)
{
    if (isApplicationThread())
    {
        _alive = false;
    }
    else
    {
        now = false;    // can't do immediate termination yet.

        if (now)
        {
            _alive = false;
            // pthread_cancel
        }
        else
        {
            _alive = false;
            if (!isRunning()) run(0,0);
        }
    }

    delete this;
}

void
Thread::suspend()
{
    if (_alive && !_suspended)
    {
        _suspended = true;

#if 0
#ifdef PLATFORM_DARWIN
        
        if (thread_suspend(pthread_mach_thread_np(_id)) != KERN_SUCCESS)
        {
            cerr << "Thread::suspend(): thread_suspend() failed" << endl;
            _alive = false;
        }
#endif
#endif

    }
}

void
Thread::resume()
{
    if (_alive && _suspended)
    {
        _suspended = false;

#if 0
#ifdef PLATFORM_DARWIN
        if (thread_resume(pthread_mach_thread_np(_id)) != KERN_SUCCESS)
        {
            cerr << "Thread::resume(): thread_resume() failed" << endl;
        }
#endif
#endif

    }
}

void*
Thread::threadState()
{
#ifdef PLATFORM_DARWIN
    if (_alive)
    {
        mach_msg_type_number_t thread_state_count = DARWIN_THREAD_STATE_COUNT;
        kern_return_t krc;
        if ((krc = thread_get_state(pthread_mach_thread_np(_id), // <-- can't use self here
                                    DARWIN_THREAD_STATE,
                                    (natural_t*)_threadState, 
                                    &thread_state_count)) != KERN_SUCCESS)
        {
            return _threadState;
        }
        else
        {
            cerr << "Thread::threadState(): error obtaining thread state" << endl;
        }
    }
#endif

    return 0;
}

size_t
Thread::threadStateSize()
{
#if 0
#ifdef PLATFORM_DARWIN
    return sizeof(darwin_thread_state);
#endif

#ifdef PLATFORM_LINUX
    return sizeof(ucontext);
#endif

#endif
	/* AJG - baaaaad ! */
	return sizeof(int);
}

void
Thread::jump(int returnValue, int n, Value v)
{
    if (_jumpPoints.size())
    {
	while ( !(_jumpPoints.back().flags & returnValue) )
	{
	    _jumpPoints.pop_back();
	}

	_returnValue = v;
	LONGJMP(_jumpPoints.back().env, returnValue);
    }

    throw BadJumpException();
}

//----------------------------------------------------------------------
void
Thread::jumpPointBegin(unsigned int flags)
{
    JumpPoint jp;
    jp.flags	   = flags;
    jp.stackSize   = _stack.size();
    jp.stackOffset = _stackOffset;
    _jumpPoints.push_back(jp);
}

jmp_buf&
Thread::jumpPoint() 
{ 
    return _jumpPoints.back().env;
}

void
Thread::jumpPointRestore()
{
    assert(_jumpPoints.size());
    JumpPoint &jp = _jumpPoints.back();
    _stack.resize(jp.stackSize);
    _stackOffset = jp.stackOffset;
}

void
Thread::jumpPointEnd()
{
    _jumpPoints.pop_back();
}

//----------------------------------------------------------------------

void
Thread::beginActivation(size_t size)
{
    //
    //	This is used for calling interpreted functions.
    //

#if 0
    if (_stack.capacity() < _stack.size() + size)
    {
	throw OutOfStackSpaceException();
    }
#endif

    _stack.resize(_stack.size()+size);
}


void
Thread::newStackFrame(size_t size)
{
    //
    //	Adds new entries on the local variable stack and returns the old
    //	stack size. The caller is responsible for keeping track of this
    //	data. This makes it possible to do stack unwinding efficiently and
    //	without any overhead.
    //

    _stackOffset = _stack.size();

#if 0
    if (_stack.capacity() < _stack.size() + size)
    {
	throw OutOfStackSpaceException();
    }
#endif

    _stack.resize(_stack.size()+size);
}


//----------------------------------------------------------------------


bool
Thread::isSymbol(const void* p) const
{
    if (!p) return true;

    if (MU_GC_PTR base = GC_base((MU_GC_PTR)p))
    {
        if (GC_size(base) >= sizeof(Symbol))
        {
            const Symbol* maybeSymbol = (const Symbol*)p;

            if (!maybeSymbol->scope())
            {
                return maybeSymbol == context()->globalScope();
            }

            if (maybeSymbol->context() == context() &&
                isSymbol(maybeSymbol->scope()) &&
                isSymbol(maybeSymbol->nextOverload()))
            {
                MU_GC_PTR stbase = GC_base(maybeSymbol->symbolTable());

                if (!stbase) return true;

                if (GC_size(stbase) >= sizeof(SymbolTable))
                {
                    const Name::Item* item = maybeSymbol->name().nameRef();

                    MU_GC_PTR ibase = GC_base((void*)item);

                    if (ibase && GC_size(ibase) >= sizeof(Name::Item))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool
Thread::isNode(const void* p, int depth) const
{
    if (MU_GC_PTR base = GC_base((MU_GC_PTR)p))
    {
        if (GC_size(base) >= sizeof(Node))
        {
            const Node* maybeNode = (const Node*)p;

            if (isSymbol(maybeNode->symbol()))
            {
                MU_GC_PTR argvBase = GC_base(maybeNode->_argv);

                if (maybeNode->_argv && argvBase)
                {
                    const size_t maxArgs = GC_size(argvBase);
                    size_t count = 0;

                    for (Node** np = maybeNode->_argv; *np && depth; np++, count++)
                    {
                        if (count >= maxArgs || !isNode(*np, depth-1)) return false;
                    }
                }

                return true;
            }
        }
    }

    return false;
}

void
Thread::backtrace(BackTrace& trace) const
{
    size_t top = 0xbeefc0de;
    vector<size_t*> spointers;

#ifdef MU_USE_BOEHM_COLLECTOR
    const size_t sizeOfNode = GC_size(GC_base(new Node()));
    const size_t sizeOfAnnNode = GC_size(GC_base(new AnnotatedNode()));
    const size_t sizeOfDataNode = GC_size(GC_base(new DataNode()));
#endif

    //
    //  Start by walking down the stack and finding any node pointers
    //
    
    for (size_t* t = &top; size_t(t) <= size_t(_bottomOfStack); t++)
    {
        size_t data = *t;

        if (data == size_t(this))
        {
            //
            //  The thread pointer
            //

            spointers.push_back(t);
        }


        if (MU_GC_PTR ptr = GC_base((MU_GC_PTR)data))
        {
            //
            //  The Boehm version has to do a lot of checks to
            //  determine if the pointer really is a Node or not. 
            //

            if ((GC_size(ptr) == sizeOfNode || 
                 GC_size(ptr) == sizeOfAnnNode ||
                 GC_size(ptr) == sizeOfDataNode) &&
                isNode((void*)data, 1))
            {
                const Node* n = (const Node*)data;

                if (dynamic_cast<const Function*>(n->symbol()))
                {
                    spointers.push_back(t);
                }
            }
        }
    }

    if (!spointers.empty())
    {
        for (size_t i = 0; i < (spointers.size()-1); i++)
        {
            size_t* t = spointers[i];
            size_t data = *t;

            if (data == size_t(this)) continue;

            bool nextIsThread = (size_t(this) == *spointers[i+1]) && (t = spointers[i+1]-1);

            if (data && nextIsThread)
            {
                Node* n = (Node*)data;
                if (isNode(n, 1))
                {
                    if (trace.empty() || trace.back().node != n)
                    {
                        trace.push_back(BackTraceFrame(n, n->symbol()));
                    }
                }
            }
        }
    }

    if (trace.empty()) return;

    Node* child = trace.front().node;
    
#if CLEANUP_GARBAGE
    for (int i=1; i < trace.size(); i++)
    {
        bool changed = false;
        Node* n = trace[i].node;
        bool exists = false;

        //
        //  Can't do this because it prevents recursive backtraces
        //

        // for (int j=i-1; j >= 0; j--)
        // {
        //     if (trace[j].node == n) { exists = true; break; }
        // }

        // if (exists)
        // {
        //     trace[i] = BackTraceFrame();
        //     continue;
        // }

        if (const Function* F = dynamic_cast<const Function*>(n->symbol()))
        {
            if (F->body() == child)
            {
                changed = true;
                child = n;
            }
        }

        if (!changed)
        {
            for (int q=0, s=n->numArgs(); q < s; q++)
            {
                if (n->argNode(q) == child)
                {
                    child = n;
                    changed = true;
                    break;
                }
                else if (const Function* f = 
                         dynamic_cast<const Function*>(n->symbol()))
                {
                    if (!f->native())
                    {
                        if (const MemberFunction* mf =
                            dynamic_cast<const MemberFunction*>(f))
                        {
                            MemberFunction::MemberFunctionVector funcs;
                            mf->findOverridingFunctions(funcs);
                            funcs.push_back(mf);

                            for (int j=0; j < funcs.size(); j++)
                            {
                                if (funcs[j]->body() == child)
                                {
                                    child = n;
                                    changed = true;
                                    trace[i].symbol = funcs[j];
                                    break;
                                }
                            }

                            if (changed) break;
                        }
                        else if (f->body() == child)
                        {
                            child = n;
                            changed = true;
                            break;
                        }
                    }
                }
            }
        }

        if (!changed) trace[i] = BackTraceFrame();
    }

    trace.erase(remove_if(trace.begin(), 
                          trace.end(), 
                          stl_ext::IsNull_p<Node>()),
                trace.end());
#endif

    const bool debugging = this->context()->debugging();

    for (int i=0; i < trace.size(); i++)
    {
        const Symbol* symbol = trace[i].symbol;
        const Node*   node   = trace[i].node;

        if (debugging)
        {
            if (dynamic_cast<const Type*>(symbol))
            {
                //
                //  Its a constant value node
                //
            }
            else if (const Function* F = dynamic_cast<const Function*>(symbol))
            {
                if (F->hasHiddenArgument())
                {
                    // its a DataNode
                }
                else
                {
                    const AnnotatedNode* anode = static_cast<const AnnotatedNode*>(node);
                    trace[i].linenum  = anode->linenum();
                    trace[i].charnum  = anode->charnum();
                    trace[i].filename = anode->sourceFileName().c_str();
                }
            }
            else
            {
                // ? probably an ASTNode of some sort
            }
        }
    }
}

} // namespace Mu
