//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/CharType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <MuLang/HalfType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/Module.h>
#include <Mu/ReferenceType.h>
#include <Mu/StringHashTable.h>
#include <Mu/ParameterVariable.h>
#include <Mu/Thread.h>
#include <Mu/TupleType.h>
#include <Mu/Vector.h>
#include <Mu/UTF8.h>
#include <ctype.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <iterator>
#include <Mu/utf8_v2/unchecked.h>
#include <Mu/utf8_v2/checked.h>

#if COMPILER == GCC2_96
#include <stdio.h>
#endif

using namespace Mu;
using namespace std;

/* AJG - we do love our windows */
#if defined _MSC_VER
#define snprintf _snprintf
#endif

typedef APIAllocatable::STLVector<TypedValue>::Type FormatArgs;
typedef APIAllocatable::STLVector<Mu::String>::Type StringVector;

static void throwBadFormatOpArgType(Thread& thread, int arg, char key,
                                    const Type* atype)
{
    ostringstream str;
    Process* p = thread.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());

    BadArgumentTypeException exc(thread);
    str << ": argument " << (arg + 1) << " of format operator (%)"
        << " has type " << atype->fullyQualifiedName()
        << " which is incompatible with format %" << key;
    string temp(str.str());
    exc.message() += temp.c_str();
    throw exc;
}

//----------------------------------------------------------------------
//
//  Constructor
//

Pointer string_string(Mu::Thread& NODE_THREAD)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    return (Pointer) new StringType::String(c->stringType());
}

Pointer string_string_int(Mu::Thread& NODE_THREAD, int i)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    char temp[40];
    sprintf(temp, "%d", i);
    return c->stringType()->allocate(temp);
}

Pointer string_string_int64(Mu::Thread& NODE_THREAD, int64 i)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    char temp[80];
    sprintf(temp, "%lld", i);
    return c->stringType()->allocate(temp);
}

Pointer string_string_float(Mu::Thread& NODE_THREAD, float i)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    char temp[40];
    sprintf(temp, "%f", i);
    return c->stringType()->allocate(temp);
}

Pointer string_string_double(Mu::Thread& NODE_THREAD, double i)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    char temp[80];
    sprintf(temp, "%f", i);
    return c->stringType()->allocate(temp);
}

Pointer string_string_byte(Mu::Thread& NODE_THREAD, char i)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    char temp[40];
    sprintf(temp, "%d", int(i));
    return c->stringType()->allocate(temp);
}

Pointer string_string_bool(Mu::Thread& NODE_THREAD, bool i)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    return c->stringType()->allocate(i ? "true" : "false");
}

Pointer string_string_string(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    const StringType::String* i = reinterpret_cast<StringType::String*>(ptr);
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    const StringType* stype = c->stringType();

    if (i)
        return stype->allocate(i->c_str());
    else
        return stype->allocate("nil");
}

Pointer string_string_QMark_class_or_interface(Mu::Thread& NODE_THREAD,
                                               Pointer ptr)
{
    Object* obj = reinterpret_cast<Object*>(ptr);
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());

    if (obj)
    {
        ostringstream str;
        obj->type()->outputValue(str, Value(obj), true);
        NODE_RETURN(c->stringType()->allocate(str));
    }

    throw NilArgumentException(NODE_THREAD);
}

Pointer string_string_QMark_opaque(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    Object* obj = reinterpret_cast<Object*>(ptr);
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());

    if (obj)
    {
        ostringstream str;
        str << "<#opaque " << hex << obj << dec << ">";
        NODE_RETURN(c->stringType()->allocate(str));
    }

    throw NilArgumentException(NODE_THREAD);
}

Pointer string_string_QMark_variant(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    return string_string_QMark_class_or_interface(NODE_THREAD, ptr);
}

Pointer string_string_vector_floatBSB_4ESB_(Mu::Thread& NODE_THREAD, Vector4f i)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());

    char temp[100];
    sprintf(temp, "<%g, %g, %g, %g>", i[0], i[1], i[2], i[3]);
    return c->stringType()->allocate(temp);
}

Pointer string_string_vector_floatBSB_3ESB_(Mu::Thread& NODE_THREAD, Vector3f i)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());

    char temp[100];
    sprintf(temp, "<%g, %g, %g>", i[0], i[1], i[2]);
    return c->stringType()->allocate(temp);
}

Pointer string_string_vector_floatBSB_2ESB_(Mu::Thread& NODE_THREAD, Vector2f i)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());

    char temp[100];
    sprintf(temp, "<%g, %g>", i[0], i[1]);
    return c->stringType()->allocate(temp);
}

//
//  + operator
//

Pointer Plus__string_string_string(Mu::Thread& NODE_THREAD, Pointer ps1,
                                   Pointer ps2)
{
    const StringType::String* s1 = reinterpret_cast<StringType::String*>(ps1);
    const StringType::String* s2 = reinterpret_cast<StringType::String*>(ps2);

    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    StringType::String* o = new StringType::String(c->stringType());

    ostringstream str;

    if (s1)
    {
        str << s1->c_str();
    }
    else
    {
        str << "nil";
    }

    if (s2)
    {
        str << s2->c_str();
    }
    else
    {
        str << "nil";
    }

    return c->stringType()->allocate(str);
}

//
//  Assignment +=
//

Pointer Plus_EQ__stringAmp__stringAmp__string(Mu::Thread& NODE_THREAD,
                                              Pointer pa, Pointer pb)
{
    Pointer* ppa = (Pointer*)pa;
    *ppa = Plus__string_string_string(NODE_THREAD, *ppa, pb);
    return (Pointer*)ppa;
}

int int_int_string(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    const StringType::String* a = reinterpret_cast<StringType::String*>(ptr);
    if (!a)
        throw NilArgumentException(NODE_THREAD);
    return atoi(a->c_str());
}

float float_float_string(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    const StringType::String* a = reinterpret_cast<StringType::String*>(ptr);
    if (!a)
        throw NilArgumentException(NODE_THREAD);
    return atof(a->c_str());
}

