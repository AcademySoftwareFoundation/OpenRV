#include "ImGuiPythonBridge.h"
#include <Python.h>
#include <iostream>
#include <memory>

namespace Rv
{
    std::vector<ImGuiPythonBridge::PyObjectPtr>* ImGuiPythonBridge::g_callbacks = new std::vector<ImGuiPythonBridge::PyObjectPtr>();

    void ImGuiPythonBridge::PyObjectDeleter::operator()(PyObject* obj) const
    {
        if (obj && Py_IsInitialized())
        {
            PyGILState_STATE gstate = PyGILState_Ensure();
            Py_DECREF(obj);
            PyGILState_Release(gstate);
        }
    }

    void ImGuiPythonBridge::registerCallback(PyObject* callable)
    {
        if (g_callbacks && PyCallable_Check(callable))
        {
            Py_INCREF(callable);
            g_callbacks->emplace_back(callable);
        }
    }

    void ImGuiPythonBridge::unregisterCallback(PyObject* callable)
    {
        if (!g_callbacks)
            return;

        for (auto it = g_callbacks->begin(); it != g_callbacks->end();)
        {
            if (PyObject_RichCompareBool(it->get(), callable, Py_EQ))
            {
                // Deleter will handle Py_DECREF automatically.
                it = g_callbacks->erase(it);
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
        if (!g_callbacks || g_callbacks->empty())
        {
            return;
        }

        for (auto& cb : *g_callbacks)
        {
            PyGILState_STATE gstate = PyGILState_Ensure();
            PyObject* result = PyObject_CallObject(cb.get(), nullptr);
            if (!result && PyErr_Occurred())
                PyErr_Print();
            Py_XDECREF(result);
            PyGILState_Release(gstate);
        }
    }

    int ImGuiPythonBridge::nbCallbacks() { return g_callbacks ? (int)g_callbacks->size() : 0; }

    void ImGuiPythonBridge::init()
    {
        if (!g_callbacks)
            g_callbacks = new std::vector<PyObjectPtr>();
    }

    void ImGuiPythonBridge::shutdown()
    {
        if (!g_callbacks)
            return;

        // Null before delete: during destruction, PyObjectDeleter calls
        // Py_DECREF which can trigger __del__ -> unregisterCallback re-entrancy.
        auto* tmp = g_callbacks;
        g_callbacks = nullptr;
        delete tmp;
    }
} // namespace Rv
