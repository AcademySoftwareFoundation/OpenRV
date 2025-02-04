//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuIO/IFStreamType.h>
#include <MuIO/IOModule.h>
#include <MuIO/ISStreamType.h>
#include <MuIO/IStreamType.h>
#include <MuIO/OFStreamType.h>
#include <MuIO/OSStreamType.h>
#include <MuIO/OStreamType.h>
#include <MuIO/StreamType.h>
#include <MuIO/ProcessType.h>
#include <Mu/Archive.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/List.h>
#include <Mu/ListType.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/TupleType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>

/* AJG - sys/xxx.h stuff */
#include <dirent.h>

#if _MSC_VER
#include <windows.h>
#include <wchar.h>
#else
#include <sys/dir.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace Mu
{
    using namespace std;

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

    IOModule::IOModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    IOModule::~IOModule() {}

    void IOModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;
        Module* path = new Module(c, "path");

        Class* streamType = new StreamType(c, "stream");
        Class* ostreamType = new OStreamType(c, "ostream", streamType);
        Class* ofstreamType = new OFStreamType(c, "ofstream", ostreamType);
        Class* istreamType = new IStreamType(c, "istream", streamType);
        Class* ifstreamType = new IFStreamType(c, "ifstream", istreamType);
        Class* processType = new ProcessType(c, "process", 0);

#ifndef TWK_STUB_IT_OUT
        Class* osstreamType = new OSStreamType(c, "osstream", ostreamType);
        Class* isstreamType = new ISStreamType(c, "isstream", istreamType);
#endif

        const char* os = "io.ostream";
        const char* is = "io.istream";

        //
        //  directory entry array types
        //

        STLVector<const Type*>::Type types(2);
        types[0] = context->stringType();
        types[1] = context->intType();
        context->listType(context->tupleType(types));

        //

        addSymbols(
            streamType, ostreamType, ofstreamType, istreamType, ifstreamType,
            processType,

#ifndef TWK_STUB_IT_OUT
            isstreamType, osstreamType,
#endif

        /* AJG - Linux dirent64 d_type's - not needed in win32 'cuz there is no
         * d_type */
#if !defined _MSC_VER
            new SymbolicConstant(c, "UnknownFileType", "int",
                                 Value(DT_UNKNOWN)),
            new SymbolicConstant(c, "RegularFileType", "int", Value(DT_REG)),
            new SymbolicConstant(c, "DirFileType", "int", Value(DT_DIR)),
            new SymbolicConstant(c, "SymbolicLinkFileType", "int",
                                 Value(DT_LNK)),
            new SymbolicConstant(c, "FIFOFileType", "int", Value(DT_FIFO)),
            new SymbolicConstant(c, "CharDeviceFileType", "int", Value(DT_CHR)),
            new SymbolicConstant(c, "BlockDeviceFileType", "int",
                                 Value(DT_BLK)),
            new SymbolicConstant(c, "SocketFileType", "int", Value(DT_SOCK)),
#endif
            new Function(c, "print", IOModule::printString, None, Return, os,
                         Args, os, "string", End),

            new Function(c, "print", IOModule::print_int, None, Return, os,
                         Args, os, "int", End),

            new Function(c, "print", IOModule::print_float, None, Return, os,
                         Args, os, "float", End),

            new Function(c, "print", IOModule::print_double, None, Return, os,
                         Args, os, "double", End),

            new Function(c, "print", IOModule::print_bool, None, Return, os,
                         Args, os, "bool", End),

            new Function(c, "print", IOModule::print_byte, None, Return, os,
                         Args, os, "byte", End),

            new Function(c, "print", IOModule::print_char, None, Return, os,
                         Args, os, "char", End),

            new Function(c, "endl", IOModule::print_endl, None, Return, os,
                         Args, os, End),

            new Function(c, "flush", IOModule::print_flush, None, Return, os,
                         Args, os, End),

            new Function(c, "read_string", IOModule::readString, None, Return,
                         "string", Parameters, new Param(c, "is", is),
                         new Param(c, "count", "int", Value(int(1))), End),

            new Function(c, "read_int", IOModule::read_int, None, Return, "int",
                         Args, is, End),

            new Function(c, "read_float", IOModule::read_float, None, Return,
                         "float", Args, is, End),

            new Function(c, "read_double", IOModule::read_double, None, Return,
                         "double", Args, is, End),

            new Function(c, "read_bool", IOModule::read_bool, None, Return,
                         "bool", Args, is, End),

            new Function(c, "read_byte", IOModule::read_byte, None, Return,
                         "byte", Args, is, End),

            new Function(c, "read_char", IOModule::read_char, None, Return,
                         "char", Args, is, End),

            new Function(c, "read_line", IOModule::read_line, None, Return,
                         "string", Args, is, "char", End),

            new Function(c, "read_all", IOModule::read_all, None, Return,
                         "string", Args, is, End),

            new Function(c, "read_all_bytes", IOModule::read_all_bytes, None,
                         Return, "byte[]", Args, is, End),

            new Function(c, "serialize", IOModule::serialize, None, Return,
                         "int", Args, os, "object", End),

            new Function(c, "deserialize", IOModule::deserialize, None, Return,
                         "object", Args, is, End),

            new Function(c, "out", IOModule::out, None, Return, os, End),

            new Function(c, "err", IOModule::err, None, Return, os, End),

            new Function(c, "in", IOModule::in, None, Return, is, End),

            new Function(c, "directory", IOModule::directory, None, Return,
                         "[(string,int)]", Args, "string", End),

            EndArguments);

        //
        //  Path module
        //

        path->addSymbols(
            new Function(c, "basename", IOModule::basename, Mapped, Return,
                         "string", Args, "string", End),

            new Function(c, "dirname", IOModule::dirname, Mapped, Return,
                         "string", Args, "string", End),

            new Function(c, "extension", IOModule::extension, Mapped, Return,
                         "string", Args, "string", End),

            new Function(c, "without_extension", IOModule::without_extension,
                         Mapped, Return, "string", Args, "string", End),

            new Function(c, "exists", IOModule::exists, None, Return, "bool",
                         Args, "string", End),

            new Function(c, "expand", IOModule::expand, None, Return, "string",
                         Args, "string", End),

            new Function(c, "join", IOModule::join, None, Return, "string",
                         Args, "string", "string", End),

            new Function(c, "concat_paths", IOModule::concat_paths, None,
                         Return, "string", Args, "string", "string", End),

            new Function(c, "path_separator", IOModule::path_separator, None,
                         Return, "string", End),

            new Function(c, "concat_separator", IOModule::concat_separator,
                         None, Return, "string", End),

            EndArguments);

        addSymbol(path);
    }

    NODE_IMPLEMENTATION(IOModule::printString, Pointer)
    {
        OStreamType::OStream* stream = NODE_ARG_OBJECT(0, OStreamType::OStream);
        const StringType::String* s = NODE_ARG_OBJECT(1, StringType::String);
        *stream->_ostream << s->c_str();
        NODE_RETURN(stream);
    }

