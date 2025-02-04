//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <iostream>
#include <fstream>
#include <MuLang/MuLangLanguage.h>
#include <MuLang/Parse.h>
#include <Mu/NodeAssembler.h>
#include <Mu/Context.h>

namespace Mu
{
    using namespace std;

    MuLangLanguage::MuLangLanguage()
        : Language("MuLang", ".") {};

    MuLangLanguage::~MuLangLanguage() {}

    Process* MuLangLanguage::parseStream(Context* context, Process* process,
                                         istream& s, const char* fileName)
    {
        context->setInput(s);

        NodeAssembler assembler(context,
                                process ? process : new Process(context));

        return Parse(fileName, &assembler);
    }

    Process* MuLangLanguage::parseFile(Context* context, Process* process,
                                       const char* fileName)
    {
        ifstream in(UNICODE_C_STR(fileName));

        if (in)
        {
            return parseStream(context, process, in, fileName);
        }
        else
        {
            context->errorStream() << "Unable to open file " << fileName << endl
                                   << flush;
        }

        return 0;
    }

    Process* MuLangLanguage::parseStdin(Context* context, Process* process,
                                        const char* name)
    {
        if (!name)
            name = "Standard Input";
        return parseStream(context, process, cin, name);
    }

} // namespace Mu
