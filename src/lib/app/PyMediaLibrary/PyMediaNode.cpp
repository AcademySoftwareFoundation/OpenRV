//
//  Copyright (c) 2023 Autodesk, Inc. All Rights Reserved.
//  SPDX-License-Identifier: Apache-2.0
//

#include <PyMediaLibrary/PyMediaNode.h>
#include <PyMediaLibrary/PyMediaLibrary.h>
#include <PyMediaLibrary/PyNode.h>

#include <TwkMediaLibrary/Library.h>

#include <TwkPython/PyLockObject.h>

#include <Python.h>

#include <utility>

namespace TwkMediaLibrary
{
    PyMediaNode::PyMediaNode(Library* lib, URL url, PyNode* parent,
                             Plugin plugin, std::string pluginName)
        : PyNode(lib, parent, std::move(pluginName), PyNodeType::PyMediaType)
        , MediaAPI()
        , m_url(std::move(url))
        , m_plugin(plugin)
    {
        Py_XINCREF(m_plugin);
    }

    PyMediaNode::~PyMediaNode() { Py_XDECREF(m_plugin); }

    const Node* PyMediaNode::node() const { return this; }

    URL PyMediaNode::mediaURL() const { return m_url; }

    bool PyMediaNode::isStreaming() const
    {
        PyLockObject lock;

        PyObject* ret =
            PyObject_CallMethod(m_plugin, "is_streaming", "s", m_url.c_str());

        if (ret == nullptr)
        {
            PyErr_Print();
            return false;
        }

        return PyObject_IsTrue(ret);
    }

    bool PyMediaNode::isRedirecting() const
    {
        PyLockObject lock;

        PyObject* ret =
            PyObject_CallMethod(m_plugin, "is_redirecting", "s", m_url.c_str());

        if (ret == nullptr)
        {
            PyErr_Print();
            return false;
        }

        return PyObject_IsTrue(ret);
    }

    const Node* PyMediaNode::baseMediaNode() const { return this; }

    HTTPCookieVector PyMediaNode::httpCookies() const
    {
        PyLockObject lock;

        HTTPCookieVector cookies;

        PyObject* ret = PyObject_CallMethod(m_plugin, "get_http_cookies", "s",
                                            m_url.c_str());
        if (ret == nullptr)
        {
            PyErr_Print();
        }

        if (PyIter_Check(ret))
        {
            PyObject* cookieDict;
            while ((cookieDict = PyIter_Next(ret)))
            {
                if (PyDict_Check(cookieDict))
                {
                    // PyDict_GetItemString returns a borrowed reference, no
                    // need
                    PyObject* nameObj =
                        PyDict_GetItemString(cookieDict, "name");
                    PyObject* valueObj =
                        PyDict_GetItemString(cookieDict, "value");
                    PyObject* domainObj =
                        PyDict_GetItemString(cookieDict, "domain");
                    PyObject* pathObj =
                        PyDict_GetItemString(cookieDict, "path");

                    std::string name =
                        nameObj != nullptr ? PyUnicode_AsUTF8(nameObj) : "";
                    std::string value =
                        valueObj != nullptr ? PyUnicode_AsUTF8(valueObj) : "";
                    std::string domain =
                        domainObj != nullptr ? PyUnicode_AsUTF8(domainObj) : "";
                    std::string path =
                        pathObj != nullptr ? PyUnicode_AsUTF8(pathObj) : "";

                    cookies.emplace_back(domain, path, name, value);

                    // PyDict_GetItemString returns a borrowed reference, no
                    // need to decref

                    // Py_XDECREF(keyObj);
                    // Py_XDECREF(valueObj);
                    // Py_XDECREF(domainObj);
                    // Py_XDECREF(pathObj);
                }

                Py_XDECREF(cookieDict);
            }

            if (PyErr_Occurred())
            {
                PyErr_Print();
            }
        }

        Py_XDECREF(ret);

        return cookies;
    }

    HTTPHeaderVector PyMediaNode::httpHeaders() const
    {
        PyLockObject lock;

        HTTPHeaderVector headers;

        PyObject* ret = PyObject_CallMethod(m_plugin, "get_http_headers", "s",
                                            m_url.c_str());
        if (ret == nullptr)
        {
            PyErr_Print();
        }

        if (PyIter_Check(ret))
        {
            PyObject* cookieDict;
            while ((cookieDict = PyIter_Next(ret)))
            {
                if (PyDict_Check(cookieDict))
                {
                    // PyDict_GetItemString returns a borrowed reference, no
                    // need
                    PyObject* nameObj =
                        PyDict_GetItemString(cookieDict, "name");
                    PyObject* valueObj =
                        PyDict_GetItemString(cookieDict, "value");

                    std::string name =
                        nameObj != nullptr ? PyUnicode_AsUTF8(nameObj) : "";
                    std::string value =
                        valueObj != nullptr ? PyUnicode_AsUTF8(valueObj) : "";

                    headers.emplace_back(name, value);

                    // PyDict_GetItemString returns a borrowed reference, no
                    // need to decref
                    //
                    // Py_XDECREF(nameObj);
                    // Py_XDECREF(valueObj);
                }

                Py_XDECREF(cookieDict);
            }
        }

        return headers;
    }

    URL PyMediaNode::httpRedirection() const
    {
        PyLockObject lock;

        PyObject* ret = PyObject_CallMethod(m_plugin, "get_http_redirection",
                                            "s", m_url.c_str());

        if (ret == nullptr)
        {
            PyErr_Print();
            return mediaURL();
        }

        std::string redirection = PyUnicode_AsUTF8(ret);

        Py_XDECREF(ret);

        return redirection;
    }

} // namespace TwkMediaLibrary