double double_double_string(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    const StringType::String* a = reinterpret_cast<StringType::String*>(ptr);
    if (!a)
        throw NilArgumentException(NODE_THREAD);
    return atof(a->c_str());
}

bool bool_bool_string(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    const StringType::String* a = reinterpret_cast<StringType::String*>(ptr);
    if (!a)
        throw NilArgumentException(NODE_THREAD);
    return *a == "true" ? true : false;
}

bool EQ_EQ__bool_string_string(Mu::Thread& NODE_THREAD, Pointer aptr,
                               Pointer bptr)
{
    const StringType::String* a = reinterpret_cast<StringType::String*>(aptr);
    const StringType::String* b = reinterpret_cast<StringType::String*>(bptr);
    if (!a || !b)
        throw NilArgumentException(NODE_THREAD);
    return *a == *b;
}

bool Bang_EQ__bool_string_string(Mu::Thread& NODE_THREAD, Pointer aptr,
                                 Pointer bptr)
{
    const StringType::String* a = reinterpret_cast<StringType::String*>(aptr);
    const StringType::String* b = reinterpret_cast<StringType::String*>(bptr);
    if (!a || !b)
        throw NilArgumentException(NODE_THREAD);

    bool result = a != b;

    if (result)
    {
        result = *a != *b;
    }

    return result;
}

static Pointer format_op(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                         const FormatArgs& args)
{
    const StringType::String* fmt =
        reinterpret_cast<StringType::String*>(fmtPtr);
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    const StringType* stype = static_cast<const StringType*>(c->stringType());
    int nargs = args.size();

    vector<regmatch_t> matches(6);
    int argnum = 0;
    string fmtsub;
    string fmtstr = fmt->c_str();
    vector<char> temp(128);

    ostringstream outstr;

    int regexec_result;

    while (!fmtstr.empty()
           && (regexec_result = regexec(&StringType::_format_re, fmtstr.c_str(),
                                        matches.size(), &matches.front(), 0))
                  != REG_NOMATCH)
    {
        if (regexec_result)
        {
            char errtemp[256];
            size_t n =
                regerror(regexec_result, &StringType::_format_re, errtemp, 256);
            cout << "ERROR: format_op() => " << errtemp << endl;
        }

        if (matches[1].rm_so != size_t(-1))
        {
            outstr << fmtstr.substr(matches[1].rm_so,
                                    matches[1].rm_eo - matches[1].rm_so);
        }

        if (matches[2].rm_so != size_t(-1))
        {
            char key = fmtstr[matches[2].rm_eo - 1];

            if (key == '%')
            {
                outstr << "%";
            }
            else
            {
                if (argnum >= nargs)
                    break;

                fmtsub = fmtstr.substr(matches[2].rm_so,
                                       matches[2].rm_eo - matches[2].rm_so);

                const Type* t = args[argnum]._type;
                int n = temp.size() - 1;

                //
                //      NOTE: funky n < 0 and unknown size crap for
                //      windows _snprintf which is horribly broken.
                //

                switch (key)
                {
                case 'd':
                case 'D':
                case 'o':
                case 'u':
                case 'X':
                case 'x':
                    if (t == c->intType() || t == c->charType())
                    {
                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(),
                                         args[argnum]._value._int);
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    else if (t == c->shortType())
                    {
                        string ts = fmtsub.c_str();
                        string::size_type p = ts.find(key);
                        string fm;
                        fm += "h";
                        fm += key;
                        ts.replace(p, 1, fm);

                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(),
                                         args[argnum]._value._short);
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    else if (t == c->byteType())
                    {
                        string ts = fmtsub.c_str();
                        string::size_type p = ts.find(key);
                        string fm;
                        fm += "hu";
                        ts.replace(p, 1, fm);

                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(),
                                         args[argnum]._value._char);
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    else if (t == c->int64Type())
                    {
                        string ts = fmtsub.c_str();
                        string::size_type p = ts.find(key);
                        string fm;
                        fm += "ll";
                        fm += key;
                        ts.replace(p, 1, fm);

                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(),
                                         args[argnum]._value._int64);
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    else
                    {
                        ::throwBadFormatOpArgType(NODE_THREAD, argnum, key, t);
                    }
                    break;

                case 'f':
                case 'F':
                case 'e':
                case 'E':
                case 'g':
                case 'G':
                    if (t == c->floatType())
                    {
                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(),
                                         args[argnum]._value._float);
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    else if (t == c->doubleType())
                    {
                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(),
                                         args[argnum]._value._double);
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    else if (t == c->halfType())
                    {
                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            Value v = args[argnum]._value;
                            float f = shortToHalf(v._short);

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(), f);
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    else
                    {
                        ::throwBadFormatOpArgType(NODE_THREAD, argnum, key, t);
                    }
                    break;

                case 'c':
                    if (t == c->charType())
                    {
                        int c = args[argnum]._value._int;
                        temp.clear();
                        utf8::utf32to8(&c, &c + 1, back_inserter(temp));
                        temp.push_back(0);
                    }
                    else
                    {
                        ::throwBadFormatOpArgType(NODE_THREAD, argnum, key, t);
                    }
                    break;

                case 's':
                    if (t == c->stringType())
                    {
                        const StringType::String* o =
                            (StringType::String*)args[argnum]._value._Pointer;

                        if (!o)
                        {
                            o = static_cast<const StringType*>(t)->allocate(
                                "nil");
                            // throw NilArgumentException(NODE_THREAD);
                        }

                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(), o->c_str());
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    else if (t == c->charArrayType())
                    {
                        DynamicArray* array =
                            (DynamicArray*)args[argnum]._value._Pointer;

                        vector<char> str;

                        if (!array)
                        {
                            str.push_back('n');
                            str.push_back('i');
                            str.push_back('l');
                        }
                        else
                        {
                            utf8::utf32to8(array->begin<int>(),
                                           array->end<int>(),
                                           back_inserter(str));
                        }

                        str.push_back(0);
                        string result = &str.front();

                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(), result.c_str());
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    else
                    {
                        ostringstream str;

                        if (t)
                        {
                            t->outputValue(str, args[argnum]._value, true);
                        }
                        else
                        {
                            str << "<#opaque " << hex
                                << args[argnum]._value._Pointer << dec << ">";
                        }

                        do
                        {
                            if (n + 1 > temp.size())
                                temp.resize(n + 1);
                            else if (n < 0)
                                temp.resize(temp.size() * 2);

                            string stemp(str.str());

                            n = snprintf(&temp.front(), temp.size(),
                                         fmtsub.c_str(), stemp.c_str());
                        } while (n + 1 > temp.size() || n < 0);
                    }
                    break;
                }

                temp.back() = 0;
                outstr << &temp.front();
                argnum++;
            }
        }

        fmtstr.erase(0, matches[0].rm_eo);
    }

    return stype->allocate(outstr);
}

