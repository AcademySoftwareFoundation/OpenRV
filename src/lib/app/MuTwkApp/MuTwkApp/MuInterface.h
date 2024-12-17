//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MuTwkApp__MuInterface__h__
#define __MuTwkApp__MuInterface__h__
#include <MuLang/MuLangContext.h>
#include <Mu/MuProcess.h>
#include <MuLang/DynamicArray.h>

namespace TwkApp
{
    class Menu;
    class Document;

    class CallEnv : public Mu::CallEnvironment
    {
    public:
        CallEnv(Document* d);
        virtual ~CallEnv();
        virtual const Mu::Value call(const Mu::Function*,
                                     Mu::Function::ArgumentVector&) const;
        virtual const Mu::Value
        callMethodByName(const char*, Mu::Function::ArgumentVector&) const;
        virtual const Mu::Context* context() const;

        void invalidate() { _doc = 0; }

    private:
        Document* _doc;
    };

    Mu::Context::ModuleList& muModuleList();
    Mu::MuLangContext* muContext();
    Mu::Process* muProcess();
    Mu::Thread* muAppThread();

    typedef int (*GCFilterFunc)(const char*, void*, size_t);

    void setDebugging(bool);
    void setCompileOnDemand(bool);
    void setDebugMUC(bool);
    bool isDebuggingOn();

    std::string muEval(Mu::MuLangContext*, Mu::Process*,
                       const Mu::Context::ModuleList& modules, const char* line,
                       const char* contextName = "eval", bool showType = true);

    std::string muEvalStringExpr(Mu::MuLangContext*, Mu::Process*,
                                 const Mu::Context::ModuleList& modules,
                                 const char* line,
                                 const char* contextName = "eval");
    void cli();

    void batch(Mu::MuLangContext*, Mu::Process*, const Mu::Context::ModuleList&,
               const char* filename);

    void initMu(const char* batchfile, GCFilterFunc F = 0);

    void initWithFile(Mu::MuLangContext*, Mu::Process*,
                      const Mu::Context::ModuleList&, const char* filename);

    void initWithString(Mu::MuLangContext*, Mu::Process*,
                        const Mu::Context::ModuleList&, const char* buffer);

    void initRc(Mu::MuLangContext*, Mu::Process*,
                const Mu::Context::ModuleList&, const char* rcfile);

    TwkApp::Menu* createTwkAppMenu(const std::string& name,
                                   Mu::DynamicArray* array);

    //
    //  For multi-threaded, etc, you can create a unique app context for
    //  each thread by calling this. batchfile and gc_filter can be 0
    //

    Mu::MuLangContext* newMuContext(const char* batchFile,
                                    GCFilterFunc gc_filter,
                                    Mu::Context::ModuleList& modules);

    void runMuInterative();

} // namespace TwkApp

#endif // __MuTwkApp__MuInterface__h__
