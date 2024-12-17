#ifndef __Mu__Module__h__
#define __Mu__Module__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Symbol.h>
#include <Mu/config.h>
#include <vector>
#include <string>

namespace Mu
{
    class Context;
    class Process;

    //
    //  class Module
    //
    //  Module is a namespace. A module may also keep track of an
    //  associated DSO.
    //

    class Module : public Symbol
    {
    public:
        typedef Module* (*InitFunction)(const char*, Context*, Process*);
        typedef Module* (*FinalizeFunction)(Context*);

        struct DSOInterface
        {
            DSOInterface()
                : _init(0)
                , _finalize(0)
                , _filename("")
            {
            }

            MU_GC_NEW_DELETE

            InitFunction _init;
            FinalizeFunction _finalize;
            String _filename;
            void* _handle;
        };

        typedef STLVector<DSOInterface>::Type DSOModules;

        //
        //	Constructors
        //

        Module(Context* context, const char*);
        virtual ~Module();

        //
        //  Compile at source load time
        //

        static void setCompileOnDemand(bool muc, bool mud);
        static void setDebugArchive(bool);

        //
        //  Location of module (on the file system)
        //

        const String& location() const { return _location; }

        const String& docLocation() const;

        //
        //	Loads a module named Name. Finds any .so files or .muc files
        //	or calls an already loaded .so to generate a new Module.
        //

        static Module* load(Name, Process*, Context*);

        //
        //  Load separate documenation file if it exists. Returns true if
        //  it existed
        //

        bool hasDocFile() const;

        bool loadDocs(Process*, Context*) const;

        bool loadedDocs() const { return _loadedDocs; }

        //
        //	Symbol API
        //

        virtual void output(std::ostream&) const;

        //
        //  Access
        //

        static const DSOModules& dsoModules() { return _dsoModules; }

    private:
        void setDSOInterface(DSOInterface* i) { _dsoInterface = i; }

        const String& findAssociatedDocFile() const;
        static int findDSOModule(const String&);
        static Module* loadDSO(const String&, Name, int, Process*, Context*);
        static Module* loadMUC(const String&, Name, Process*, Context*);
        static Module* loadSource(const String&, Name, Process*, Context*);
        static bool fileOK(const String&);
        // static bool         compileMUC(const String&, Process*, Context*);

    private:
        bool _native;
        DSOInterface* _dsoInterface;
        String _location;
        mutable String _mudfile;
        mutable bool _loadedDocs;
        mutable bool _checkedDocs;
        static bool _compileOnDemand;
        static bool _compileDocs;
        static bool _debugArchive;
        static DSOModules _dsoModules;
    };

} // namespace Mu

#endif // __Mu__Module__h__
