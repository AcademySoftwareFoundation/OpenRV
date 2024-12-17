//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <memory>
#include <string>
#include <unordered_set>

namespace TwkApp
{

    class OutputPlugin;
    using OutputPluginPtr = std::shared_ptr<TwkApp::OutputPlugin>;

    //
    //  OutputPlugins
    //
    //  Manages the audio/video output plugins
    //

    class OutputPlugins
    {
    public:
        OutputPlugins();
        virtual ~OutputPlugins();

        using Plugins = std::unordered_set<OutputPluginPtr>;

        //
        //  Load all on-disk plugins
        //

        static void loadPlugins(const std::string& envVar);

        //
        //  Unload all on-disk plugins
        //

        static void unloadPlugins();

        //
        //  Returns an unordered set of all the currently loaded plugins
        //

        static Plugins& plugins();

    private:
        static void addPlugin(const TwkApp::OutputPluginPtr&);

        static bool m_loadedAll;
        static Plugins* m_plugins;
    };

} // namespace TwkApp
