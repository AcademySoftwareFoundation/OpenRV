#include "ImGuiPythonBridge.h"
#include <Python.h>
#include <iostream>
#include <memory>

namespace Rv
{
    void ImGuiPythonBridge::PyObjectDeleter::operator()(PyObject* obj) const
    {
        if (obj)
            Py_DECREF(obj);
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
                it = s_callbacks.erase(it);
                Py_DECREF(callable);
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
} // namespace Rv
