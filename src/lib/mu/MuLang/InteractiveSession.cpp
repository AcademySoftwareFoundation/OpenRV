//
// Copyright (c) 2013, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/InteractiveSession.h>
#include <MuLang/Parse.h>
#include <Mu/NodeAssembler.h>
#include <Mu/Context.h>
#include <Mu/MuProcess.h>
#include <Mu/Thread.h>
#include <Mu/NodeAssembler.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
using namespace std;

namespace
{

    int getline(char* buffer, int max_size)
    {
        int i = 0;

        for (; i < max_size; i++)
        {
            if (!cin.eof() && !cin.fail())
            {
                char c;
                cin.get(c);
                if (c == '\n')
                {
                    buffer[i] = 0;
                    return i;
                }
                else
                {
                    buffer[i] = c;
                }
            }
            else
            {
                break;
            }
        }

        if (i)
        {
            if (i != max_size)
                i--;
            return i;
        }
        else
        {
            return 0;
        }
    }

} // namespace

namespace Mu
{
    using namespace std;

    InteractiveSession::InteractiveSession() {}

    void InteractiveSession::run(Context* context, Process* process,
                                 Thread* thread)
    {
        char temp[1024];
#ifndef MU_USE_READLINE
        int noReadline = 1;
#endif

        while (1)
        {
            if (feof(stdin))
            {
                cout << endl;
                exit(0);
            }

            char* line = 0;

            if (noReadline)
            {
                cout << "mu> " << flush;
                size_t s = getline(temp, 1024);
                if (!s)
                    line = 0;
                else
                    line = temp;
            }
            else
            {
#ifdef MU_USE_READLINE
                line = readline("mu> ");
#endif
            }

            if (!line || !*line)
                continue;

#ifdef MU_USE_READLINE
            if (!noReadline)
                add_history(line);
#endif

            string statement(line);
            statement.push_back(';');
            istringstream str(statement);

            context->setInput(str);
            NodeAssembler assembler(context, process);
            assembler.throwOnError(true);

            try
            {
                if (Process* p = Parse("input", &assembler))
                {
                    if (p->rootNode())
                    {
                        Value v = p->evaluate(thread);
                        const Type* type = thread->returnValueType();

                        if (!thread->uncaughtException()
                            && type != context->voidType()
                            && !type->isTypePattern())
                        {
                            type->output(cout);
                            cout << " => ";
                            type->outputValue(cout, v);
                            cout << endl << flush;
                        }
                    }
                }
                else
                {
                    cout << "nil" << endl;
                }
            }
            catch (...)
            {
                cout << "=> ERROR" << endl;
            }
        }
    }

} // namespace Mu
