//
//  Copyright (c) 2023 Autodesk, Inc. All Rights Reserved.
//  SPDX-License-Identifier: Apache-2.0
//

#include <PyMediaLibrary/PyRootNode.h>
#include <PyMediaLibrary/PyMediaNode.h>
#include <PyMediaLibrary/PyNode.h>

#include <TwkPython/PyLockObject.h>

#include <TwkUtil/File.h>
#include <TwkUtil/EnvVar.h>

#include <Python.h>

#include <array>
#include <string_view>

namespace
{
    static ENVVAR_BOOL(evUseMediaLibraryPlugins, "RV_USE_MEDIA_LIBRARY_PLUGINS",
                       true);

    constexpr std::array<std::string_view, 7> PLUGIN_FUNCTIONS = {
        {{"is_plugin_enabled"},
         {"is_library_media_url"},
         {"is_streaming"},
         {"is_redirecting"},
         {"get_http_cookies"},
         {"get_http_headers"},
         {"get_http_redirection"}}};

    using Plugin = PyObject*;
    using PluginVector = std::vector<std::pair<std::string, Plugin>>;

    PluginVector plugins;
    bool pluginsLoaded = !evUseMediaLibraryPlugins.getValue();

    void loadPlugins()
    {
        const char* pluginPathsEnv = getenv("TWK_MEDIA_LIBRARY_PLUGIN_PATH");
        if (pluginPathsEnv == nullptr)
        {
            return;
        }

        std::set<std::string> pluginNames;
        PyLockObject lock;

        const TwkUtil::FileNameList pluginPackages =
            TwkUtil::findInPath(".*", pluginPathsEnv);

        for (const auto& pluginPackage : pluginPackages)
        {
            const TwkUtil::FileNameList pluginFiles =
                TwkUtil::findInPath(".*\\.py", pluginPackage);

            for (const auto& pluginFile : pluginFiles)
            {
                const std::string pluginImportPath =
                    TwkUtil::basename(TwkUtil::dirname(pluginFile)) + "."
                    + TwkUtil::prefix(pluginFile);

                if (pluginNames.find(pluginImportPath) != pluginNames.end())
                {
                    std::cerr << "Plug-In " << pluginImportPath << " ("
                              << pluginFile
                              << ") already loaded with the same name. Skipped."
                              << std::endl;
                    continue;
                }

                std::cout << "Importing MediaLibrary Plug-In: "
                          << pluginImportPath << std::endl;

                Plugin pluginObj =
                    PyImport_ImportModule(pluginImportPath.c_str());
                if (pluginObj)
                {
                    bool valid = true;
                    for (const auto& func : PLUGIN_FUNCTIONS)
                    {
                        valid &= PyObject_HasAttrString(pluginObj, func.data());

                        if (!valid)
                        {
                            std::cerr << "Plug-In " << pluginImportPath << "("
                                      << pluginFile << ") does not have "
                                      << func.data() << " function. Skipped."
                                      << std::endl;
                            break;
                        }
                    }

                    if (!valid)
                    {
                        Py_XDECREF(pluginObj);
                        continue;
                    }

                    PyObject* enabledObj = PyObject_CallMethod(
                        pluginObj, "is_plugin_enabled", nullptr);

                    if (enabledObj == nullptr)
                    {
                        PyErr_Print();
                    }

                    if (PyObject_IsTrue(enabledObj))
                    {
                        pluginNames.insert(pluginImportPath);
                        plugins.emplace_back(pluginImportPath, pluginObj);
                    }
                    else
                    {
                        Py_XDECREF(pluginObj);
                    }

                    Py_XDECREF(enabledObj);
                }
                else
                {
                    std::cerr << "Failed to load " << pluginImportPath << "("
                              << pluginFile << ")" << std::endl;
                    PyErr_Print();
                }
            }
        }
        pluginsLoaded = true;
    }

    void unloadPlugins()
    {
        if (!pluginsLoaded || plugins.empty())
        {
            return;
        }

        PyLockObject lock;
        for (auto& pluginObjContainer : plugins)
        {
            Py_XDECREF(pluginObjContainer.second);
        }
        plugins.clear();
    }

} // namespace

namespace TwkMediaLibrary
{
    class PyMediaLibrary;

    PyRootNode::PyRootNode(Library* lib)
        : PyNode(lib, nullptr, "", PyNodeType::PyRootType)
    {
    }

    PyRootNode::~PyRootNode() { unloadPlugins(); }

    bool PyRootNode::isLibraryMediaURL(const URL& url) const
    {
        return !mediaNodesForURL(url).empty();
    }

    MediaNodeVector PyRootNode::mediaNodesForURL(const URL& url) const
    {
        MediaNodeVector mediaNodes;

        if (!pluginsLoaded)
        {
            loadPlugins();
        }

        if (plugins.empty())
        {
            return mediaNodes;
        }

        {
            PyLockObject lock;

            for (const auto& pluginContainer : plugins)
            {
                const auto& pluginName = pluginContainer.first;
                const auto& plugin = pluginContainer.second;

                PyObject* ret = PyObject_CallMethod(
                    plugin, "is_library_media_url", "s", url.c_str());
                if (ret == nullptr)
                {
                    PyErr_Print();
                    continue;
                }

                if (PyObject_IsTrue(ret))
                {
                    mediaNodes.push_back(new PyMediaNode(
                        m_library, url, (PyNode*)this, plugin, pluginName));
                }

                Py_XDECREF(ret);
            }
        }

        return mediaNodes;
    }
} // namespace TwkMediaLibrary
