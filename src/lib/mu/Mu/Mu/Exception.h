#ifndef __Mu__Exception__h__
#define __Mu__Exception__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Thread.h>
#include <exception>
#include <Mu/config.h>
#include <Mu/Object.h>

namespace Mu
{

    //
    //  This is an exception that will be thrown during evaluation
    //  (possibly evaluation at parse time or compile time).
    //

    class Exception : public std::exception
    {
    public:
        MU_GC_NEW_DELETE

        typedef Thread::BackTrace BackTrace;

        Exception(const char* m = "", Object* o = 0) throw();
        Exception(Thread&, const char* m = "", Object* o = 0) throw();
        virtual ~Exception() throw();

        virtual const char* what() const throw();

        const String& message() const throw() { return _message; }

        String& message() throw() { return _message; }

        const BackTrace& backtrace() const { return _backtrace; }

        Object* object() const throw() { return _object; }

        String backtraceAsString() const;
        static String backtraceAsString(const BackTrace&);

    private:
        Object* _object;
        String _message;
        BackTrace _backtrace;
    };

#define MU_BASIC_EXC(NAME, DESCRIPTION)                   \
    class NAME##Exception : public Exception              \
    {                                                     \
    public:                                               \
        NAME##Exception(Object* o = 0) throw()            \
            : Exception(DESCRIPTION, o)                   \
        {                                                 \
        }                                                 \
        NAME##Exception(Thread& t, Object* o = 0) throw() \
            : Exception(t, DESCRIPTION, o)                \
        {                                                 \
        }                                                 \
    };

    //
    //  These two are used if MU_FLOW_SETJMP is defined. In that case break and
    //  continue type statements should be implemented using these exceptions.
    //

    MU_BASIC_EXC(Break, "break")
    MU_BASIC_EXC(Continue, "continue")
    MU_BASIC_EXC(Return, "return")
    MU_BASIC_EXC(BadDynamicCast, "bad dynamic cast")
    MU_BASIC_EXC(BadCast, "bad cast")
    MU_BASIC_EXC(NilMethodInvocation, "member function call on nil object")
    MU_BASIC_EXC(NilArgument, "nil argument to function")
    MU_BASIC_EXC(NilNodeFunc, "bad (0) NodeFunc in Function node")
    MU_BASIC_EXC(Program, "program exception")
    MU_BASIC_EXC(BadJump, "Thread::jump() unsuccessful")
    MU_BASIC_EXC(UnimplementedMethod, "unimplemented method")
    MU_BASIC_EXC(IncompatableArrays, "arrays incompatable")
    MU_BASIC_EXC(BadInternalArrayCall, "bad internal array call")
    MU_BASIC_EXC(BadInternalListCall, "bad internal list call")
    MU_BASIC_EXC(OutOfRange, "out of range")
    MU_BASIC_EXC(UnimplementedFeature, "feature not implemented")
    MU_BASIC_EXC(OutOfStackSpace, "out of stack space")
    MU_BASIC_EXC(OutOfHeapMemory, "out of heap memory")
    MU_BASIC_EXC(BadArgument, "bad argument to function")
    MU_BASIC_EXC(BadArgumentType, "incompatible argument type for function")
    MU_BASIC_EXC(AmbiguousSymbolName, "ambiguous symbol name")
    MU_BASIC_EXC(InconsistantSignature, "inconsistant signature usage")
    MU_BASIC_EXC(ArchiveContextMismatch, "archive context mismatch")
    MU_BASIC_EXC(ArchiveUnknownFormat, "archive format unknown")
    MU_BASIC_EXC(UnarchivableObject, "object not archivable")
    MU_BASIC_EXC(UnresolvedSignature, "unresolved signature");
    MU_BASIC_EXC(ArchiveReadFailure, "archive read failure");
    MU_BASIC_EXC(StreamOpenFailure, "stream open failure");
    MU_BASIC_EXC(UnresolvedFunction, "attempted call to unresolved function");
    MU_BASIC_EXC(UnresolvedReference,
                 "attempted to reference unresolved symbol");
    MU_BASIC_EXC(UnresolvableSymbol, "unable to resolve symbol");
    MU_BASIC_EXC(FileOpenError, "unable to open file");
    MU_BASIC_EXC(AbstractCall, "illegal call to abstract function")
    MU_BASIC_EXC(NodeAssembly, "construction error")
    MU_BASIC_EXC(BadInterfaceInvocation, "bad interface invocation")
    MU_BASIC_EXC(LimitedNodeAssembler,
                 "restricted action in limited node assembler")
    MU_BASIC_EXC(ParseSyntax, "syntax error")
    MU_BASIC_EXC(ThreadMismatch, "calling thread did not create thread object")
    MU_BASIC_EXC(MissingMatch, "missing matching pattern for variant")
    MU_BASIC_EXC(PatternFailed, "pattern match failed")

    // POSIX errnums
    MU_BASIC_EXC(OutOfSystemMemory, "out of system memory")
    MU_BASIC_EXC(PermissionDenied, "permission denied")
    MU_BASIC_EXC(PathnameNonexistant, "path name non-existant")
    MU_BASIC_EXC(GenericPosix, "posix exception")

    //
    //  Codes returned by setjmp and longjmp
    //

    namespace JumpReturnCode
    {
        enum
        {
            NoJump = 0,
            Continue = 0x1 << 0,
            Break = 0x1 << 1,
            Return = 0x1 << 2,
            PatternFail = 0x1 << 3,
            TailFuse = 0x1 << 4
        };

    } // namespace JumpReturnCode

} // namespace Mu

#endif // __Mu__Exception__h__
