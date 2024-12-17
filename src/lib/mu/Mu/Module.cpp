//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Module.h>
#include <Mu/Archive.h>
#include <Mu/Environment.h>
#include <Mu/Context.h>
#include <Mu/UTF8.h>
#include <Mu/Exception.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef _MSC_VER
#include <windows.h>
#else
#define HAVE_DLOPEN
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace Mu
{
    using namespace std;

    Module::DSOModules Module::_dsoModules;
    bool Module::_compileOnDemand = false;
    bool Module::_compileDocs = false;
    bool Module::_debugArchive = false;

    Module::Module(Context* context, const char* name)
        : Symbol(context, name)
        , _native(false)
        , _loadedDocs(false)
        , _checkedDocs(false)
    {
        _searchable = true;
    }

    Module::~Module() {}

    void Module::setCompileOnDemand(bool muc, bool mud)
    {
        _compileOnDemand = muc;
        _compileDocs = mud;
    }

    void Module::setDebugArchive(bool b) { _debugArchive = b; }

    int Module::findDSOModule(const String& filename)
    {
        for (int i = 0; i < Module::_dsoModules.size(); i++)
        {
            const DSOInterface& m = Module::_dsoModules[i];
            if (m._filename == filename)
                return i;
        }

        return -1;
    }

    Module* Module::loadDSO(const String& soName, Name name, int i,
                            Process* process, Context* context)
    {
        DSOInterface dsoInterface;
        int index;
        String initName = "MuInitialize";

        if ((index = findDSOModule(soName)) != -1)
        {
            DSOInterface& di = _dsoModules[index];
            Context::PrimaryBit fence(context, false);

            if (Module* m = (*di._init)(name.c_str(), context, process))
            {
                return m;
            }

            cerr << "WARNING: error intializing already loaded compiled module "
                 << soName << endl;
        }

#ifdef HAVE_DLOPEN
        if (dsoInterface._handle =
                dlopen(soName.c_str(), RTLD_NOW | RTLD_GLOBAL))
        {
            if (void* sym = dlsym(dsoInterface._handle, initName.c_str()))
#if 0
            }
#endif
#else
        UTF16String uSoName = UTF16convert(soName);
        // UTF16String uInitName = UTF16convert(initName);

        if (dsoInterface._handle = LoadLibrary((LPCTSTR)uSoName.c_str()))
        {
            if (void* sym = GetProcAddress((HMODULE)dsoInterface._handle,
                                           initName.c_str()))
#endif
            {
                dsoInterface._init = (InitFunction)sym;
                Context::PrimaryBit fence(context, false);
                Context::SourceFileScope fileScope(context,
                                                   context->internName(soName));

                if (Module* m =
                        (*dsoInterface._init)(name.c_str(), context, process))
                {
                    dsoInterface._filename = soName;
                    _dsoModules.push_back(dsoInterface);
                    m->_location = soName;
                    return m;
                }

                cerr << "WARNING: unable to intialize compiled module "
                     << soName << endl;
            }
            else
            {
                cerr << "WARING: there is a bogus compiled module at " << soName
                     << endl;
#ifndef PLATFORM_WINDOWS
                cerr << dlerror() << endl;
#endif
            }
        }
        else
        {
            cerr << "ERROR trying to open " << soName << endl;
#ifndef PLATFORM_WINDOWS
            cerr << dlerror() << endl;
#endif
        }

        return 0;
    }

    Module* Module::loadMUC(const String& mucName, Name name, Process* process,
                            Context* context)
    {
        Archive::Reader reader(process, context);
        reader.setDebugOutput(_debugArchive);

        ifstream infile(UNICODE_C_STR(mucName.c_str()), ios_base::binary);

        if (infile)
        {
            try
            {
                Environment::PathComponents path;
                Environment::pathComponents(mucName, path);
                Context::SourceFileScope fileScope(
                    context, context->internName(mucName));

                // cout << "INFO: reading muc file " << mucName << endl;
                reader.read(infile);
                // cout << "INFO: done reading muc file " << mucName << endl;
                const Archive::Modules& modules = reader.modules();
                Module* mainModule = 0;

                for (size_t i = 0; i < modules.size(); i++)
                {
                    Module* m = const_cast<Module*>(modules[i]);
                    m->_location = mucName;

                    String mname = m->name();
                    String::size_type p = mucName.rfind(mname);

                    if (p + mname.size() + 4 == mucName.size())
                    {
                        mainModule = m;
                    }
                }

                if (!mainModule)
                    mainModule = new Module(context, name.c_str());

                return mainModule;
            }
            catch (std::exception& e)
            {
                cerr << "ERROR: while loading muc module \"" << mucName << "\""
                     << endl;
                cerr << "ERROR: " << e.what() << endl;
            }
            catch (...)
            {
                cerr << "ERROR: while loading muc module \"" << mucName << "\""
                     << endl;
                cerr << "ERROR: unknown exception" << endl;
            }

            context->popSourceRecordStack();
        }

        cerr << "ERROR: failed to load muc file " << mucName << endl;
        return 0;
    }

    Module* Module::loadSource(const String& muName, Name name,
                               Process* process, Context* context)
    {
        Context::ModuleList modules;

        try
        {
            Context::SourceFileScope fileScope(context,
                                               context->internName(muName));
            context->evalFile(muName.c_str(), process, modules);
        }
        catch (TypedValue& val)
        {
            cerr << "ERROR: while loading source module \"" << muName << "\""
                 << endl;
            cerr << "ERROR: caught ";

            if (val._type)
            {
                val._type->output(cerr);
                cerr << ": ";
                Object* o = reinterpret_cast<Object*>(val._value._Pointer);
                val._type->outputValue(cerr, val._value);
            }
            else
            {
                cerr << "unknown internal exception";
            }

            cerr << endl;
        }
        catch (std::exception& e)
        {
            cerr << "ERROR: while loading source module \"" << muName << "\""
                 << endl;
            cerr << "ERROR: " << e.what() << endl;
        }
        catch (...)
        {
            cerr << "ERROR: while loading source module \"" << muName << "\""
                 << endl;
            cerr << "ERROR: unknown exception" << endl;
        }

        Module* m =
            context->globalScope()->findSymbolOfTypeByQualifiedName<Module>(
                name);
        if (m)
            m->_location = muName;
        return m;
    }

    const String& Module::findAssociatedDocFile() const
    {
        if (_checkedDocs)
            return _mudfile;

        //
        //  Check for similarily named .mud file exists
        //

        String base;

        if (_location != "")
        {
            size_t pos = _location.rfind('.');

            if (pos != string::npos)
            {
                base = _location.substr(0, pos);
                String filename = base + ".mud";
                if (fileOK(filename))
                    _mudfile = filename;
            }
        }

        if (_mudfile == "")
        {
#ifdef _MSC_VER
            STLVector<String>::Type path = Environment::modulePath();
#else
            Environment::SearchPath path = Environment::modulePath();
#endif

            for (int i = 0; i < path.size(); i++)
            {
                String testpath = path[i];
                if (testpath[testpath.size() - 1] != '/')
                    testpath += "/";

                String mudName = testpath + name().c_str() + ".mud";

                if (fileOK(mudName))
                {
                    _mudfile = mudName;
                    break;
                }
            }
        }

        _checkedDocs = true;
        return _mudfile;
    }

    const String& Module::docLocation() const
    {
        Module* m = const_cast<Module*>(this);
        hasDocFile();
        return _mudfile;
    }

    bool Module::hasDocFile() const
    {
        if (_loadedDocs)
            return true;
        if (!_checkedDocs)
            findAssociatedDocFile();
        return _mudfile != "";
    }

    bool Module::loadDocs(Process* process, Context* context) const
    {
        if (_loadedDocs)
            return true;
        if (!_checkedDocs)
            findAssociatedDocFile();
        if (_mudfile == "")
            return false;

        try
        {
            cout << "INFO: loading " << _mudfile << endl;
            context->parseFile(process, _mudfile.c_str());
        }
        catch (...)
        {
        }

        _loadedDocs = true;

        return _loadedDocs;
    }

    bool Module::fileOK(const String& filename)
    {
#if defined(_MSC_VER)
        // cout << "INFO: " << __FUNCTION__ << ": testing " << filename << endl;
        ifstream tfile(UNICODE_C_STR(filename.c_str()));
        return bool(tfile) == true;
#else
        return access(filename.c_str(), R_OK) != -1;
#endif
    }

    Module* Module::load(Name name, Process* process, Context* context)
    {
        if (Module* m =
                context->findSymbolOfTypeByQualifiedName<Module>(name, true))
        {
            //
            //  If the module is already defined, return it. Loading an
            //  identically named module on top of an existing one can
            //  lead only to evil.
            //

            return m;
        }

#ifdef _MSC_VER
        STLVector<String>::Type path = Environment::modulePath();
#else
        Environment::SearchPath path = Environment::modulePath();
#endif

        for (int i = 0; i < path.size(); i++)
        {
            String testpath = path[i];
            if (testpath[testpath.size() - 1] != '/')
                testpath += "/";

            String soName = testpath + name.c_str() + ".so";
            String mucName = testpath + name.c_str() + ".muc";
            String muName = testpath + name.c_str() + ".mu";
            String mudName = testpath + name.c_str() + ".mud";

            if (fileOK(soName))
            {
                if (Module* m = loadDSO(soName, name, i, process, context))
                {
                    return m;
                }
            }
            else if (fileOK(mucName))
            {
                if (Module* m = loadMUC(mucName, name, process, context))
                {
                    return m;
                }
            }
            else if (fileOK(muName))
            {
                if (_compileOnDemand)
                {
                    context->debugging(true);
                    Context::PrimaryBit pstate(context, true);

                    if (Module* m = loadSource(muName, name, process, context))
                    {
                        Archive::Writer writer(process, context);
                        Archive::SymbolVector symbols;
                        Archive::Names names;
                        writer.setDebugOutput(_debugArchive);
                        writer.setAnnotationOutput(true);
                        writer.collectSymbolsFromFile(
                            context->internName(muName), symbols);
                        writer.collectNames(symbols, names);
                        writer.add(symbols);
                        writer.add(names);

                        ofstream file(mucName.c_str(), ios_base::binary);

                        if (!file)
                        {
                            cerr << "ERROR: Unable to open output file" << endl;
                            throw FileOpenErrorException();
                        }

                        writer.write(file);
                        cout << "INFO: compiled " << mucName << endl;
                        file.close();

                        if (_compileDocs)
                        {
                            //
                            //  Write to memory first then the file. This
                            //  way an existing mud file of the same name
                            //  will load first then its contents will be
                            //  merged into the output mud file.
                            //

                            stringstream str;

                            if (writer.writeDocumentation(str) > 0)
                            {
                                ofstream dfile(mudName.c_str(),
                                               ios_base::binary);
                                dfile << str.str();
                                cout << "INFO: compiled " << mudName << endl;
                            }
                        }

                        return m;
                    }
                }
                else
                {
                    if (Module* m = loadSource(muName, name, process, context))
                    {
                        return m;
                    }
                }
            }
        }

        return 0;
    }

    void Module::output(ostream& o) const { Symbol::output(o); }

#if 0
bool
Module::compileMUC(const String& sourceFile, Process* process, Context* context)
{
    String mucFile = sourceFile + "c";

    Archive::SymbolVector symbols;
    Archive::Names names;
    Archive::Writer writer(process, context);
    writer.setDebugOutput(_debugArchive);
    writer.setAnnotationOutput(true);
    writer.collectPrimarySymbols(context->globalScope(), symbols);
    writer.collectNames(symbols, names);
    writer.add(symbols);
    writer.add(names);

    ofstream file(mucFile.c_str(), ios_base::binary);

    if (!file)
    {
        cerr << "ERROR: Unable to open output file " << mucFile;
        return false;
    }

    writer.write(file);

    for (size_t i = 0; i < symbols.size(); i++)
    {
        symbols[i]->_primary = false;
    }

    cout << "INFO: compiled " << sourceFile << endl;

    return true;
}
#endif

} // namespace Mu
