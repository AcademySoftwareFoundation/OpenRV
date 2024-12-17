//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Exception.h>
#include <Mu/Thread.h>
#include <Mu/ParameterVariable.h>
#include <sstream>

namespace Mu
{
    using namespace std;

    Exception::Exception(const char* m, Object* o) throw()
        : std::exception()
        , _message(m)
        , _object(o)
    {
    }

    Exception::Exception(Thread& thread, const char* m, Object* o) throw()
        : std::exception()
        , _message(m)
        , _object(o)
    {
        thread.backtrace(_backtrace);
    }

    Exception::~Exception() throw() {}

    const char* Exception::what() const throw() { return _message.c_str(); }

    String Exception::backtraceAsString() const
    {
        return backtraceAsString(_backtrace);
    }

    String Exception::backtraceAsString(const BackTrace& bt)
    {
        ostringstream cstr;

        int stackcount = 0;

        if (bt.empty())
        {
            cstr << "no backtrace available";
        }

        for (int i = 0; i < bt.size(); i++)
        {
            const Node* n = bt[i].node;
            const Symbol* s = bt[i].symbol;
            const Function* f = dynamic_cast<const Function*>(s);

            if (bt[i].filename && *bt[i].filename && bt[i].linenum)
            {
                cstr << bt[i].filename << ", line " << bt[i].linenum
                     << ", char " << bt[i].charnum << ":" << endl;
            }

            if (i < 100)
                cstr << " ";
            if (i < 10)
                cstr << " ";
            cstr << i << ": ";

            s->outputNode(cstr, n);

            cstr << endl;

            string sname = s->name();
            if (sname.size() > 2 && sname[0] == '_' && sname[1] == '_')
                continue;

            if (f && f->returnType()->isTypePattern())
            {
                cstr << "    returns: " << n->type()->fullyQualifiedName()
                     << endl;
            }

            for (int q = 0, s = n->numArgs(); q < s; q++)
            {
                const Node* p = n->argNode(q);
                cstr << "     ";
                if (q < 10)
                    cstr << " ";
                cstr << q << ": ";

                cstr << p->type()->fullyQualifiedName() << " ";

                if (f && f->hasParameters())
                {
                    cstr << f->parameter(q)->name() << " ";
                }

                if (const Type* t = dynamic_cast<const Type*>(p->symbol()))
                {
                    DataNode* dn = (DataNode*)p;
                    cstr << "=> constant ";
                    t->outputValue(cstr, dn->_data);
                }

                cstr << endl;
            }
        }

        string temp = cstr.str();
        return String(temp.c_str());
    }

} // namespace Mu