Pointer PCent__string_string_int(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                                 int arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._int = arg;
    args.front()._type = c->intType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_int64(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                                   int64 arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._int64 = arg;
    args.front()._type = c->int64Type();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_float(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                                   float arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._float = arg;
    args.front()._type = c->floatType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_double(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                                    double arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._double = arg;
    args.front()._type = c->doubleType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_half(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                                  short arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._short = arg;
    args.front()._type = c->halfType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_short(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                                   short arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._short = arg;
    args.front()._type = c->shortType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_char(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                                  int arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._int = arg;
    args.front()._type = c->charType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_byte(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                                  char arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._char = arg;
    args.front()._type = c->byteType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_QMark_class_not_tuple(Mu::Thread& NODE_THREAD,
                                                   Pointer fmtPtr, Pointer arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    ClassInstance* o = reinterpret_cast<ClassInstance*>(arg);

    args.front()._value._Pointer = arg;
    args.front()._type = o ? o->type() : 0;
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_QMark_opaque(Mu::Thread& NODE_THREAD,
                                          Pointer fmtPtr, Pointer arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._Pointer = arg;
    args.front()._type = 0;
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_bool(Mu::Thread& NODE_THREAD, Pointer fmtPtr,
                                  bool arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._bool = arg;
    args.front()._type = c->boolType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_vector_floatBSB_4ESB_(Mu::Thread& NODE_THREAD,
                                                   Pointer fmtPtr, Vector4f arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._Vector4f = arg;
    args.front()._type = c->vec4fType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_vector_floatBSB_3ESB_(Mu::Thread& NODE_THREAD,
                                                   Pointer fmtPtr, Vector3f arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._Vector3f = arg;
    args.front()._type = c->vec3fType();
    return (Pointer)format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_vector_floatBSB_2ESB_(Mu::Thread& NODE_THREAD,
                                                   Pointer fmtPtr, Vector2f arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._Vector2f = arg;
    args.front()._type = c->vec2fType();
    return format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_charBSB_2ESB_(Mu::Thread& NODE_THREAD,
                                           Pointer fmtPtr, Pointer arg)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    FormatArgs args(1);

    args.front()._value._Pointer = arg;
    args.front()._type = c->arrayType(c->charType(), 1, 0);
    return format_op(NODE_THREAD, fmtPtr, args);
}

Pointer PCent__string_string_QMark_tuple(Mu::Thread& NODE_THREAD,
                                         Pointer fmtPtr, Pointer argPtr)
{
    ClassInstance* obj = (ClassInstance*)argPtr;

    if (!obj)
    {
        ostringstream str;
        NilArgumentException exc(NODE_THREAD);
        exc.message() += ": nil tuple argument to format operator (%)";
        throw exc;
    }

    const TupleType* t = static_cast<const TupleType*>(obj->type());
    const TupleType::Types& types = t->tupleFieldTypes();
    size_t n = types.size();
    FormatArgs args(n);

    for (int i = 0; i < n; i++)
    {
        const Type* atype = types[i];
        args[i]._type = atype;
        const MachineRep* rep = atype->machineRep();

        if (rep == FloatRep::rep())
        {
            args[i]._value._float = *(float*)obj->field(i);
        }
        else if (rep == DoubleRep::rep())
        {
            args[i]._value._double = *(double*)obj->field(i);
        }
        else if (rep == IntRep::rep())
        {
            args[i]._value._int = *(int*)obj->field(i);
        }
        else if (rep == Int64Rep::rep())
        {
            args[i]._value._int64 = *(int64*)obj->field(i);
        }
        else if (rep == ShortRep::rep())
        {
            short s = *(short*)obj->field(i);
            args[i]._value._short = s;
        }
        else if (rep == CharRep::rep())
        {
            args[i]._value._char = *(char*)obj->field(i);
        }
        else if (rep == BoolRep::rep())
        {
            args[i]._value._bool = *(bool*)obj->field(i);
        }
        else if (rep == Vector4FloatRep::rep())
        {
            args[i]._value._Vector4f = *(Vector4f*)obj->field(i);
        }
        else if (rep == Vector3FloatRep::rep())
        {
            args[i]._value._Vector3f = *(Vector3f*)obj->field(i);
        }
        else if (rep == Vector2FloatRep::rep())
        {
            args[i]._value._Vector2f = *(Vector2f*)obj->field(i);
        }
        else if (rep == PointerRep::rep())
        {
            args[i]._value._Pointer = *(Pointer*)obj->field(i);
        }
    }

    return format_op(NODE_THREAD, fmtPtr, args);
}

void print_void_string(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    if (const StringType::String* o =
            reinterpret_cast<StringType::String*>(ptr))
    {
        cout << o->c_str() << flush;
    }
    else
    {
        cout << "nil" << flush;
    }
}

int string_size_int_string(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    const StringType::String* o = reinterpret_cast<StringType::String*>(ptr);
    if (!o)
        throw NilArgumentException(NODE_THREAD);
    return o->numChars();
}

int string_hash_int_string(Mu::Thread& NODE_THREAD, Pointer ptr)
{
    const StringType::String* o = reinterpret_cast<StringType::String*>(ptr);
    if (!o)
        throw NilArgumentException(NODE_THREAD);
    return o->hash();
}

int compare_int_string_string(Mu::Thread& NODE_THREAD, Pointer aptr,
                              Pointer bptr)
{
    const StringType::String* a = reinterpret_cast<StringType::String*>(aptr);
    const StringType::String* b = reinterpret_cast<StringType::String*>(bptr);
    if (!a || !b)
        throw NilArgumentException(NODE_THREAD);

    UTF32String abuf, bbuf;
    a->copyConvert(abuf);
    b->copyConvert(bbuf);

    return abuf.compare(bbuf);
}

Pointer string_join_string_stringBSB_ESB__string(Mu::Thread& NODE_THREAD,
                                                 Pointer aptr, Pointer bptr)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    DynamicArray* array = reinterpret_cast<DynamicArray*>(aptr);
    const StringType::String* del = reinterpret_cast<StringType::String*>(bptr);
    if (!array || !del)
        throw NilArgumentException(NODE_THREAD);

    const StringType* stype = static_cast<const StringType*>(del->type());
    const size_t size = array->size();

    ostringstream sout;

    for (int i = 0; i < size; i++)
    {
        if (i)
            sout << del->c_str();
        const StringType::String* e = array->element<StringType::String*>(i);
        sout << e->c_str();
    }

    return stype->allocate(sout);
}

Pointer string_split_stringBSB_ESB__string_string_bool(Mu::Thread& NODE_THREAD,
                                                       Pointer aptr,
                                                       Pointer bptr, bool seq)
{
    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    const StringType::String* str = reinterpret_cast<StringType::String*>(aptr);
    const StringType::String* del = reinterpret_cast<StringType::String*>(bptr);
    if (!str || !del)
        throw NilArgumentException(NODE_THREAD);

    StringVector buffer;

    if (seq)
    {
        UTF8String text = str->utf8();
        UTF8String seperator = del->utf8();
        UTF8String::size_type p0 = text.empty() ? UTF8String::npos : 0;
        UTF8String::size_type p1 = 0;

        while (p0 != UTF8String::npos && p0 < text.size())
        {
            p1 = text.find(seperator, p0);
            buffer.push_back(text.substr(p0, p1 - p0));
            if (p1 != UTF8String::npos)
                p0 = p1 + seperator.size();
            else
                p0 = p1;
        }
    }
    else
    {
        UTF8tokenize(buffer, str->utf8(), del->utf8());
    }

    DynamicArrayType::SizeVector sizes(1);
    sizes.front() = 0;
    const StringType* stype = static_cast<const StringType*>(str->type());
    const DynamicArrayType* atype =
        static_cast<DynamicArrayType*>(c->arrayType(stype, sizes));

    DynamicArray* array = new DynamicArray(atype, 1);
    int n = buffer.size();
    array->resize(n);

    for (int i = 0; i < n; i++)
    {
        array->element<StringType::String*>(i) = stype->allocate(buffer[i]);
    }

    return array;
}

int string_BSB_ESB__char_string_int(Mu::Thread& NODE_THREAD, Pointer aptr,
                                    int a)
{
    //
    //  This is the index operator.
    //

    const StringType::String* self =
        reinterpret_cast<StringType::String*>(aptr);
    if (!self)
        throw NilArgumentException(NODE_THREAD);

    const char* sp = self->c_str();
    int s = self->numChars();

    if (a < 0)
        a += s;

    if (a < s)
    {
        for (int i = 0; i < s; i++)
        {
            int n;
            UTF32Char c = UTF8convert(sp, n);

            if (i == a)
                NODE_RETURN(c);
            sp += n;
        }
    }

    throw OutOfRangeException();
}

Pointer string_substr_string_string_int_int(Mu::Thread& NODE_THREAD,
                                            Pointer aptr, int start, int len)
{
    const StringType::String* s = reinterpret_cast<StringType::String*>(aptr);
    if (!s)
        throw NilArgumentException(NODE_THREAD);

    Process* p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    int usize = s->numChars();

    if (start < 0)
    {
        start += int(usize);
    }

    if (len <= 0)
    {
        len = usize + len - start;
    }

    if (start + len > usize)
        len = usize - start;

    const char* str = s->c_str();
    const size_t strsize = s->size();

    const char* beg = str;
    utf8::advance(beg, start, str + strsize);
    const char* end = beg;
    utf8::advance(end, len, str + strsize);

    string os;
    os.insert(0, beg, end - beg);
    return c->stringType()->allocate(os);
}

//----------------------------------------------------------------------

namespace Mu
{
    using namespace std;
    using namespace Mu;

    //----------------------------------------------------------------------

    regex_t StringType::_format_re;
    bool StringType::_init = true;
    static const char* blank = 0;

    StringType::String::String(const Class* c)
        : ClassInstance(c)
    {
        _utf8string = blank;
    }

    StringType::String* StringType::allocate(const Mu::String& instr) const
    {
        StringType::String* s = new StringType::String(this);
        s->set(instr);
        return s;
    }

    StringType::String* StringType::allocate(const Mu::Name& instr) const
    {
        StringType::String* s = new StringType::String(this);
        s->set(instr);
        return s;
    }

    StringType::String* StringType::allocate(const std::string& instr) const
    {
        StringType::String* s = new StringType::String(this);
        s->setn(instr.c_str(), instr.size());
        return s;
    }

    StringType::String* StringType::allocate(const ostringstream& str) const
    {
        StringType::String* s = new StringType::String(this);
        s->set(str.str());
        return s;
    }

    StringType::String* StringType::allocate(const char* instr) const
    {
        StringType::String* s = new StringType::String(this);
        s->setn(instr, strlen(instr));
        return s;
    }

    StringType::String* StringType::allocate(size_t n) const
    {
        StringType::String* s = new StringType::String(this);
        return s;
    }

    void StringType::String::setn(const char* s, size_t size)
    {
        // if (_utf8string && _utf8string != blank) MU_GC_FREE(_utf8string);
        char* p = (char*)MU_GC_ALLOC_ATOMIC(size + 1);
        strcpy(p, s);
        p[size] = 0;
        _utf8string = p;
    }

    void StringType::String::set(const char* s)
    {
        // if (_utf8string && _utf8string != blank)
        // MU_GC_FREE((void*)_utf8string);
        size_t size = strlen(s);
        char* p = (char*)MU_GC_ALLOC_ATOMIC(size + 1);
        strcpy(p, s);
        p[size] = 0;
        _utf8string = p;
    }

    unsigned long StringType::String::hash() const
    {
        //
        //	Taken from the ELF format hash function
        //

        unsigned long h = 0, g;

        for (int i = 0, l = size(); i < l; i++)
        {
            h = (h << 4) + _utf8string[i];
            if (g = h & 0xf0000000)
                h ^= g >> 24;
            h &= ~g;
        }

        return h;
    }

    size_t StringType::String::numChars() const { return UTF8len(_utf8string); }

    bool StringType::String::operator==(const char* b) const
    {
        return compare(b) == 0;
    }

    bool StringType::String::operator==(const StringType::String& b) const
    {
        return compare(b.c_str()) == 0;
    }

    bool StringType::String::operator!=(const char* b) const
    {
        return compare(b) != 0;
    }

    bool StringType::String::operator!=(const StringType::String& b) const
    {
        return compare(b.c_str()) != 0;
    }

    int StringType::String::compare(const char* p) const
    {
        if (p && _utf8string)
        {
            return strcmp(p, _utf8string);
        }
        else
        {
            throw NilArgumentException();
        }
    }

    UTF32String StringType::String::utf32() const
    {
        UTF32String abuf;
        copyConvert(abuf);
        return abuf;
    }

    UTF16String StringType::String::utf16() const
    {
        UTF16String abuf;
        copyConvert(abuf);
        return abuf;
    }

    stdUTF32String StringType::String::utf32std() const
    {
        stdUTF32String abuf;
        copyConvert(abuf);
        return abuf;
    }

    stdUTF16String StringType::String::utf16std() const
    {
        stdUTF16String abuf;
        copyConvert(abuf);
        return abuf;
    }

    void StringType::String::copyConvert(UTF32String& s) const
    {
        utf8::utf8to32(_utf8string, _utf8string + size(), back_inserter(s));
    }

    void StringType::String::copyConvert(UTF16String& s) const
    {
        utf8::utf8to16(_utf8string, _utf8string + size(), back_inserter(s));
    }

    void StringType::String::copyConvert(UTF8String& s) const
    {
        s = _utf8string;
    }

    void StringType::String::copyConvert(stdUTF32String& s) const
    {
        utf8::utf8to32(_utf8string, _utf8string + size(), back_inserter(s));
    }

    void StringType::String::copyConvert(stdUTF16String& s) const
    {
        utf8::utf8to16(_utf8string, _utf8string + size(), back_inserter(s));
    }

    void StringType::String::copyConvert(stdUTF8String& s) const
    {
        s = _utf8string;
    }

    //----------------------------------------------------------------------

    StringType::StringType(Context* c, Class* super)
        : Class(c, "string", super)
    {
    }

    StringType::~StringType() {}

    Object* StringType::newObject() const { return new String(this); }

    size_t StringType::objectSize() const { return sizeof(StringType::String); }

    void StringType::constructInstance(Pointer obj) const
    {
        new (obj) StringType::String(this);
    }

    void StringType::freeze()
    {
        Class::freeze();
        _isGCAtomic = false;
    }

    void StringType::copyInstance(Pointer a, Pointer b) const
    {
        StringType::String* src = reinterpret_cast<StringType::String*>(a);
        StringType::String* dst = reinterpret_cast<StringType::String*>(b);
        dst->set(src->c_str());
    }

    void StringType::deleteObject(Object* obj) const
    {
        delete static_cast<StringType::String*>(obj);
    }

    void StringType::outputValue(ostream& o, const Value& value,
                                 bool full) const
    {
        String* s = reinterpret_cast<String*>(value._Pointer);

        if (s)
        {
            outputQuotedString(o, s->utf8());
        }
        else
        {
            o << "nil";
        }
    }

    void StringType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                          ValueOutputState&) const
    {
        if (vp)
        {
            const String* s = *reinterpret_cast<const String**>(vp);

            if (s)
            {
                outputQuotedString(o, s->c_str());
            }
            else
            {
                o << "nil";
            }
        }
        else
        {
            o << "nil";
        }
    }

    void StringType::outputQuotedString(ostream& o, const Mu::String& str,
                                        char delim)
    {
        o << delim;

        for (int i = 0; i < str.size(); i++)
        {
            char c = str[i];

            if (c == 0)
            {
                o << "^@";
            }
            else if (iscntrl(c))
            {
                o << "\\";
                switch (c)
                {
                case '\n':
                    o << "n";
                    break;
                case '\b':
                    o << "b";
                    break;
                case '\r':
                    o << "r";
                    break;
                case '\t':
                    o << "t";
                    break;
                default:
                {
                    //
                    //    escaped octal unicode
                    //
                    ostringstream str;
                    str << "u" << setfill('0') << setw(4) << hex << int(c);
                    o << str.str();
                    break;
                }
                }
            }
            else if (c == delim)
            {
                o << '\\' << delim;
            }
            else if (c & 0x80)
            {
                // UTF-8
                o << c;
            }
            else
            {
                o << c;
            }
        }

        o << delim;
    }

    void StringType::serialize(std::ostream& o, Archive::Writer& archive,
                               const ValuePointer p) const
    {
        const String* ci = *static_cast<const String**>(p);
        o << ci->c_str();
        o.put(0);
        Class::serialize(o, archive, p);
    }

    void StringType::deserialize(std::istream& i, Archive::Reader& archive,
                                 ValuePointer p) const
    {
        String* ci = *static_cast<String**>(p);
        ostringstream str;

        for (int ch; ch = i.get();)
        {
            str << char(ch);
        }

        ci->set(str.str());
        Class::deserialize(i, archive, p);
    }

#define T Thread&

    Pointer __C_EQ__stringAmp__stringAmp__string(T, Pointer a, Pointer b)
    {
        return *(Pointer*)(a = b);
    }

#undef T

    void StringType::load()
    {
        if (_init)
        {
            char* b = (char*)MU_GC_ALLOC_ATOMIC(1);
            *b = 0;
            blank = b;

            if (int err =
                    regcomp(&_format_re,
                            "([^%]*)?(%[-+ 0]*([0-9]*)(\\.[0-9]+)?[a-zA-Z%])?",
                            REG_EXTENDED))
            {
                vector<char> temp(1);
                size_t n =
                    regerror(err, &_format_re, &temp.front(), temp.size());
                temp.resize(n + 1);
                regerror(err, &_format_re, &temp.front(), temp.size());

                cerr << "ERROR: internal format re failed: " << &temp.front()
                     << endl;
            }

            _init = false;
        }

        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();

        s->addSymbols(
            new ReferenceType(c, "string&", this),

            new Function(c, "string", StringType::construct, None, Compiled,
                         string_string, Return, "string", End),

            new Function(c, "string", BaseFunctions::dereference, Cast, Return,
                         "string", Args, "string&", End),

            new Function(c, "string", StringType::from_int, Cast, Compiled,
                         string_string_int, Return, "string", Args, "int", End),

            new Function(c, "string", StringType::from_int64, Cast, Compiled,
                         string_string_int64, Return, "string", Args, "int64",
                         End),

            new Function(c, "string", StringType::from_float, Lossy, Compiled,
                         string_string_float, Return, "string", Args, "float",
                         End),

            new Function(c, "string", StringType::from_double, Lossy, Compiled,
                         string_string_double, Return, "string", Args, "double",
                         End),

            new Function(c, "string", StringType::from_bool, Cast, Compiled,
                         string_string_bool, Return, "string", Args, "bool",
                         End),

            new Function(c, "string", StringType::from_byte, Cast, Compiled,
                         string_string_byte, Return, "string", Args, "byte",
                         End),

            new Function(c, "string", StringType::from_vector4, Cast, Compiled,
                         string_string_vector_floatBSB_4ESB_, Return, "string",
                         Args, "vector float[4]", End),

            new Function(c, "string", StringType::from_vector3, Cast, Compiled,
                         string_string_vector_floatBSB_3ESB_, Return, "string",
                         Args, "vector float[3]", End),

            new Function(c, "string", StringType::from_vector2, Cast, Compiled,
                         string_string_vector_floatBSB_2ESB_, Return, "string",
                         Args, "vector float[2]", End),

            new Function(c, "string", StringType::from_string, None, Compiled,
                         string_string_string, Return, "string", Args, "string",
                         End),

            new Function(c, "string", StringType::from_class, Cast, Compiled,
                         string_string_QMark_class_or_interface, Return,
                         "string", Args, "?class_or_interface", End),

            new Function(c, "string", StringType::from_opaque, Cast, Compiled,
                         string_string_QMark_opaque, Return, "string", Args,
                         "?opaque", End),

            new Function(c, "string", StringType::from_variant, Cast, Compiled,
                         string_string_QMark_variant, Return, "string", Args,
                         "?variant", End),

            new Function(c, "int", StringType::to_int, Mapped, Compiled,
                         int_int_string, Return, "int", Args, "string", End),

            new Function(c, "float", StringType::to_float, Mapped, Compiled,
                         float_float_string, Return, "float", Args, "string",
                         End),

            new Function(c, "double", StringType::to_double, Mapped, Compiled,
                         double_double_string, Return, "double", Args, "string",
                         End),

            new Function(c, "bool", StringType::to_bool, Mapped, Compiled,
                         bool_bool_string, Return, "bool", Args, "string", End),

            new Function(c, "=", BaseFunctions::assign, AsOp | NativeInlined,
                         Compiled, __C_EQ__stringAmp__stringAmp__string, Return,
                         "string&", Args, "string&", "string", End),

            new Function(c, "%", StringType::formatOp, Function::Operator,
                         Compiled, PCent__string_string_QMark_tuple, Return,
                         "string", Args, "string", "?tuple", End),

            new Function(c, "%", StringType::formatOp_class, Function::Operator,
                         Compiled, PCent__string_string_QMark_class_not_tuple,
                         Return, "string", Args, "string", "?object_not_tuple",
                         End),

            new Function(c, "%", StringType::formatOp_opaque,
                         Function::Operator, Compiled,
                         PCent__string_string_QMark_opaque, Return, "string",
                         Args, "string", "?opaque", End),

            new Function(c, "%", StringType::formatOp_int, Function::Operator,
                         Compiled, PCent__string_string_int, Return, "string",
                         Args, "string", "int", End),

            new Function(c, "%", StringType::formatOp_int64, Function::Operator,
                         Compiled, PCent__string_string_int64, Return, "string",
                         Args, "string", "int64", End),

            new Function(c, "%", StringType::formatOp_float, Function::Operator,
                         Compiled, PCent__string_string_float, Return, "string",
                         Args, "string", "float", End),

            new Function(c, "%", StringType::formatOp_double,
                         Function::Operator, Compiled,
                         PCent__string_string_double, Return, "string", Args,
                         "string", "double", End),

            new Function(c, "%", StringType::formatOp_half, Function::Operator,
                         Compiled, PCent__string_string_half, Return, "string",
                         Args, "string", "half", End),

            new Function(c, "%", StringType::formatOp_char, Function::Operator,
                         Compiled, PCent__string_string_char, Return, "string",
                         Args, "string", "char", End),

            new Function(c, "%", StringType::formatOp_bool, Function::Operator,
                         Compiled, PCent__string_string_bool, Return, "string",
                         Args, "string", "bool", End),

            new Function(c, "%", StringType::formatOp_byte, Function::Operator,
                         Compiled, PCent__string_string_byte, Return, "string",
                         Args, "string", "byte", End),

            new Function(c, "%", StringType::formatOp_short, Function::Operator,
                         Compiled, PCent__string_string_short, Return, "string",
                         Args, "string", "short", End),

            new Function(c, "%", StringType::formatOp_Vector4f,
                         Function::Operator, Compiled,
                         PCent__string_string_vector_floatBSB_4ESB_, Return,
                         "string", Args, "string", "vector float[4]", End),

            new Function(c, "%", StringType::formatOp_Vector3f,
                         Function::Operator, Compiled,
                         PCent__string_string_vector_floatBSB_3ESB_, Return,
                         "string", Args, "string", "vector float[3]", End),

            new Function(c, "%", StringType::formatOp_Vector2f,
                         Function::Operator, Compiled,
                         PCent__string_string_vector_floatBSB_2ESB_, Return,
                         "string", Args, "string", "vector float[2]", End),

            new Function(c, "%", StringType::formatOp_charArray,
                         Function::Operator, Compiled,
                         PCent__string_string_charBSB_2ESB_, Return, "string",
                         Args, "string", "char[]", End),

            new Function(c, "+", StringType::plus, Op, Compiled,
                         Plus__string_string_string, Return, "string", Args,
                         "string", "string", End),

            new Function(c, "==", StringType::equals, CommOp, Compiled,
                         EQ_EQ__bool_string_string, Return, "bool", Args,
                         "string", "string", End),

            new Function(c, "!=", StringType::notequals, CommOp, Compiled,
                         Bang_EQ__bool_string_string, Return, "bool", Args,
                         "string", "string", End),

            new Function(c, "print", StringType::print, None, Compiled,
                         print_void_string, Return, "void", Args, "string",
                         End),

            new Function(c, "+=", StringType::assignPlus, AsOp, Compiled,
                         Plus_EQ__stringAmp__stringAmp__string, Return,
                         "string&", Args, "string&", "string", End),

            new Function(c, "compare", StringType::compare, Mapped, Compiled,
                         compare_int_string_string, Return, "int", Args,
                         "string", "string", End),

            EndArguments);

        //
        //  Can't make the array type until the reference type above exists
        //

        MuLangContext* context = (MuLangContext*)globalModule()->context();
        context->arrayType(this, 1, 0);

        addSymbols(
            new Function(c, "size", StringType::size, Mapped, Compiled,
                         string_size_int_string, Return, "int", Parameters,
                         new ParameterVariable(c, "this", "string"), End),

            new Function(c, "substr", StringType::substr, Mapped, Compiled,
                         string_substr_string_string_int_int, Return, "string",
                         Parameters, new ParameterVariable(c, "this", "string"),
                         new ParameterVariable(c, "index0", "int"),
                         new ParameterVariable(c, "length", "int"), End),

            new Function(c, "[]", StringType::index, Mapped, Compiled,
                         string_BSB_ESB__char_string_int, Return, "char",
                         Parameters, new ParameterVariable(c, "this", "string"),
                         new ParameterVariable(c, "index", "int"), End),

            new Function(
                c, "split", StringType::split, Mapped, Compiled,
                string_split_stringBSB_ESB__string_string_bool, Return,
                "string[]", Parameters,
                new ParameterVariable(c, "this", "string"),
                new ParameterVariable(c, "delim", "string"),
                new ParameterVariable(c, "sequence", "bool", Value(false)),
                End),

            new Function(c, "join", StringType::join_array, Mapped, Compiled,
                         string_join_string_stringBSB_ESB__string, Return,
                         "string", Parameters,
                         new ParameterVariable(c, "strings", "string[]"),
                         new ParameterVariable(c, "seperator", "string"), End),

            new Function(c, "hash", StringType::hash, Mapped, Compiled,
                         string_hash_int_string, Return, "int", Parameters,
                         new ParameterVariable(c, "this", "string"), End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(StringType::construct, Pointer)
    {
        NODE_RETURN(::string_string(NODE_THREAD));
    }

    NODE_IMPLEMENTATION(StringType::assignPlus, Pointer)
    {
        Pointer* i = reinterpret_cast<Pointer*>(NODE_ARG(0, Pointer));
        Pointer o = NODE_ARG(1, Pointer);
        NODE_RETURN(::Plus_EQ__stringAmp__stringAmp__string(NODE_THREAD,
                                                            (Pointer)i, o));
    }

    NODE_IMPLEMENTATION(StringType::plus, Pointer)
    {
        String* s1 = reinterpret_cast<String*>(NODE_ARG(0, Pointer));
        String* s2 = reinterpret_cast<String*>(NODE_ARG(1, Pointer));
        NODE_RETURN(Pointer(::Plus__string_string_string(NODE_THREAD, s1, s2)));
    }

    NODE_IMPLEMENTATION(StringType::from_int, Pointer)
    {
        NODE_RETURN(string_string_int(NODE_THREAD, NODE_ARG(0, int)));
    }

    NODE_IMPLEMENTATION(StringType::from_int64, Pointer)
    {
        NODE_RETURN(string_string_int64(NODE_THREAD, NODE_ARG(0, int64)));
    }

    NODE_IMPLEMENTATION(StringType::from_float, Pointer)
    {
        NODE_RETURN(string_string_float(NODE_THREAD, NODE_ARG(0, float)));
    }

    NODE_IMPLEMENTATION(StringType::from_double, Pointer)
    {
        NODE_RETURN(string_string_double(NODE_THREAD, NODE_ARG(0, double)));
    }

    NODE_IMPLEMENTATION(StringType::from_bool, Pointer)
    {
        NODE_RETURN(string_string_bool(NODE_THREAD, NODE_ARG(0, bool)));
    }

    NODE_IMPLEMENTATION(StringType::from_byte, Pointer)
    {
        NODE_RETURN(string_string_byte(NODE_THREAD, NODE_ARG(0, char)));
    }

    NODE_IMPLEMENTATION(StringType::from_vector4, Pointer)
    {
        NODE_RETURN(string_string_vector_floatBSB_4ESB_(NODE_THREAD,
                                                        NODE_ARG(0, Vector4f)));
    }

    NODE_IMPLEMENTATION(StringType::from_vector3, Pointer)
    {
        NODE_RETURN(string_string_vector_floatBSB_3ESB_(NODE_THREAD,
                                                        NODE_ARG(0, Vector3f)));
    }

    NODE_IMPLEMENTATION(StringType::from_vector2, Pointer)
    {
        NODE_RETURN(string_string_vector_floatBSB_2ESB_(NODE_THREAD,
                                                        NODE_ARG(0, Vector2f)));
    }

    NODE_IMPLEMENTATION(StringType::from_string, Pointer)
    {
        NODE_RETURN(string_string_string(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::from_class, Pointer)
    {
        NODE_RETURN(string_string_QMark_class_or_interface(
            NODE_THREAD, NODE_ARG_OBJECT(0, Object)));
    }

    NODE_IMPLEMENTATION(StringType::from_opaque, Pointer)
    {
        NODE_RETURN(string_string_QMark_opaque(NODE_THREAD,
                                               NODE_ARG_OBJECT(0, Object)));
    }

    NODE_IMPLEMENTATION(StringType::from_variant, Pointer)
    {
        NODE_RETURN(string_string_QMark_variant(NODE_THREAD,
                                                NODE_ARG_OBJECT(0, Object)));
    }

    NODE_IMPLEMENTATION(StringType::to_int, int)
    {
        NODE_RETURN(int_int_string(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::to_float, float)
    {
        NODE_RETURN(float_float_string(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::to_double, double)
    {
        NODE_RETURN(double_double_string(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::to_bool, bool)
    {
        NODE_RETURN(bool_bool_string(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::equals, bool)
    {
        NODE_RETURN(EQ_EQ__bool_string_string(NODE_THREAD, NODE_ARG(0, Pointer),
                                              NODE_ARG(1, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::notequals, bool)
    {
        NODE_RETURN(Bang_EQ__bool_string_string(
            NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::formatOp, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        Pointer arg = NODE_ARG(1, Pointer);
        NODE_RETURN(PCent__string_string_QMark_tuple(NODE_THREAD, fmt, arg));
    }

#define FORMAT_OPTYPE(TYPE)                                              \
    NODE_IMPLEMENTATION(StringType::formatOp_##TYPE, Pointer)            \
    {                                                                    \
        Pointer fmt = NODE_ARG(0, Pointer);                              \
        TYPE arg = NODE_ARG(1, TYPE);                                    \
        NODE_RETURN(PCent__string_string_##TYPE(NODE_THREAD, fmt, arg)); \
    }

    FORMAT_OPTYPE(int)
    FORMAT_OPTYPE(int64)
    FORMAT_OPTYPE(float)
    FORMAT_OPTYPE(double)
    FORMAT_OPTYPE(short)
    FORMAT_OPTYPE(bool)

    NODE_IMPLEMENTATION(StringType::formatOp_half, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        short arg = NODE_ARG(1, short);
        NODE_RETURN(PCent__string_string_half(NODE_THREAD, fmt, arg));
    }

    NODE_IMPLEMENTATION(StringType::formatOp_char, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        int arg = NODE_ARG(1, int);
        NODE_RETURN(PCent__string_string_char(NODE_THREAD, fmt, arg));
    }

    NODE_IMPLEMENTATION(StringType::formatOp_Vector4f, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        Vector4f arg = NODE_ARG(1, Vector4f);
        NODE_RETURN(
            PCent__string_string_vector_floatBSB_4ESB_(NODE_THREAD, fmt, arg));
    }

    NODE_IMPLEMENTATION(StringType::formatOp_Vector3f, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        Vector3f arg = NODE_ARG(1, Vector3f);
        NODE_RETURN(
            PCent__string_string_vector_floatBSB_3ESB_(NODE_THREAD, fmt, arg));
    }

    NODE_IMPLEMENTATION(StringType::formatOp_Vector2f, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        Vector2f arg = NODE_ARG(1, Vector2f);
        NODE_RETURN(
            PCent__string_string_vector_floatBSB_2ESB_(NODE_THREAD, fmt, arg));
    }

    NODE_IMPLEMENTATION(StringType::formatOp_charArray, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        DynamicArray* arg = NODE_ARG_OBJECT(1, DynamicArray);
        NODE_RETURN(PCent__string_string_charBSB_2ESB_(NODE_THREAD, fmt, arg));
    }

    NODE_IMPLEMENTATION(StringType::formatOp_byte, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        int arg = NODE_ARG(1, int);
        // convert to UTF-8
        NODE_RETURN(PCent__string_string_byte(NODE_THREAD, fmt, arg));
    }

    NODE_IMPLEMENTATION(StringType::formatOp_class, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        Pointer arg = NODE_ARG(1, Pointer);
        NODE_RETURN(
            PCent__string_string_QMark_class_not_tuple(NODE_THREAD, fmt, arg));
    }

    NODE_IMPLEMENTATION(StringType::formatOp_opaque, Pointer)
    {
        Pointer fmt = NODE_ARG(0, Pointer);
        Pointer arg = NODE_ARG(1, Pointer);
        NODE_RETURN(PCent__string_string_QMark_opaque(NODE_THREAD, fmt, arg));
    }

    NODE_IMPLEMENTATION(StringType::print, void)
    {
        print_void_string(NODE_THREAD, NODE_ARG(0, Pointer));
    }

    NODE_IMPLEMENTATION(StringType::size, int)
    {
        NODE_RETURN(string_size_int_string(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::hash, int)
    {
        NODE_RETURN(string_hash_int_string(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::compare, int)
    {
        NODE_RETURN(compare_int_string_string(NODE_THREAD, NODE_ARG(0, Pointer),
                                              NODE_ARG(1, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::split, Pointer)
    {
        NODE_RETURN(string_split_stringBSB_ESB__string_string_bool(
            NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
            NODE_ARG(2, bool)));
    }

    NODE_IMPLEMENTATION(StringType::join_array, Pointer)
    {
        NODE_RETURN(string_join_string_stringBSB_ESB__string(
            NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    NODE_IMPLEMENTATION(StringType::index, int)
    {
        NODE_RETURN(string_BSB_ESB__char_string_int(
            NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, int)));
    }

    NODE_IMPLEMENTATION(StringType::substr, Pointer)
    {
        NODE_RETURN(string_substr_string_string_int_int(
            NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int)));
    }

} // namespace Mu

#if defined _MSC_VER
#undef snprintf
#endif
