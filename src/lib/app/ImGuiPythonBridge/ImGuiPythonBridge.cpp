#include "ImGuiPythonBridge.h"
#include <Python.h>
#include <iostream>
#include <memory>

namespace Rv
{
    static bool s_shuttingDown = false;

    void ImGuiPythonBridge::PyObjectDeleter::operator()(PyObject* obj) const
    {
        if (obj && !s_shuttingDown && Py_IsInitialized())
        {
            Py_DECREF(obj);
        }
    }

    std::vector<ImGuiPythonBridge::PyObjectPtr> ImGuiPythonBridge::s_callbacks;

    void ImGuiPythonBridge::registerCallback(PyObject* callable)
    {
        if (PyCallable_Check(callable))
        {
            Py_INCREF(callable);
            s_callbacks.emplace_back(callable);
        }
    }

    void ImGuiPythonBridge::unregisterCallback(PyObject* callable)
    {
        for (auto it = s_callbacks.begin(); it != s_callbacks.end();)
        {
            if (PyObject_RichCompareBool(it->get(), callable, Py_EQ))
            {
                // Deleter will handle Py_DECREF automatically.
                it = s_callbacks.erase(it); 
                return;
            }
            else
            {
                ++it;
            }
        }
    }

    void ImGuiPythonBridge::callCallbacks()
    {
        // Early exit optimization - avoid Python overhead if no callbacks registered
        if (s_callbacks.empty())
        {
            return;
        }
        
        for (auto& cb : s_callbacks)
        {
            PyGILState_STATE gstate = PyGILState_Ensure();
            PyObject* result = PyObject_CallObject(cb.get(), nullptr);
            if (!result && PyErr_Occurred())
                PyErr_Print();
            Py_XDECREF(result);
            PyGILState_Release(gstate);
        }
    }

    int ImGuiPythonBridge::nbCallbacks() { return (int)s_callbacks.size(); }

    void ImGuiPythonBridge::clearCallbacks()
    {
        s_shuttingDown = true;
        
        // During shutdown, we cannot safely call Py_DECREF, so we just release
        // the smart pointers without calling their deleters and let Python
        // handle cleanup during interpreter shutdown.
        for (auto& ptr : s_callbacks)
        {
            if (ptr)
            {
                // Release without calling Py_DECREF.
                ptr.release(); 
            }
        }
        
        s_callbacks.clear();
    }
} // namespace Rv
