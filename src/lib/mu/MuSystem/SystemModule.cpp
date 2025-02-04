//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

extern "C"
{
    extern char** environ;
}
#ifdef _MSC_VER
//
//  We are targetting at least XP, (IE no windows95, etc).
//
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#include <windows.h>
#include <winnt.h>
#include <wincon.h>
#include <pthread.h>
//
//  NOTE: win_pthreads, which supplies implement.h, seems
//  targetted at an earlier version of windows (pre-XP).  If you
//  include implement.h here, it won't compile.  But as far as I
//  can tell, it's not needed, so just leave it out.
//
//  #include <implement.h>
#endif

#include <MuSystem/SystemModule.h>
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/List.h>
#include <Mu/ListType.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/Signature.h>
#include <Mu/StructType.h>
#include <Mu/Symbol.h>
#include <Mu/SymbolTable.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <Mu/OpaqueType.h>
#include <MuLang/StringType.h>
#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
extern "C" int getdtablesize();
#else
#include <time.h>
#endif

// ajg2 - Is there some way to warn via the code that getuid, etc. returns 0 on
// win32?
#if defined _MSC_VER
#include <direct.h>
#else
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

#ifndef WCOREDUMP
#define WCOREDUMP(x) false
#endif

#define SYMCONST(name) new SymbolicConstant(c, #name, "int", Value(name))

#ifdef WIN32

