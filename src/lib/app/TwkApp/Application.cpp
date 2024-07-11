//******************************************************************************
// Copyright (c) 2005 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#include <TwkApp/Application.h>
#include <TwkApp/OutputPlugin.h>
#include <TwkApp/VideoDevice.h>
#include <TwkApp/VideoModule.h>
#include <stl_ext/stl_ext_algo.h>

namespace TwkApp {
using namespace stl_ext;

Application* Application::m_app = 0;

Application::Application() : TwkUtil::Notifier()
{
    assert(m_app == 0);
    m_app = this;
}

Application::~Application()
{
    m_app = 0;

    // Make sure to release all the video modules before we unload the plugins
    m_videoModules.clear();
}

void
Application::add(Document* d)
{
    d->addNotification(this, Document::deleteMessage());
    m_documents.push_back(d);
}

void
Application::remove(Document* d)
{
    stl_ext::remove(m_documents, d);
}

bool 
Application::receive(Notifier* originator,
                     Notifier* sender, 
                     Notifier::MessageId id, 
                     Notifier::MessageData* data)
{
    if (id == Document::deleteMessage())
    {
        remove(static_cast<Document*>(originator));
    }

    return true;
}

VideoModule* 
Application::primaryVideoModule() const
{
    return m_videoModules.empty() ? 0 : m_videoModules.front().get();
}

void
Application::loadOutputPlugins(const std::string& envvar)
{
    m_outputPlugins.loadPlugins(envvar);
    for (auto outputPlugin : m_outputPlugins.plugins())
    {
        int moduleIndex = 0;
        TwkApp::VideoModule* outputModule = nullptr;
        do 
        {
            outputModule = outputPlugin->output_module_create(OUTPUT_PLUGIN_VERSION, moduleIndex++);
            if (outputModule)
            {
                addVideoModule(outputModule);
                for (auto d : outputModule->devices())
                {
                    std::cout << "INFO: " << outputModule->name() << " found " << d->name() << std::endl;
                }
            }
        } while (outputModule!=nullptr);
    }
}

void
Application::unloadOutputPlugins()
{
    m_outputPlugins.unloadPlugins();
}


} // TwkApp