#define PRINT_MANIP(MANIP)                                \
    NODE_IMPLEMENTATION(IOModule::print_##MANIP, Pointer) \
    {                                                     \
        OStreamType::OStream* stream =                    \
            NODE_ARG_OBJECT(0, OStreamType::OStream);     \
        *stream->_ostream << MANIP;                       \
        NODE_RETURN(stream);                              \
    }

    PRINT_MANIP(endl)
    PRINT_MANIP(flush)

#define PRINT_TYPE(TYPE)                                 \
    NODE_IMPLEMENTATION(IOModule::print_##TYPE, Pointer) \
    {                                                    \
        OStreamType::OStream* stream =                   \
            NODE_ARG_OBJECT(0, OStreamType::OStream);    \
        TYPE i = NODE_ARG(1, TYPE);                      \
        *stream->_ostream << i;                          \
        NODE_RETURN(stream);                             \
    }

    PRINT_TYPE(int)
    PRINT_TYPE(float)
    PRINT_TYPE(double)
    PRINT_TYPE(bool)
    PRINT_TYPE(char)

    NODE_IMPLEMENTATION(IOModule::print_byte, Pointer)
    {
        OStreamType::OStream* stream = NODE_ARG_OBJECT(0, OStreamType::OStream);
        char i = NODE_ARG(1, char);
        *stream->_ostream << int(i);
        NODE_RETURN(stream);
    }

    NODE_IMPLEMENTATION(IOModule::readString, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const StringType* stype =
            static_cast<const StringType*>(NODE_THIS.type());

        IStreamType::IStream* stream = NODE_ARG_OBJECT(0, IStreamType::IStream);
        int count = NODE_ARG(1, int) - 1;

        for (int i = 0; i < count; i++)
        {
            string s;
            *stream->_istream >> s;
        }

        string sn;
        *stream->_istream >> sn;
        NODE_RETURN(stype->allocate(sn));
    }

    NODE_IMPLEMENTATION(IOModule::read_line, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const StringType* stype =
            static_cast<const StringType*>(NODE_THIS.type());

        IStreamType::IStream* stream = NODE_ARG_OBJECT(0, IStreamType::IStream);
        char delim = NODE_ARG(1, char);
        ostringstream ss;

        while (stream->_istream->good())
        {
            char c = stream->_istream->get();
            if (c == delim)
                break;
            ss << c;
        }

        NODE_RETURN(stype->allocate(ss));
    }

    NODE_IMPLEMENTATION(IOModule::read_all, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const StringType* stype =
            static_cast<const StringType*>(NODE_THIS.type());
        IStreamType::IStream* stream = NODE_ARG_OBJECT(0, IStreamType::IStream);
        ostringstream ss;

        while (stream->_istream->good())
        {
            int c = stream->_istream->get();
            if (c != -1)
                ss << char(c);
        }

        NODE_RETURN(stype->allocate(ss));
    }

    NODE_IMPLEMENTATION(IOModule::read_all_bytes, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Class* stype = static_cast<const StringType*>(NODE_THIS.type());

        IStreamType::IStream* stream = NODE_ARG_OBJECT(0, IStreamType::IStream);
#define MYBUFSIZE 4096

        DynamicArrayType* type = (DynamicArrayType*)NODE_THIS.type();
        DynamicArray* array = new DynamicArray(type, 1);

        size_t offset = 0;

        do
        {
            size_t sz = array->size();
            offset = sz;
            array->resize(sz + MYBUFSIZE);
            stream->_istream->read(array->data<char>() + offset, MYBUFSIZE);
            array->resize(sz + stream->_istream->gcount());
        } while (stream->_istream->good());

        NODE_RETURN(array);
    }

#define READ_TYPE(TYPE)                               \
    NODE_IMPLEMENTATION(IOModule::read_##TYPE, TYPE)  \
    {                                                 \
        IStreamType::IStream* stream =                \
            NODE_ARG_OBJECT(0, IStreamType::IStream); \
        TYPE tp;                                      \
        *stream->_istream >> tp;                      \
        NODE_RETURN(tp);                              \
    }

    // READ_TYPE(int)
    // READ_TYPE(bool)
    READ_TYPE(float)
    READ_TYPE(char)
    READ_TYPE(double)

    /* AJG - explicit implementations of read_int and read_bool to beat Win32 */
    NODE_IMPLEMENTATION(IOModule::read_int, int)
    {
        IStreamType::IStream* stream = NODE_ARG_OBJECT(0, IStreamType::IStream);
        char tp[1024];
        *stream->_istream >> tp;

        // #ifdef _MSC_VER
        /* AJG - is a comma the only failure case? */
        // seems to happen everywhere now
        if (tp[strlen(tp) - 1] == ',')
            (*stream->_istream).unget();
        // #endif

        NODE_RETURN(atoi(tp));
    }

    NODE_IMPLEMENTATION(IOModule::read_bool, bool)
    {
        IStreamType::IStream* stream = NODE_ARG_OBJECT(0, IStreamType::IStream);
        char tp[1024];
        *stream->_istream >> tp;

        // #ifdef _MSC_VER
        /* AJG - is a comma the only failure case? */
        // seems to happen everywhere now
        if (tp[strlen(tp) - 1] == ',')
            (*stream->_istream).unget();
        // #endif

        NODE_RETURN(bool(atoi(tp)));
    }

    NODE_IMPLEMENTATION(IOModule::read_byte, char)
    {
        IStreamType::IStream* stream = NODE_ARG_OBJECT(0, IStreamType::IStream);
        char tp[1024];
        *stream->_istream >> tp;

        // #ifdef _MSC_VER
        /* AJG - is this the only failure case? */
        // seems to happen everywhere now
        if (tp[strlen(tp) - 1] == ',')
            (*stream->_istream).unget();
        // #endif

        NODE_RETURN(char(atoi(tp)));
    }

    NODE_IMPLEMENTATION(IOModule::in, Pointer)
    {
        static IStreamType::IStream* std_cin = 0;

        if (!std_cin)
        {
            Process* p = NODE_THREAD.process();
            const Class* c = static_cast<const Class*>(NODE_THIS.type());
            IStreamType::IStream* o = new IStreamType::IStream(c);
            o->retainExternal();
            o->_istream = &std::cin;
            o->setString("*standard in*");
            std_cin = o;
        }

        NODE_RETURN(std_cin);
    }

    NODE_IMPLEMENTATION(IOModule::out, Pointer)
    {
        static OStreamType::OStream* std_cout = 0;

        if (!std_cout)
        {
            Process* p = NODE_THREAD.process();
            const Class* c = static_cast<const Class*>(NODE_THIS.type());
            OStreamType::OStream* o = new OStreamType::OStream(c);
            o->retainExternal();
            o->_ostream = &std::cout;
            o->setString("*standard out*");
            std_cout = o;
        }

        NODE_RETURN(std_cout);
    }

    NODE_IMPLEMENTATION(IOModule::err, Pointer)
    {
        static OStreamType::OStream* std_cerr = 0;

        if (!std_cerr)
        {
            Process* p = NODE_THREAD.process();
            const Class* c = static_cast<const Class*>(NODE_THIS.type());
            OStreamType::OStream* o = new OStreamType::OStream(c);
            o->retainExternal();
            o->_ostream = &std::cerr;
            o->setString("*standard error*");
            std_cerr = o;
        }

        NODE_RETURN(std_cerr);
    }

    NODE_IMPLEMENTATION(IOModule::serialize, int)
    {
        Process* p = NODE_THREAD.process();
        OStreamType::OStream* stream = NODE_ARG_OBJECT(0, OStreamType::OStream);
        Object* obj = NODE_ARG_OBJECT(1, Object);

        Archive::Writer ar(p, p->context());
        ar.add(obj);
        size_t n = ar.write(*stream->_ostream);

        NODE_RETURN(n);
    }

    NODE_IMPLEMENTATION(IOModule::deserialize, Pointer)
    {
        Process* p = NODE_THREAD.process();
        IStreamType::IStream* stream = NODE_ARG_OBJECT(0, IStreamType::IStream);

        Archive::Reader ar(p, p->context());
        const STLVector<Object*>::Type& objects = ar.read(*stream->_istream);
        NODE_RETURN(Pointer(objects.front()));
    }

    //----------------------------------------------------------------------
    //
    //  path module
    //

    static char delimiter = '/';

    NODE_IMPLEMENTATION(IOModule::basename, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* s = NODE_ARG_OBJECT(0, StringType::String);

        int i = s->utf8().rfind(delimiter);

        if (i == string::npos)
        {
            NODE_RETURN(Pointer(s));
        }
        else
        {
            NODE_RETURN(c->stringType()->allocate(s->utf8().substr(i + 1)));
        }
    }

    NODE_IMPLEMENTATION(IOModule::dirname, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* s = NODE_ARG_OBJECT(0, StringType::String);

        String::size_type i = s->utf8().rfind(delimiter);

        if (i > 0 && i == s->size() - 1)
            i = s->utf8().rfind(delimiter, i - 1);

        size_t size = s->size();

        if (i != size - 1)
        {
            StringType::String* r =
                new StringType::String((const Class*)NODE_THIS.type());

            if (i == string::npos)
            {
                NODE_RETURN(c->stringType()->allocate(""));
            }
            else
            {
                NODE_RETURN(
                    c->stringType()->allocate(s->utf8().substr(0, i + 1)));
            }
        }

        NODE_RETURN(Pointer(s));
    }

    NODE_IMPLEMENTATION(IOModule::extension, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* s = NODE_ARG_OBJECT(0, StringType::String);
        if (s == 0)
            NODE_RETURN(0);

        String::size_type i = s->utf8().rfind('.');

        if (i == String::npos)
        {
            NODE_RETURN(c->stringType()->allocate(""));
        }
        else
        {
            NODE_RETURN(
                c->stringType()->allocate(s->utf8().substr(i + 1, s->size())));
        }
    }

    NODE_IMPLEMENTATION(IOModule::without_extension, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* s = NODE_ARG_OBJECT(0, StringType::String);
        if (s == 0)
            NODE_RETURN(0);

        const StringType::String* r = 0;

        String::size_type i = s->utf8().rfind('.');

        if (i == String::npos)
        {
            r = s;
        }
        else
        {
            r = c->stringType()->allocate(s->utf8().substr(0, i));
        }

        NODE_RETURN(Pointer(r));
    }

    NODE_IMPLEMENTATION(IOModule::exists, bool)
    {
        Process* p = NODE_THREAD.process();
        const StringType::String* s = NODE_ARG_OBJECT(0, StringType::String);

#if defined _MSC_VER
        /* AJG - incomplete rewrite!  Probably have to look to fox to figure out
         * how to do this right */
        FILE* fp = _wfopen(UNICODE_C_STR(s->c_str()), UNICODE_C_STR("rb"));

        if (fp)
        {
            fclose(fp);
            return true;
        }
        else
        {
            return false;
        }

#else

        struct stat ss;

        if (stat(s->c_str(), &ss))
        {
            if (errno == ENOTDIR || errno == ENOENT)
            {
                NODE_RETURN(false);
            }
            else
            {
                ThrowErrno(NODE_THREAD, 0);
            }
        }
#endif
        NODE_RETURN(true);
    }

    NODE_IMPLEMENTATION(IOModule::expand, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* s = NODE_ARG_OBJECT(0, StringType::String);
        const char* home = getenv("HOME");

        char nonTilde[3] = {delimiter, '~', 0};
        char twoDel[3] = {delimiter, delimiter, 0};
        String str = s->c_str();
        String::size_type i;

        //
        //  This output of this function needs to be clairified
        //

        if (home)
        {
            while ((i = str.find('~')) != string::npos)
            {
                string::size_type e = str.find(nonTilde, i + 1);
                if (e == string::npos)
                    e = i + 1;
                str.replace(i, e - i, home);
            }
        }

        StringType::String* r = c->stringType()->allocate(str);
        NODE_RETURN(r);
    }

    NODE_IMPLEMENTATION(IOModule::directory, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const ListType* ltype = static_cast<const ListType*>(NODE_THIS.type());
        const Class* tupleType =
            static_cast<const Class*>(ltype->elementType());
        const StringType* stype =
            static_cast<const StringType*>(tupleType->fieldType(0));

        const StringType::String* path = NODE_ARG_OBJECT(0, StringType::String);
        List list(p, ltype);

        struct DirEnt
        {
            StringType::String* name;
            int type;
        };

        if (DIR* d = opendir(path->c_str()))
        {
            struct dirent* e = 0;

            while (e = readdir(d))
            {
                ClassInstance* i = ClassInstance::allocate(tupleType);
                DirEnt* dent = (DirEnt*)i->structure();
                dent->name = stype->allocate(e->d_name);
                /* AJG - not a POSIX standard apparently */
#ifdef _MSC_VER
                dent->type = 0; // e->d_type;
#else
                dent->type = e->d_type;
#endif

                list.append(i);
            }

            closedir(d);
        }
        else
        {
            ThrowErrno(NODE_THREAD, 0);
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(IOModule::join, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* s0 = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* s1 = NODE_ARG_OBJECT(1, StringType::String);

        if (s0 == 0 || s1 == 0)
        {
            throw NilArgumentException(NODE_THREAD);
        }

        String ostr;

        ostr = s0->c_str();
        if (ostr[ostr.size() - 1] != '/')
            ostr += "/";
        ostr += s1->c_str();

        NODE_RETURN(c->stringType()->allocate(ostr));
    }

    NODE_IMPLEMENTATION(IOModule::concat_paths, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* s0 = NODE_ARG_OBJECT(0, StringType::String);
        const StringType::String* s1 = NODE_ARG_OBJECT(1, StringType::String);

        if (s0 == 0 || s1 == 0)
        {
            throw NilArgumentException(NODE_THREAD);
        }

        String r = s0->c_str();
#ifdef WIN32
        r += ";";
#else
        r += ":";
#endif
        r += s1->c_str();

        NODE_RETURN(c->stringType()->allocate(r));
    }

    NODE_IMPLEMENTATION(IOModule::path_separator, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        NODE_RETURN(c->stringType()->allocate("/"));
    }

    NODE_IMPLEMENTATION(IOModule::concat_separator, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
#ifdef WIN32
        NODE_RETURN(c->stringType()->allocate(";"));
#else
        NODE_RETURN(c->stringType()->allocate(":"));
#endif
    }

} // namespace Mu
