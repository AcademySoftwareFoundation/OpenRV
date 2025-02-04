//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <PyTwkApp/PyCommands.h>
#include <PyTwkApp/PyFunctionAction.h>
#include <PyTwkApp/PyInterface.h>
#include <PyTwkApp/PyMenuState.h>

#include <TwkPython/PyLockObject.h>
#include <Python.h>

#include <MuPy/PyModule.h>
#include <TwkApp/Action.h>
#include <TwkApp/Document.h>
#include <TwkApp/Event.h>
#include <TwkApp/EventTable.h>
#include <TwkApp/Menu.h>
#include <sstream>

namespace TwkApp
{
    using namespace std;

    static PyObject* badArgument()
    {
        PyErr_SetString(PyExc_Exception, "Bad argument");
        return NULL;
    }

    static Document* currentDocument()
    {
        if (Document* d = Document::eventDocument())
        {
            return d;
        }
        else
        {
            return Document::activeDocument();
        }
    }

    static PyObject* bind(PyObject* self, PyObject* args)
    {
        PyLockObject locker;
        const char* modeName = 0;
        const char* tableName = 0;
        const char* eventName = 0;
        PyObject* callable = 0;
        const char* docString = 0;

        if (!PyArg_ParseTuple(args, "sssOs", &modeName, &tableName, &eventName,
                              &callable, &docString))
            return NULL;

        if (!docString)
            docString = "";
        Document* d = currentDocument();

        if (!d)
        {
            PyErr_SetString(PyExc_Exception, "No active document");
            return NULL;
        }

        if (Mode* mode = d->findModeByName(modeName))
        {
            EventTable* table = mode->findTableByName(tableName);

            if (!table)
            {
                table = new EventTable(tableName);
                mode->addEventTable(table);
            }

            PyFunctionAction* action =
                new PyFunctionAction(callable, docString);
            table->bind(eventName, action);
            //
            //  currentDocument has a copy of this table, so invalidate
            //  that copy.
            //
            d->invalidateEventTables();
        }
        else
        {
            return badArgument();
        }
        // else
        // {
        //     string msg = "No mode named " + string(modeName->c_str());
        //     throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
        // }

        return PyLong_FromLong(1);
    }

    static PyObject* bindRegex(PyObject* self, PyObject* args)
    {
        PyLockObject locker;
        const char* modeName = 0;
        const char* tableName = 0;
        const char* eventName = 0;
        PyObject* callable = 0;
        const char* docString = 0;

        if (!PyArg_ParseTuple(args, "sssOs", &modeName, &tableName, &eventName,
                              &callable, &docString))
            return NULL;

        if (!docString)
            docString = "";
        Document* d = currentDocument();

        if (Mode* mode = d->findModeByName(modeName))
        {
            EventTable* table = mode->findTableByName(tableName);

            if (!table)
            {
                table = new EventTable(tableName);
                mode->addEventTable(table);
            }

            PyFunctionAction* action =
                new PyFunctionAction(callable, docString);
            table->bindRegex(eventName, action);
            //
            //  currentDocument has a copy of this table, so invalidate
            //  that copy.
            //
            d->invalidateEventTables();
        }
        else
        {
            return badArgument();
        }
        // else
        // {
        //     string msg = "No mode named " + string(modeName->c_str());
        //     throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
        // }

        return PyLong_FromLong(1);
    }

    static PyObject* defineModeMenu(PyObject* self, PyObject* args)
    {
        PyLockObject locker;
        PyObject* listobj;
        const char* modeName;
        int strict = 0;

        int ok = PyArg_ParseTuple(args, "sO!|i", &modeName, &PyList_Type,
                                  &listobj, &strict);
        if (!ok)
            return NULL;

        Document* d = currentDocument();
        Menu* menu = pyListToMenu("Main", listobj);

        if (Mode* m = d->findModeByName(modeName))
        {
            if (strict)
                m->setMenu(menu);
            else
                m->merge(menu);

            //
            //  If we are re-defining an existing menu, we need to invalidate
            //  the old menu.
            //
            d->invalidateMenu();
        }
        else
        {
            return badArgument();
        }

        return PyLong_FromLong(1);
    }

    static vector<PyMethodDef> methods;

    static PyMethodDef localmethods[] = {
        {"bind", bind, METH_VARARGS, "bind event to action."},
        {"bindRegex", bindRegex, METH_VARARGS, "bind regex event to action."},
        {"defineModeMenu", defineModeMenu, METH_VARARGS,
         "define the menu for a mode."},
        {NULL}};

    void pyInitCommands(void* othermethods)
    {
        for (size_t i = 0; localmethods[i].ml_name; i++)
            methods.push_back(localmethods[i]);

        if (othermethods)
        {
            for (PyMethodDef* d = (PyMethodDef*)othermethods; d->ml_name; d++)
                methods.push_back(*d);
        }

        PyMethodDef last = {NULL};
        methods.push_back(last);

        PyObject* rvObj = PyImport_ImportModule("rv");
        PyObject* cmdObj = PyImport_AddModule("rv.commands");

        if (!rvObj)
            cerr << "ERROR: Module 'rv' could not be imported" << endl;
        if (!cmdObj)
            cerr << "ERROR: Module 'rv.commands' could not be imported" << endl;

        if (rvObj && cmdObj)
        {
            PyModule_AddFunctions(cmdObj, &methods.front());

            PyModule_AddObject(rvObj, "commands", cmdObj);
            Py_XINCREF(cmdObj);
        }
    }

} // namespace TwkApp
