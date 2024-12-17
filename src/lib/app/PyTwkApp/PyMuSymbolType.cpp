//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <PyTwkApp/PyMuSymbolType.h>
#include <PyTwkApp/PyEventType.h>

#include <TwkPython/PyLockObject.h>
#include <Python.h>

#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/List.h>
#include <Mu/ListType.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Module.h>
#include <Mu/NodeAssembler.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Signature.h>
#include <Mu/Symbol.h>
#include <Mu/SymbolTable.h>
#include <Mu/SymbolType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/TupleType.h>
#include <Mu/TypeModifier.h>
#include <Mu/Value.h>
#include <Mu/VariantTagType.h>
#include <Mu/VariantType.h>
#include <MuLang/FixedArray.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <MuPy/PyModule.h>
#include <MuTwkApp/EventType.h>
#include <MuTwkApp/MuInterface.h>
#include <boost/algorithm/string.hpp>
#include <half.h>
#include <sstream>
#include <stdexcept>

namespace TwkApp
{
    using namespace std;

    Mu::FunctionObject*
    createFunctionObjectFromPyObject(const Mu::FunctionType* t, PyObject* pyobj)
    {
        PyLockObject locker;
        Mu::MuLangContext* c = (Mu::MuLangContext*)t->context();
        Mu::NodeAssembler as(c);
        const Mu::Signature* sig = t->signature();
        size_t nargs = sig->size() - 1;
        const Mu::Type* rtype = sig->returnType();
        const Mu::Type* ptype = c->findSymbolOfTypeByQualifiedName<Mu::Type>(
            c->internName("python.PyObject"));
        const Mu::Function* C =
            c->findSymbolOfTypeByQualifiedName<Mu::Function>(
                c->internName("python.PyObject_CallObject"));
        assert(C);
        assert(ptype);

        //
        //  Search for a cast function from PyObject to the type we
        //  need. If not found throw out of here.
        //

        string cname = "to_";
        cname += rtype->name().c_str();
        const Mu::Function* Fcast =
            c->findSymbolOfTypeByQualifiedName<Mu::Function>(
                c->internName(cname.c_str()));

        if (Fcast)
        {
            bool found = false;

            for (const Mu::Symbol* s = Fcast->firstOverload(); s;
                 s = s->nextOverload())
            {
                if (const Mu::Function* f =
                        dynamic_cast<const Mu::Function*>(s))
                {
                    if (f->returnType() == rtype && f->numArgs() == 1
                        && f->argType(0) == ptype)
                    {
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
                Fcast = 0;
        }

        if (!Fcast && rtype != c->voidType())
        {
            ostringstream str;
            str << "can't create PyCallable thunk which returns "
                << rtype->fullyQualifiedName();
            throw invalid_argument(str.str().c_str());
        }

        //
        //  Assemble the thunk
        //

        Mu::NodeAssembler::SymbolList params = as.emptySymbolList();
        Mu::FunctionObject* fobj = new Mu::FunctionObject(t);

        for (size_t i = 0; i < nargs; i++)
        {
            ostringstream name;
            name << "_" << i;
            const Mu::Type* argType = sig->argType(i);
            params.push_back(
                new Mu::ParameterVariable(c, name.str().c_str(), argType));
        }

        as.newStackFrame();

        Mu::Function* F = 0;

        if (nargs > 0)
        {
            F = new Mu::Function(c, "__lambda", rtype, nargs,
                                 (Mu::ParameterVariable**)&params.front(), 0,
                                 Mu::Function::ContextDependent
                                     | Mu::Function::LambdaExpression);
        }
        else
        {
            F = new Mu::Function(c, "__lambda", rtype, 0, 0, 0,
                                 Mu::Function::ContextDependent
                                     | Mu::Function::LambdaExpression);
        }

        as.scope()->addAnonymousSymbol(F);
        as.pushScope(F);
        as.declareParameters(params);
        as.removeSymbolList(params);

        // Mu::Node* root = ...

        // Make a constant for the pyobj
        Mu::DataNode* pynode = as.constant(ptype);
        pynode->_data._Pointer = pyobj;
        Mu::Node* argNode = 0;

        if (nargs > 1)
        {
            // make a tuple out of the rest of the args
            Mu::NodeAssembler::NodeList nl = as.emptyNodeList();

            for (size_t i = 0; i < nargs; i++)
            {
                nl.push_back(as.dereferenceVariable(F->parameter(i)));
            }

            Mu::Node* tnode = as.tupleNode(nl);
            as.removeNodeList(nl);
            argNode = tnode;
        }
        else if (nargs == 1)
        {
            argNode = as.dereferenceVariable(F->parameter(0));
        }
        else
        {
            Mu::DataNode* dn = as.constant(t); // t could be anything as long as
                                               // its a Mu::Class
            dn->_data._Pointer = 0;
            argNode = dn;
        }

        Mu::NodeAssembler::NodeList nl = as.newNodeList(pynode);
        nl.push_back(argNode);
        Mu::Node* root = as.callFunction(C, nl);
        as.removeNodeList(nl);

        if (Fcast)
        {
            nl = as.newNodeList(root);
            root = as.callFunction(Fcast, nl);
            as.removeNodeList(nl);
        }

        int stackSize = as.endStackFrame();
        as.popScope();
        F->stackSize(stackSize);

        if (F->hasReturn())
        {
            F->setBody(root);
        }
        else if (Mu::Node* code = as.cast(root, F->returnType()))
        {
            F->setBody(code);
        }
        else
        {
            throw invalid_argument(
                "failed to build closue for PyObject because of bad cast");
        }

        fobj->setFunction(F);
        return fobj;
    }

    static PyObject* MuSymbol_new(PyTypeObject* type, PyObject* args,
                                  PyObject* kwds)
    {
        PyLockObject locker;
        PyMuSymbolObject* self;

        if (self = (PyMuSymbolObject*)type->tp_alloc(type, 0))
        {
            self->symbol = 0;
            self->function = 0;
        }

        return (PyObject*)self;
    }

    int MuSymbol_init(PyObject* _self, PyObject* args, PyObject* kwds)
    {
        PyMuSymbolObject* self = (PyMuSymbolObject*)_self;

        PyLockObject locker;
        const char* name;

        int ok = PyArg_ParseTuple(args, "s", &name);
        if (!ok)
            return -1;

        const Mu::Symbol* symbol = muContext()->findSymbolByQualifiedName(
            muContext()->internName(name), true);

        if (!symbol)
        {
            vector<string> parts;
            boost::algorithm::split(parts, name, boost::is_any_of(string(".")));
            if (parts.size() > 1)
            {
                Mu::Name modname = muContext()->internName(parts[0].c_str());
                Mu::Module::load(modname, muProcess(), muContext());
                symbol = muContext()->findSymbolByQualifiedName(
                    muContext()->internName(name), true);
            }
        }

        if (!symbol)
        {
            ostringstream str;
            str << "Could not find Mu symbol '" << name << "'" << endl;
            PyErr_SetString(PyExc_Exception, str.str().c_str());
            self->symbol = 0;
            self->function = 0;
            return -1;
        }
        else
        {
            self->symbol = symbol;
            self->function = dynamic_cast<const Mu::Function*>(symbol);
        }

        return 0;
    }

    static void MuSymbol_dealloc(PyObject* self)
    {
        PyLockObject locker;
        Py_TYPE(self)->tp_free(self);
    }

    //----------------------------------------------------------------------

    static PyObject* MuSymbol_call(PyObject* _self, PyObject* args,
                                   PyObject* kwds)
    {

        PyLockObject locker;
        PyMuSymbolObject* self = (PyMuSymbolObject*)_self;

        if (!self->function)
        {
            PyErr_SetString(PyExc_Exception,
                            "Mu symbol is not a function -- cannot call");
            return NULL;
        }

        size_t nargs = PyTuple_Size(args);

        Mu::Function::ArgumentVector muargs(self->function->numArgs());
        size_t i = 0;

        //
        //  Check that caller provided an acceptable number of args.
        //
        if (nargs > self->function->numArgs()
            || nargs < self->function->minimumArgs())
        {
            ostringstream str;
            str << "Wrong number of arguments (" << nargs << ") to function "
                << self->function->fullyQualifiedName() << " -- requires "
                << self->function->numArgs() << " or less and at least "
                << self->function->minimumArgs() << endl;

            PyErr_SetString(PyExc_Exception, str.str().c_str());
            return NULL;
        }

        //
        //  Fill in arg values provided by caller.
        //
        try
        {
            for (i = 0; i < nargs; i++)
            {
                muargs[i] = Mu::PyModule::py2mu(muContext(), muProcess(),
                                                self->function->argType(i),
                                                PyTuple_GetItem(args, i));
            }
        }
        catch (std::exception& e)
        {
            ostringstream str;
            str << "Bad argument (" << i << ") to function "
                << self->function->fullyQualifiedName() << ": " << e.what();

            PyErr_SetString(PyExc_TypeError, str.str().c_str());
            return NULL;
        }

        //
        //  Fill in remaining arg values from default values.
        //
        for (i = nargs; i < self->function->numArgs(); ++i)
        {
            const Mu::ParameterVariable* p = self->function->parameter(i);

            if (!p->hasDefaultValue())
            {
                ostringstream str;
                str << "Bad argument (" << i << ") to function "
                    << self->function->fullyQualifiedName() << ": "
                    << p->fullyQualifiedName() << " has no default value";

                PyErr_SetString(PyExc_TypeError, str.str().c_str());
                return NULL;
            }
            muargs[i] = p->defaultValue();
        }

        try
        {
            const Mu::Value v =
                muAppThread()->call(self->function, muargs, false);

            if (muAppThread()->uncaughtException())
            {
                throw true;
            }
            else
            {
                return Mu::PyModule::mu2py(muContext(), muProcess(),
                                           self->function->returnType(), v);
            }
        }
        catch (std::exception& exc)
        {
            ostringstream str;
            str << "Exception thrown while calling "
                << self->function->fullyQualifiedName();

            if (const Mu::Object* o = muAppThread()->exception())
            {
                str << " -- ";
                o->type()->outputValue(str, (Mu::ValuePointer)&o);
            }

            str << ", ";
            str << exc.what();

            if (muContext()->debugging())
            {
                // Printing the error using cerr right away because the
                // exception string does not get displayed. The issue is that
                // PyErr_Occurred does not return true and the printing is
                // skipped in PyStateFunc::state().
                cerr << "ERROR: " << str.str() << endl;
            }

            PyErr_SetString(PyExc_Exception, str.str().c_str());
            return NULL;
        }
        catch (...)
        {
            ostringstream str;
            str << "Exception thrown while calling "
                << self->function->fullyQualifiedName();

            if (const Mu::Object* o = muAppThread()->exception())
            {
                str << " -- ";
                o->type()->outputValue(str, (Mu::ValuePointer)&o);
            }

            PyErr_SetString(PyExc_Exception, str.str().c_str());
            return NULL;
        }
    }

    static PyMethodDef methods[] = {
        {NULL} /* Sentinel */
    };

    static PyTypeObject type = {
        PyVarObject_HEAD_INIT(NULL, 0) "MuSymbol", /*tp_name*/
        sizeof(PyMuSymbolObject),                  /*tp_basicsize*/
        0,                                         /*tp_itemsize*/
        MuSymbol_dealloc,                          /*tp_dealloc*/
        0,                                         /*tp_print*/
        0,                                         /*tp_getattr*/
        0,                                         /*tp_setattr*/
        0,                                         /*tp_compare*/
        0,                                         /*tp_repr*/
        0,                                         /*tp_as_number*/
        0,                                         /*tp_as_sequence*/
        0,                                         /*tp_as_mapping*/
        0,                                         /*tp_hash */
        MuSymbol_call,                             /*tp_call*/
        0,                                         /*tp_str*/
        0,                                         /*tp_getattro*/
        0,                                         /*tp_setattro*/
        0,                                         /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,                        /*tp_flags*/
        "Application Menu Item",                   /* tp_doc */
        0,                                         /* tp_traverse */
        0,                                         /* tp_clear */
        0,                                         /* tp_richcompare */
        0,                                         /* tp_weaklistoffset */
        0,                                         /* tp_iter */
        0,                                         /* tp_iternext */
        methods,                                   /* tp_methods */
        0,                                         /* tp_members */
        0,                                         /* tp_getset */
        0,                                         /* tp_base */
        0,                                         /* tp_dict */
        0,                                         /* tp_descr_get */
        0,                                         /* tp_descr_set */
        0,                                         /* tp_dictoffset */
        MuSymbol_init,                             /* tp_init */
        0,                                         /* tp_alloc */
        MuSymbol_new,                              /* tp_new */
    };

    PyTypeObject* pyMuSymbolType() { return &type; }

    void initPyMuSymbolType()
    {
        //
    }

} // namespace TwkApp
