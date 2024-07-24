//******************************************************************************
// Copyright (c) 2005 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifndef __TwkApp__Application__h__
#define __TwkApp__Application__h__
#include <TwkUtil/Notifier.h>
#include <TwkApp/Document.h>
#include <TwkApp/OutputPlugins.h>
#include <TwkApp/VideoModule.h>

namespace TwkApp {

//
//  class Application
//
//  Application class (only a single instance allowed). The class instance
//  should be accessed through like this:
//
//      TwkApp::App()
//
//  of just
//
//      App()
//
//  if using namespace TwkApp.
//


class Application : public TwkUtil::Notifier
{
  public:
    //
    //  Types
    //

    typedef std::vector<Document*>    Documents;
    typedef std::vector<std::shared_ptr<VideoModule>> VideoModules;
    typedef TwkUtil::Notifier         Notifier;

    //
    //  Access
    //

    static Application* instance() { return m_app; }

    //
    //  Documents
    //

    const Documents& documents() const { return m_documents; }

    //
    //  Video Device Modules
    //

    void loadOutputPlugins(const std::string& envvar);
    void unloadOutputPlugins();
    void addVideoModule(VideoModule* m) { m_videoModules.push_back(std::shared_ptr<VideoModule>(m)); }
    virtual VideoModule* primaryVideoModule() const;
    const VideoModules& videoModules() const { return m_videoModules; }

  protected:
    virtual bool receive(Notifier*,
                         Notifier*, 
                         Notifier::MessageId, 
                         Notifier::MessageData*);

  protected:
    Application();
    virtual ~Application();

    virtual void add(Document*);
    virtual void remove(Document*);

  private:
    Documents           m_documents;
    VideoModules        m_videoModules;
    OutputPlugins       m_outputPlugins;
    static Application* m_app;
    friend class Document;
};


inline Application* App() { return Application::instance(); }

} // TwkApp

#endif // __TwkApp__Application__h__
