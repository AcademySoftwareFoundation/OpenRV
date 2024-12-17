#ifndef __MuLang__MuLangLanguage__h__
#define __MuLang__MuLangLanguage__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Language.h>
#include <Mu/MuProcess.h>
#include <iosfwd>

namespace Mu
{

    //
    //  class MuLangLanguage
    //
    //  Implementation of the language class for MuLang
    //

    class MuLangLanguage : public Language
    {
    public:
        MuLangLanguage();
        ~MuLangLanguage();

        //
        //	Parses cin, returns Process object for evaluation.
        //

        Process* parseStdin(Context*, Process*, const char* name = 0);

        //
        //	Parses file filename and returns Process object for evaluation.
        //

        Process* parseFile(Context*, Process*, const char* fileName);

        //
        //	Parses a stream
        //

        Process* parseStream(Context*, Process*, std::istream&, const char*);
    };

} // namespace Mu

#endif // __MuLang__MuLangLanguage__h__