struct timezone
{
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL
#endif

int gettimeofday(struct timeval* tv, struct timezone* tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag;

    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres /= 10; /*convert into microseconds*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}

#endif

namespace Mu
{
    using namespace std;

    //
    //  Structs internal to the System module in C++ form
    //

    struct TimeSpec
    {
        int64 tv_sec;
        int tv_nsec;
    };

    struct StatStruct
    {
        int dev;
        int ino;
        short mode;
        short nlink;
        int uid;
        int gid;
        int64 size;
        ClassInstance* atime;
        ClassInstance* mtime;
        ClassInstance* ctime;
    };

    //
    //
    //

    Module* SystemModule::init(const char* name, Context* context)
    {
        Module* m = new SystemModule(context, name);
        MuLangContext* c = static_cast<MuLangContext*>(context);
        Symbol* s = c->globalScope();
        s->addSymbol(m);
        return m;
    }

    SystemModule::SystemModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    SystemModule::~SystemModule() {}

    void SystemModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        context->arrayType(context->intType(), 1, 0);
        context->arrayType(context->stringType(), 1, 0);
        context->arrayType(context->byteType(), 1, 0);

        Context::TypeVector sl(2);
        sl[0] = context->intType();
        sl[1] = context->intType();
        context->tupleType(sl);

        Context::TypeVector sockname(2);
        sockname[0] = context->stringType();
        sockname[1] = context->intType();
        context->tupleType(sockname);

        sl[0] = context->int64Type(); // gettimeofday() (int64,int)
        sl[1] = context->intType();
        context->tupleType(sl);

        const Type* int_T = context->intType();
        const Type* short_T = context->shortType();
        const Type* int64_T = context->int64Type();

        Context::NameValuePairs timespecFields(2);
        timespecFields[0] = make_pair(string("tv_sec"), int64_T);
        timespecFields[1] = make_pair(string("tv_nsec"), int_T);
        const Type* timespec_T =
            context->structType(this, "timespec", timespecFields);

        //
        //  These may have to change. The largest file size that can be
        //  recorded here is 2^63 which is smaller than ZFS can handle
        //  (2^128). But since we don't really have an int128 (yet) this
        //  will have to do.
        //

        Context::NameValuePairs statFields(10);
        statFields[0] = make_pair(string("st_dev"), int_T);
        statFields[1] = make_pair(string("st_ino"), int_T);
        statFields[2] = make_pair(string("st_mode"), short_T);
        statFields[3] = make_pair(string("st_nlink"), short_T);
        statFields[4] = make_pair(string("st_uid"), int_T);
        statFields[5] = make_pair(string("st_gid"), int_T);
        statFields[6] = make_pair(string("st_size"), int64_T);
        statFields[7] = make_pair(string("st_atime"), timespec_T);
        statFields[8] = make_pair(string("st_mtime"), timespec_T);
        statFields[9] = make_pair(string("st_ctime"), timespec_T);
        const StructType* stat_T =
            context->structType(this, "statinfo", statFields);

        //
        //  Make the function type (void;)
        //

        Signature* sig = new Signature;
        sig->push_back(context->voidType());
        FunctionType* voidFuncType = context->functionType(sig);

        context->listType(context->stringType()); // [string]

#if 0
    addSymbols( new VariantType(c, "SocketAddress",
                                "UNIX", "string",
                                "INET", "(string,int)",
                                "INET6", "(string,int,int,int)",
                                EndArguments),
                0);
#endif

        addSymbols(

#ifndef _MSC_VER
            SYMCONST(WNOHANG), SYMCONST(WUNTRACED),
#endif

            SYMCONST(SEEK_SET), SYMCONST(SEEK_CUR), SYMCONST(SEEK_END),

            SYMCONST(SIGABRT), SYMCONST(SIGINT), SYMCONST(SIGILL),
            SYMCONST(SIGFPE), SYMCONST(SIGSEGV), SYMCONST(SIGTERM),

#ifndef _MSC_VER
            // fcntl commands
            SYMCONST(F_DUPFD), SYMCONST(F_GETFD), SYMCONST(F_SETFD),
            SYMCONST(F_GETFL), SYMCONST(F_SETFL), SYMCONST(F_GETOWN),
            SYMCONST(F_SETOWN), SYMCONST(F_GETLK), SYMCONST(F_SETLK),
            SYMCONST(F_SETLKW),

            // flock
            SYMCONST(LOCK_SH), SYMCONST(LOCK_EX), SYMCONST(LOCK_NB),
            SYMCONST(LOCK_UN),

            SYMCONST(SIGHUP), SYMCONST(SIGQUIT), SYMCONST(SIGTRAP),
            //                 SYMCONST(SIGEMT),
            SYMCONST(SIGKILL), SYMCONST(SIGBUS), SYMCONST(SIGSYS),
            SYMCONST(SIGPIPE), SYMCONST(SIGALRM), SYMCONST(SIGURG),
            SYMCONST(SIGSTOP), SYMCONST(SIGTSTP), SYMCONST(SIGCONT),
            SYMCONST(SIGCHLD), SYMCONST(SIGTTIN), SYMCONST(SIGTTOU),
            SYMCONST(SIGIO), SYMCONST(SIGXCPU), SYMCONST(SIGXFSZ),
            SYMCONST(SIGVTALRM), SYMCONST(SIGPROF), SYMCONST(SIGWINCH),
            //                 SYMCONST(SIGINFO),
            SYMCONST(SIGUSR1), SYMCONST(SIGUSR2),
#endif

            SYMCONST(O_RDONLY), SYMCONST(O_WRONLY), SYMCONST(O_RDWR),
            SYMCONST(O_APPEND), SYMCONST(O_CREAT), SYMCONST(O_TRUNC),
            SYMCONST(O_EXCL),
#ifndef WIN32
            SYMCONST(O_NONBLOCK),
#endif

        //                 SYMCONST(O_SHLOCK),
        //                 SYMCONST(O_EXLOCK),

#ifndef WIN32
            SYMCONST(AF_UNIX), SYMCONST(AF_INET), SYMCONST(AF_INET6),

            SYMCONST(SOCK_STREAM), SYMCONST(SOCK_DGRAM), SYMCONST(SOCK_RAW),
            SYMCONST(SOCK_SEQPACKET), SYMCONST(SOCK_RDM),

            // stat
            SYMCONST(S_IFMT), SYMCONST(S_IFIFO), SYMCONST(S_IFCHR),
            SYMCONST(S_IFDIR), SYMCONST(S_IFBLK), SYMCONST(S_IFREG),
            SYMCONST(S_IFLNK), SYMCONST(S_IFSOCK),
            // SYMCONST(S_IFWHT),
            SYMCONST(S_ISUID), SYMCONST(S_ISGID), SYMCONST(S_ISVTX),
            SYMCONST(S_IRWXU), SYMCONST(S_IRUSR), SYMCONST(S_IWUSR),
            SYMCONST(S_IXUSR), SYMCONST(S_IRWXG), SYMCONST(S_IRGRP),
            SYMCONST(S_IWGRP), SYMCONST(S_IXGRP), SYMCONST(S_IRWXO),
            SYMCONST(S_IROTH), SYMCONST(S_IWOTH), SYMCONST(S_IXOTH),
            // SYMCONST(S_ISTXT),
            SYMCONST(S_IREAD), SYMCONST(S_IWRITE), SYMCONST(S_IEXEC),
#endif

            EndArguments);

        addSymbols(
            new Function(c, "getenv", SystemModule::getenv, None, Args,
                         "string", Return, "string", End),

            new Function(c, "getenv", SystemModule::getenv2, None, Args,
                         "string", "string", Return, "string", End),

            new Function(c, "getenv", SystemModule::getenv3, None, Return,
                         "[string]", End),

            new Function(c, "unsetenv", SystemModule::unsetenv, None, Args,
                         "string", Return, "void", End),

            new Function(c, "putenv", SystemModule::putenv, None, Args,
                         "string", Return, "void", End),

            new Function(c, "setenv", SystemModule::setenv, None, Args,
                         "string", "string", "bool", Return, "void", End),

            new Function(c, "getcwd", SystemModule::getcwd, None, Return,
                         "string", End),

            new Function(c, "ctermid", SystemModule::ctermid, None, Return,
                         "string", End),

            new Function(c, "chdir", SystemModule::chdir, None, Args, "string",
                         Return, "void", End),

            new Function(c, "getuid", SystemModule::getuid, None, Return, "int",
                         End),

            new Function(c, "geteuid", SystemModule::geteuid, None, Return,
                         "int", End),

            new Function(c, "getgid", SystemModule::getgid, None, Return, "int",
                         End),

            new Function(c, "getegid", SystemModule::getegid, None, Return,
                         "int", End),

            new Function(c, "getpid", SystemModule::getpid, None, Return, "int",
                         End),

            new Function(c, "getppid", SystemModule::getppid, None, Return,
                         "int", End),

            new Function(c, "getpgid", SystemModule::getpgid, None, Args, "int",
                         Return, "int", End),

            new Function(c, "getpgrp", SystemModule::getpgrp, None, Return,
                         "int", End),

            new Function(c, "setuid", SystemModule::setuid, None, Args, "int",
                         Return, "void", End),

            new Function(c, "seteuid", SystemModule::seteuid, None, Args, "int",
                         Return, "void", End),

            new Function(c, "setgid", SystemModule::setgid, None, Args, "int",
                         Return, "void", End),

            new Function(c, "setegid", SystemModule::setegid, None, Args, "int",
                         Return, "void", End),

            new Function(c, "setsid", SystemModule::setsid, None, Return, "int",
                         End),

            new Function(c, "setpgid", SystemModule::setpgid, None, Args, "int",
                         "int", Return, "void", End),

            new Function(c, "setreuid", SystemModule::setreuid, None, Args,
                         "int", "int", Return, "void", End),

            new Function(c, "setregid", SystemModule::setregid, None, Args,
                         "int", "int", Return, "void", End),

            new Function(c, "umask", SystemModule::umask, None, Args, "int",
                         Return, "int", End),

            new Function(c, "chmod", SystemModule::chmod, None, Args, "string",
                         "int", Return, "void", End),

            new Function(c, "mkdir", SystemModule::mkdir, None, Args, "string",
                         "int", Return, "void", End),

            new Function(c, "mkfifo", SystemModule::mkfifo, None, Args,
                         "string", "int", Return, "void", End),

            new Function(c, "mknod", SystemModule::mknod, None, Args, "string",
                         "int", "int", Return, "void", End),

            new Function(c, "readlink", SystemModule::readlink, None, Args,
                         "string", Return, "string", End),

            new Function(c, "fork", SystemModule::fork, None, Return, "int",
                         End),

            new Function(c, "exec", SystemModule::exec, None, Return, "void",
                         Args, "string", "[string]", "[string]", End),

            new Function(c, "system", SystemModule::system, None, Return, "int",
                         Args, "string", End),

            new Function(c, "system2", SystemModule::system2, None, Return,
                         "string", Args, "string", End),

            new Function(c, "defaultWindowsOpen",
                         SystemModule::defaultWindowsOpen, None, Return, "int",
                         Args, "string", End),

            new Function(c, "wait", SystemModule::wait, None, Return,
                         "(int,int)", End),

            new Function(c, "wait", SystemModule::waitpid, None, Return,
                         "(int,int)", Args, "int", "int", End),

            new Function(c, "WIFEXITED", SystemModule::wifexited, None, Return,
                         "bool", Args, "int", End),

            new Function(c, "WIFSIGNALED", SystemModule::wifsignaled, None,
                         Return, "bool", Args, "int", End),

            new Function(c, "WIFSTOPPED", SystemModule::wifstopped, None,
                         Return, "bool", Args, "int", End),

            new Function(c, "WCOREDUMP", SystemModule::wcoredump, None, Return,
                         "int", Args, "bool", End),

            new Function(c, "WEXITSTATUS", SystemModule::wexitstatus, None,
                         Return, "int", Args, "int", End),

            new Function(c, "WTERMSIG", SystemModule::wtermsig, None, Return,
                         "int", Args, "int", End),

            new Function(c, "WSTOPSIG", SystemModule::wstopsig, None, Return,
                         "int", Args, "int", End),

            new Function(c, "_exit", SystemModule::_exit, None, Return, "void",
                         Args, "int", End),

            new Function(c, "kill", SystemModule::kill, None, Return, "void",
                         Args, "int", "int", End),

            new Function(c, "getdtablesize", SystemModule::getdtablesize, None,
                         Return, "int", End),

            new Function(c, "flock", SystemModule::flock, None, Return, "int",
                         Args, "int", "int", End),

            new Function(c, "fcntl", SystemModule::fcntl, None, Return, "int",
                         Args, "int", "int", "int", End),

            new Function(c, "pipe", SystemModule::pipe, None, Return,
                         "(int,int)", End),

            new Function(c, "open", SystemModule::open, None, Return, "int",
                         Args, "string", "int", "int", End),

            new Function(c, "close", SystemModule::close, None, Return, "void",
                         Args, "int", End),

            new Function(c, "dup", SystemModule::dup, None, Return, "int", Args,
                         "int", End),

            new Function(c, "dup2", SystemModule::dup2, None, Return, "int",
                         Args, "int", "int", End),

            new Function(c, "read", SystemModule::read, None, Return, "int",
                         Args, "int", "byte[]", End),

            new Function(c, "write", SystemModule::write, None, Return, "int",
                         Args, "int", "byte[]", End),

            new Function(c, "lseek", SystemModule::lseek, None, Return, "int",
                         Args, "int", "int", "int", End),

            new Function(c, "link", SystemModule::link, None, Return, "void",
                         Args, "string", "string", End),

            new Function(c, "unlink", SystemModule::link, None, Return, "void",
                         Args, "string", End),

            new Function(c, "ioctl", SystemModule::ioctl, None, Return, "void",
                         Args, "int", "int64", "?", End),

            new Function(c, "stat", SystemModule::stat, None, Return,
                         "system.statinfo", Args, "string", End),

            new Function(c, "S_ISBLK", SystemModule::s_isblk, Mapped, Return,
                         "bool", Args, "short", End),

            new Function(c, "S_ISCHR", SystemModule::s_ischr, Mapped, Return,
                         "bool", Args, "short", End),

            new Function(c, "S_ISDIR", SystemModule::s_isdir, Mapped, Return,
                         "bool", Args, "short", End),

            new Function(c, "S_ISFIFO", SystemModule::s_isfifo, Mapped, Return,
                         "bool", Args, "short", End),

            new Function(c, "S_ISREG", SystemModule::s_isreg, Mapped, Return,
                         "bool", Args, "short", End),

            new Function(c, "S_ISLNK", SystemModule::s_islnk, Mapped, Return,
                         "bool", Args, "short", End),

            new Function(c, "S_ISSOCK", SystemModule::s_issock, Mapped, Return,
                         "bool", Args, "short", End),

            new Function(c, "socket", SystemModule::socket, None, Return, "int",
                         Args, "int", "int", "int", End),

            new Function(c, "time", SystemModule::time, None, Return, "int64",
                         End),

            new Function(c, "gettimeofday", SystemModule::gettimeofday, None,
                         Return, "(int64,int)", End),

            EndArguments);

        //
        //  Additional standard C functions. FILE* is made an OpaqueType
        //  so it can be passed around directly (but no operations can be
        //  performed on it).
        //

        addSymbol(new OpaqueType(c, "FILE"));

        addSymbols(new Function(c, "fopen", SystemModule::fopen, None, Return,
                                "system.FILE", Args, "string", "string", End),

                   new Function(c, "fdopen", SystemModule::fdopen, None, Return,
                                "system.FILE", Args, "int", "string", End),

                   new Function(c, "freopen", SystemModule::freopen, None,
                                Return, "system.FILE", Args, "string", "string",
                                "system.FILE", End),

                   new Function(c, "fclose", SystemModule::fclose, None, Return,
                                "void", Args, "system.FILE", End),

                   new Function(c, "clearerr", SystemModule::clearerr, None,
                                Return, "void", Args, "system.FILE", End),

                   new Function(c, "feof", SystemModule::feof, None, Return,
                                "bool", Args, "system.FILE", End),

                   new Function(c, "ferror", SystemModule::ferror, None, Return,
                                "bool", Args, "system.FILE", End),

                   new Function(c, "fileno", SystemModule::fileno, None, Return,
                                "int", Args, "system.FILE", End),

#if 0
                new Function(c, "fread", SystemModule::fread, None,
                             Return, "int64",
                             Args, "?dyn_array", "int64", "int64", "FILE",
                             End),

                new Function(c, "fwrite", SystemModule::fwrite, None,
                             Return, "int64",
                             Args, "?dyn_array", "int64", "int64", "FILE",
                             End),

                new Function(c, "fgetc", SystemModule::fgetc, None,
                             Return, "byte",
                             Args, "FILE",
                             End),

                new Function(c, "fputc", SystemModule::fputc, None,
                             Return, "byte",
                             Args, "char", "FILE",
                             End),
#endif

                   EndArguments);
    }

    static void ThrowErrno(Thread& thread, int errnum)
    {
        if (errnum == 0)
            errnum = errno;

        Process* p = thread.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        StringType::String* s = c->stringType()->allocate(strerror(errnum));

        switch (errnum)
        {
        case ENOMEM:
            throw OutOfSystemMemoryException(thread, s);
        case EACCES:
            throw PermissionDeniedException(thread, s);
        case ENOENT:
            throw PathnameNonexistantException(thread, s);
        default:
            throw GenericPosixException(thread, s);
        }
    }

    NODE_IMPLEMENTATION(SystemModule::getenv, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* key = NODE_ARG_OBJECT(0, StringType::String);
        const StringType* stype = static_cast<const StringType*>(key->type());

        if (const char* env = ::getenv(key->c_str()))
        {
            NODE_RETURN(c->stringType()->allocate(env));
        }
        else
        {
            ThrowErrno(NODE_THREAD, 0);
            NODE_RETURN(0);
        }
    }

    NODE_IMPLEMENTATION(SystemModule::getenv2, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* key = NODE_ARG_OBJECT(0, StringType::String);
        const StringType* stype = static_cast<const StringType*>(key->type());

        if (const char* env = ::getenv(key->c_str()))
        {
            NODE_RETURN(c->stringType()->allocate(env));
        }
        else
        {
            NODE_RETURN(NODE_ARG_OBJECT(1, StringType::String));
        }
    }

    NODE_IMPLEMENTATION(SystemModule::getenv3, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const ListType* ltype = static_cast<const ListType*>(NODE_THIS.type());
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(ltype->elementType());
        List list(p, ltype);

        for (char** e = environ; *e; e++)
        {
            list.append(stype->allocate(*e));
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(SystemModule::unsetenv, void)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* key = NODE_ARG_OBJECT(0, StringType::String);
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

#ifndef WIN32
        ::unsetenv(key->c_str());
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::putenv, void)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* keyval =
            NODE_ARG_OBJECT(0, StringType::String);

#ifndef _MSC_VER
        if (::putenv((char*)keyval->c_str()))
#endif
        {
            ThrowErrno(NODE_THREAD, 0);
        }
    }

    NODE_IMPLEMENTATION(SystemModule::setenv, void)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* val = NODE_ARG_OBJECT(1, StringType::String);
        bool overwrite = NODE_ARG(2, bool);

#ifndef WIN32
        if (::setenv(name->c_str(), val->c_str(), overwrite ? 1 : 0))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::getcwd, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const StringType* stype =
            static_cast<const StringType*>(NODE_THIS.type());

        char temp[1024];

        // AJG - Can't universally replace getcwd in this file
#ifdef _MSC_VER
#define getcwd _getcwd
#endif

        if (const char* env = ::getcwd(temp, 1024))
        {
            NODE_RETURN(stype->allocate(env));
        }
        else
        {
            ThrowErrno(NODE_THREAD, 0);
            NODE_RETURN(0);
        }

#ifdef _MSC_VER
#undef getcwd
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::ctermid, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const StringType* stype =
            static_cast<const StringType*>(NODE_THIS.type());

#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        if (const char* env = ::ctermid(0))
        {
            NODE_RETURN(stype->allocate(env));
        }
        else
        {
            ThrowErrno(NODE_THREAD, 0);
            NODE_RETURN(0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::chdir, void)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* dir = NODE_ARG_OBJECT(0, StringType::String);

        if (::chdir(dir->c_str()))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
    }

    NODE_IMPLEMENTATION(SystemModule::getuid, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        NODE_RETURN(int(::getuid()));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::geteuid, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        NODE_RETURN(int(::geteuid()));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::getgid, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        NODE_RETURN(int(::getgid()));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::getegid, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        NODE_RETURN(int(::getegid()));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::getpid, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        NODE_RETURN(int(::getpid()));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::getppid, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        NODE_RETURN(int(::getppid()));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::getpgid, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
#if (__GNUC__ == 2) && (__GNUC_MINOR__ == 95)
        ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(0);
#else
        NODE_RETURN(int(::getpgid(NODE_ARG(0, int))));
#endif
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::getpgrp, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        NODE_RETURN(int(::getpgrp()));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::setuid, void)
    {
#ifndef _MSC_VER
        if (::setuid(NODE_ARG(0, int)))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::seteuid, void)
    {
#ifndef _MSC_VER
        if (::seteuid(NODE_ARG(0, int)))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::setgid, void)
    {
#ifndef _MSC_VER
        if (::setgid(NODE_ARG(0, int)))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::setegid, void)
    {
#ifndef _MSC_VER
        if (::setegid(NODE_ARG(0, int)))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::setsid, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int pid = ::setsid();

        if (pid == -1)
        {
            ThrowErrno(NODE_THREAD, 0);
        }

        NODE_RETURN(pid);
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::setpgid, void)
    {
#ifndef _MSC_VER
        if (::setpgid(NODE_ARG(0, int), NODE_ARG(1, int)))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

#if 0
NODE_IMPLEMENTATION(SystemModule::setpgrp, void)
{
    if (::setpgrp(NODE_ARG(0,int), NODE_ARG(1,int)))
    {
        ThrowErrno(NODE_THREAD, 0);
    }
}
#endif

    NODE_IMPLEMENTATION(SystemModule::setreuid, void)
    {
#ifndef _MSC_VER
        if (::setreuid(NODE_ARG(0, int), NODE_ARG(1, int)))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::setregid, void)
    {
#ifndef _MSC_VER
        if (::setregid(NODE_ARG(0, int), NODE_ARG(1, int)))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

#if 0
NODE_IMPLEMENTATION(SystemModule::issetugid, bool)
{
    NODE_RETURN(bool(::issetugid()));
}
#endif

    NODE_IMPLEMENTATION(SystemModule::umask, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        NODE_RETURN(::umask(NODE_ARG(0, int)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::chmod, void)
    {
        const StringType::String* dir = NODE_ARG_OBJECT(0, StringType::String);
        int mode = NODE_ARG(1, int);

#ifndef _MSC_VER
        if (::chmod(dir->c_str(), mode))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::mkdir, void)
    {
        const StringType::String* dir = NODE_ARG_OBJECT(0, StringType::String);
        int mode = NODE_ARG(1, int);

#ifdef _MSC_VER
        //
        //  Ignore mode for windows
        //

        SECURITY_ATTRIBUTES attrs;
        attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
        attrs.lpSecurityDescriptor = NULL;
        attrs.bInheritHandle = 1;

        if (!CreateDirectory(dir->c_str(), &attrs))
        {
            ThrowErrno(NODE_THREAD, 0);
        }

#else
        if (::mkdir(dir->c_str(), mode))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::mkfifo, void)
    {
        const StringType::String* dir = NODE_ARG_OBJECT(0, StringType::String);
        int mode = NODE_ARG(1, int);

#ifndef _MSC_VER
        if (::mkfifo(dir->c_str(), mode))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::mknod, void)
    {
        const StringType::String* dir = NODE_ARG_OBJECT(0, StringType::String);
        int mode = NODE_ARG(1, int);
        int dev = NODE_ARG(2, int);

#ifndef _MSC_VER
        if (::mknod(dir->c_str(), mode, dev))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::readlink, Pointer)
    {
        const StringType::String* link = NODE_ARG_OBJECT(0, StringType::String);
        char buf[512];
        memset(buf, 0, 512 * sizeof(char));

#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        if (::readlink(link->c_str(), buf, 512) < 0)
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif

        Process* p = NODE_THREAD.process();
        const StringType* stype =
            static_cast<const StringType*>(NODE_THIS.type());

        NODE_RETURN(stype->allocate(buf));
    }

    NODE_IMPLEMENTATION(SystemModule::fork, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        pid_t p = ::fork();
        if (p == -1)
            ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(int(p));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::exec, void)
    {
        Process* p = NODE_THREAD.process();
        Context* c = p->context();
        const StringType::String* path = NODE_ARG_OBJECT(0, StringType::String);
        vector<const char*> argv;
        vector<const char*> envv;

        for (List args(p, NODE_ARG_OBJECT(1, ClassInstance)); args.isNotNil();
             args++)
        {
            const StringType::String* s = args.value<StringType::String*>();
            argv.push_back(s->c_str());
        }

        for (List envp(p, NODE_ARG_OBJECT(2, ClassInstance)); envp.isNotNil();
             envp++)
        {
            const StringType::String* s = envp.value<StringType::String*>();
            envv.push_back(s->c_str());
        }

        argv.push_back(0);
        envv.push_back(0);

#ifndef _MSC_VER
        ::execve(path->c_str(), (char**)&argv.front(), (char**)&envv.front());
#endif

        ThrowErrno(NODE_THREAD, 0);
    }

    NODE_IMPLEMENTATION(SystemModule::defaultWindowsOpen, int)
    {
        const StringType::String* url = NODE_ARG_OBJECT(0, StringType::String);
        int status = 0;

#ifdef PLATFORM_WINDOWS
        HINSTANCE st =
            ShellExecute(NULL, "open", url->c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif

        NODE_RETURN(status);
    }

    NODE_IMPLEMENTATION(SystemModule::system, int)
    {
        const StringType::String* cmd = NODE_ARG_OBJECT(0, StringType::String);
        Process* p = NODE_THREAD.process();
        Context* c = p->context();

        int status = ::system(cmd->c_str());

        if (status == -1)
        {
            ThrowErrno(NODE_THREAD, 0);
        }

        NODE_RETURN(status);
    }

    NODE_IMPLEMENTATION(SystemModule::wait, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const Class* rtype = static_cast<const Class*>(NODE_THIS.type());
        ClassInstance* obj = ClassInstance::allocate(rtype);
        int* ip = (int*)obj->structure();

#ifndef _MSC_VER
        int status;
        pid_t pid = ::wait(&status);
        ip[0] = pid;
        ip[1] = status;
#endif

        NODE_RETURN(obj);
    }

    NODE_IMPLEMENTATION(SystemModule::waitpid, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const Class* rtype = static_cast<const Class*>(NODE_THIS.type());
        ClassInstance* obj = ClassInstance::allocate(rtype);
        int* ip = (int*)obj->structure();

        int wpid = NODE_ARG(0, int);
        int options = NODE_ARG(1, int);

#ifndef _MSC_VER
        int status;
        pid_t pid = ::waitpid(wpid, &status, options);
        ip[0] = pid;
        ip[1] = status;
#endif

        NODE_RETURN(obj);
    }

    NODE_IMPLEMENTATION(SystemModule::wifexited, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        int status = NODE_ARG(0, int);
        NODE_RETURN(bool(WIFEXITED(status)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::wifsignaled, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        int status = NODE_ARG(0, int);
        NODE_RETURN(bool(WIFSIGNALED(status)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::wifstopped, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        int status = NODE_ARG(0, int);
        NODE_RETURN(bool(WIFSTOPPED(status)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::wcoredump, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        int status = NODE_ARG(0, int);
        NODE_RETURN(bool(WCOREDUMP(status)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::wexitstatus, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        int status = NODE_ARG(0, int);
        NODE_RETURN(int(WEXITSTATUS(status)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::wtermsig, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        int status = NODE_ARG(0, int);
        NODE_RETURN(int(WTERMSIG(status)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::wstopsig, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        int status = NODE_ARG(0, int);
        NODE_RETURN(int(WSTOPSIG(status)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::_exit, void)
    {
        //
        //  NOTE: POSIX only defines _exit()! There is an exit() function
        //  in the runtime module, but this is the C99 exit function which
        //  calls the atexit() callbacks.
        //

        ::_exit(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(SystemModule::kill, void)
    {
#ifndef _MSC_VER
        if (::kill(NODE_ARG(0, int), NODE_ARG(1, int)))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::getdtablesize, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        NODE_RETURN(::getdtablesize());
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::flock, int)
    {
        int fd = NODE_ARG(0, int);
        int op = NODE_ARG(1, int);

#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int r = ::flock(fd, op);
        if (r < 0)
            ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(r);
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::fcntl, int)
    {
        int fd = NODE_ARG(0, int);
        int cmd = NODE_ARG(1, int);
        int arg = NODE_ARG(2, int);

#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int r = ::fcntl(fd, cmd, arg);

        switch (cmd)
        {
        case F_DUPFD:
        case F_GETFD:
        case F_GETFL:
        case F_GETOWN:
            NODE_RETURN(r);
        default:
            if (r == -1)
                ThrowErrno(NODE_THREAD, 0);
            NODE_RETURN(r);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::pipe, Pointer)
    {
        ClassInstance* obj = 0;
        int fd[2];

#ifdef _MSC_VER
        NODE_RETURN(obj);
#else
        if (::pipe(fd))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
        else
        {
            Process* p = NODE_THREAD.process();
            MuLangContext* c = static_cast<MuLangContext*>(p->context());
            const Class* rtype = static_cast<const Class*>(NODE_THIS.type());
            obj = ClassInstance::allocate(rtype);
            int* ip = (int*)obj->structure();
            ip[0] = fd[0];
            ip[1] = fd[1];
        }

        NODE_RETURN(obj);
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::open, int)
    {
        const StringType::String* path = NODE_ARG_OBJECT(0, StringType::String);

#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int fd = ::open(path->c_str(), NODE_ARG(1, int), NODE_ARG(2, int));

        if (fd < 0)
            ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(fd);
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::system2, Pointer)
    {
        const StringType::String* path = NODE_ARG_OBJECT(0, StringType::String);

#ifdef _MSC_VER
        StringType::String* o = NULL;
        NODE_RETURN(o);
#else
        FILE* fd = ::popen(path->c_str(), "r");
        if (!fd)
            ThrowErrno(NODE_THREAD, 0);

        Process* p = NODE_THREAD.process();

        const StringType* stype =
            static_cast<const StringType*>(NODE_THIS.type());

        int ch = fgetc(fd);
        ostringstream ostr;

        while (ch != EOF)
        {
            ostr << char(ch);
            ch = fgetc(fd);
        }

        pclose(fd);

        NODE_RETURN(stype->allocate(ostr));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::close, void)
    {
#ifndef _MSC_VER
        if (::close(NODE_ARG(0, int)))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::dup, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int fd = ::dup(NODE_ARG(0, int));
        if (fd < 0)
            ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(fd);
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::dup2, int)
    {
#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int fd = ::dup2(NODE_ARG(0, int), NODE_ARG(1, int));
        if (fd < 0)
            ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(fd);
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::read, int)
    {
        int fd = NODE_ARG(0, int);
        DynamicArray* array = NODE_ARG_OBJECT(1, DynamicArray);

#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int r = ::read(fd, array->data<void>(), array->size());
        if (r == -1)
            ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(r);
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::write, int)
    {
        int fd = NODE_ARG(0, int);
        DynamicArray* array = NODE_ARG_OBJECT(1, DynamicArray);

#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int r = ::write(fd, array->data<void>(), array->size());
        if (r == -1)
            ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(r);
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::lseek, int)
    {
        int fd = NODE_ARG(0, int);
        int off = NODE_ARG(1, int);
        int whence = NODE_ARG(2, int);

#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int r = ::lseek(fd, off, whence);
        if (r == -1)
            ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(r);
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::link, void)
    {
        const StringType::String* name1 =
            NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* name2 =
            NODE_ARG_OBJECT(1, StringType::String);

#ifndef _MSC_VER
        if (::link(name1->c_str(), name2->c_str()))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::unlink, void)
    {
        const StringType::String* name1 =
            NODE_ARG_OBJECT(0, StringType::String);

        if (::unlink(name1->c_str()))
        {
            ThrowErrno(NODE_THREAD, 0);
        }
    }

    NODE_IMPLEMENTATION(SystemModule::ioctl, void)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        int fd = NODE_ARG(0, int);
        unsigned long r = (unsigned long)(NODE_ARG(1, int64));
        int rval = 0;
        const Type* rtype = NODE_THIS.type();

#ifndef _MSC_VER
        if (rtype == c->intType())
        {
            if (::ioctl(fd, r, NODE_ARG(2, int)))
            {
                ThrowErrno(NODE_THREAD, 0);
            }
        }
        else if (dynamic_cast<const Class*>(rtype))
        {
            ClassInstance* o = NODE_ARG_OBJECT(2, ClassInstance);

            if (::ioctl(fd, r, o->data<char>()))
            {
                ThrowErrno(NODE_THREAD, 0);
            }
        }
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::stat, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const Class* rtype = static_cast<const Class*>(NODE_THIS.type());
        const Class* ttype = static_cast<const Class*>(rtype->fieldType(7));
        struct stat s;

        const StringType::String* arg0 =
            NODE_ARG_OBJECT(0, const StringType::String);
        const char* filename = arg0->c_str();

        if (::stat(filename, &s))
        {
            ThrowErrno(NODE_THREAD, 0);
        }

        ClassInstance* o = ClassInstance::allocate(rtype);
        StatStruct* ss = o->data<StatStruct>();
        ss->atime = ClassInstance::allocate(ttype);
        ss->mtime = ClassInstance::allocate(ttype);
        ss->ctime = ClassInstance::allocate(ttype);
        TimeSpec* atime = ss->atime->data<TimeSpec>();
        TimeSpec* mtime = ss->mtime->data<TimeSpec>();
        TimeSpec* ctime = ss->ctime->data<TimeSpec>();

        ss->dev = s.st_dev;
        ss->ino = s.st_ino;
        ss->mode = s.st_mode;
        ss->nlink = s.st_nlink;
        ss->uid = s.st_uid;
        ss->gid = s.st_gid;
        ss->size = s.st_size;

#ifndef _MSC_VER

#ifdef PLATFORM_DARWIN
        atime->tv_sec = s.st_atimespec.tv_sec;
        atime->tv_nsec = s.st_atimespec.tv_nsec;
        mtime->tv_sec = s.st_mtimespec.tv_sec;
        mtime->tv_nsec = s.st_mtimespec.tv_nsec;
        ctime->tv_sec = s.st_ctimespec.tv_sec;
        ctime->tv_nsec = s.st_ctimespec.tv_nsec;
#else
        atime->tv_sec = s.st_atim.tv_sec;
        atime->tv_nsec = s.st_atim.tv_nsec;
        mtime->tv_sec = s.st_mtim.tv_sec;
        mtime->tv_nsec = s.st_mtim.tv_nsec;
        ctime->tv_sec = s.st_ctim.tv_sec;
        ctime->tv_nsec = s.st_ctim.tv_nsec;
#endif

#endif

        NODE_RETURN(o);
    }

    NODE_IMPLEMENTATION(SystemModule::s_isblk, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        NODE_RETURN(S_ISBLK(NODE_ARG(0, short)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::s_ischr, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        NODE_RETURN(S_ISCHR(NODE_ARG(0, short)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::s_isdir, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        NODE_RETURN(S_ISDIR(NODE_ARG(0, short)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::s_isfifo, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        NODE_RETURN(S_ISFIFO(NODE_ARG(0, short)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::s_isreg, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        NODE_RETURN(S_ISREG(NODE_ARG(0, short)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::s_islnk, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        NODE_RETURN(S_ISLNK(NODE_ARG(0, short)));
#endif
    }

    NODE_IMPLEMENTATION(SystemModule::s_issock, bool)
    {
#ifdef _MSC_VER
        NODE_RETURN(bool(false));
#else
        NODE_RETURN(S_ISSOCK(NODE_ARG(0, short)));
#endif
    }

    //
    //  BSD socket functions
    //

    NODE_IMPLEMENTATION(SystemModule::socket, int)
    {
        int domain = NODE_ARG(0, int);
        int type = NODE_ARG(1, int);
        int protocol = NODE_ARG(2, int);

#ifdef _MSC_VER
        NODE_RETURN(0);
#else
        int r = ::socket(domain, type, protocol);
        if (r == -1)
            ThrowErrno(NODE_THREAD, 0);
        NODE_RETURN(r);
#endif
    }

    //
    //  C standard I/O
    //

    NODE_IMPLEMENTATION(SystemModule::fopen, Pointer)
    {
        const StringType::String* arg0 =
            NODE_ARG_OBJECT(0, const StringType::String);
        const StringType::String* arg1 =
            NODE_ARG_OBJECT(1, const StringType::String);
        const char* filename = arg0->c_str();
        const char* mode = arg1->c_str();

        FILE* file = ::fopen(filename, mode);
        if (!file)
            ThrowErrno(NODE_THREAD, 0);

        NODE_RETURN(file);
    }

    NODE_IMPLEMENTATION(SystemModule::freopen, Pointer)
    {
        const StringType::String* arg0 =
            NODE_ARG_OBJECT(0, const StringType::String);
        const StringType::String* arg1 =
            NODE_ARG_OBJECT(1, const StringType::String);
        const char* filename = arg0->c_str();
        const char* mode = arg1->c_str();
        FILE* f = NODE_ARG_OBJECT(2, FILE);

        FILE* file = ::freopen(filename, mode, f);
        if (!file)
            ThrowErrno(NODE_THREAD, 0);

        NODE_RETURN(file);
    }

    NODE_IMPLEMENTATION(SystemModule::fdopen, Pointer)
    {
        int fd = NODE_ARG(0, int);
        const StringType::String* arg1 =
            NODE_ARG_OBJECT(0, const StringType::String);

        FILE* file = ::fdopen(fd, arg1->c_str());
        if (!file)
            ThrowErrno(NODE_THREAD, 0);

        NODE_RETURN(file);
    }

    NODE_IMPLEMENTATION(SystemModule::fclose, void)
    {
        FILE* file = NODE_ARG_OBJECT(0, FILE);
        int rval = ::fclose(file);
        if (rval != 0)
            ThrowErrno(NODE_THREAD, 0);
    }

    NODE_IMPLEMENTATION(SystemModule::clearerr, void)
    {
        FILE* file = NODE_ARG_OBJECT(0, FILE);
        ::clearerr(file);
    }

    NODE_IMPLEMENTATION(SystemModule::feof, bool)
    {
        FILE* file = NODE_ARG_OBJECT(0, FILE);
        NODE_RETURN(::feof(file) != 0);
    }

    NODE_IMPLEMENTATION(SystemModule::ferror, bool)
    {
        FILE* file = NODE_ARG_OBJECT(0, FILE);
        NODE_RETURN(::ferror(file) != 0);
    }

    NODE_IMPLEMENTATION(SystemModule::fileno, int)
    {
        FILE* file = NODE_ARG_OBJECT(0, FILE);
        NODE_RETURN(::fileno(file));
    }

    NODE_IMPLEMENTATION(SystemModule::time, int64) { NODE_RETURN(::time(0)); }

    struct TimeTuple
    {
        int64 seconds;
        int microseconds;
    };

    NODE_IMPLEMENTATION(SystemModule::gettimeofday, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const Class* type = static_cast<const Class*>(NODE_THIS.type());

        ClassInstance* o = ClassInstance::allocate(type);
        TimeTuple* tt = o->data<TimeTuple>();

        struct timeval tv;
        struct timezone tz;
        ::gettimeofday(&tv, &tz);

        tt->seconds = tv.tv_sec;
        tt->microseconds = tv.tv_usec;

        NODE_RETURN(o);
    }

} // namespace Mu
