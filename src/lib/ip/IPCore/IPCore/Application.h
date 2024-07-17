//******************************************************************************
// Copyright (c) 2005 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifndef __IPCore__Application__h__
#define __IPCore__Application__h__
#include <TwkApp/Application.h>
#include <TwkApp/VideoDevice.h>
#include <TwkApp/VideoModule.h>
#include <functional>
#include <pthread.h>
#include <vector>
#include <boost/signals2.hpp>
#include <boost/any.hpp>

namespace IPCore {
class Session;
class NodeManager;

//
//  class IPGraph::Application
//
//  This class should be created by Rv's main. A timer should be
//  hooked up to this class. When the Application sends a
//  startTimerMessage() the timer should start firing at ~100hz. When
//  the timer fires it should call timerCB() on the Application
//  object. If the Application sends stopTimerMessage() the timer
//  should stop.
//

class Application : public TwkApp::Application
{
  public:
    //
    //  Types
    //

    typedef boost::any                                  VariantType;
    typedef std::map<std::string,VariantType>           OptionMap;
    typedef std::vector<Session*>                       Sessions;
    typedef std::vector<std::string>                    StringVector;
    typedef std::map<std::string,std::string>           PropEnvMap;
    typedef TwkApp::VideoModule                         VideoModule;
    typedef std::vector<std::shared_ptr<VideoModule>>   VideoModules;
    typedef size_t                                      DispatchID;
    typedef std::function<void(DispatchID)>             DispatchCallback;

    //
    //  Signal Types
    //

    typedef boost::signals2::signal<void ()>                    VoidSignal;
    typedef boost::signals2::signal<void (const std::string&)>  StringSignal;
    typedef boost::signals2::signal<void (const StringVector&)> StringVectorSignal;

    //
    //  Signals
    //

    VoidSignal& startTimerSignal() { return m_startTimerSignal; }
    VoidSignal& stopTimerSignal() { return m_stopTimerSignal; }
    StringVectorSignal& createNewSessionFromFilesSignal() { return m_createNewSessionFromFilesSignal; }
    StringSignal& optionSetSignal() { return m_optionSetSignal; }

    //
    //  Messages
    //

    NOTIFIER_MESSAGE(startTimerMessage);
    NOTIFIER_MESSAGE(stopTimerMessage);
    NOTIFIER_MESSAGE(createNewSessionFromFiles);

    //
    //  Constructors
    //

    Application();
    virtual ~Application();

    static Application* instance() 
        { return static_cast<Application*>(TwkApp::App()); }

    //
    //  NodeManager
    //
    //  options:    nodePath -> path seperated list string
    //              nodeDefinitionOverride -> path string 
    //
    //  If either of the above are set and loadOptionNodeDefinitions()
    //  is called those will be loaded dynamically. 
    //

    NodeManager* nodeManager() { return m_nodeManager; }
    const NodeManager* nodeManager() const { return m_nodeManager; }
    void loadOptionNodeDefinitions();

    //
    //  Session API (called by the Session instances)
    //

    void startPlay(Session*);
    void stopPlay(Session*);
    void stopAll();
    void resumeAll();

    //
    //  Generate a new Session (possibly at some point in the
    //  the future).
    //

    virtual void createNewSessionFromFiles(const StringVector&);

    //
    //  Get a session by name
    //

    Session* session(const std::string& name) const;

    //
    // Dispatch the execution of the given function into the main thread.
    //

    virtual DispatchID dispatchToMainThread(DispatchCallback callback);

    //
    // cancel the job with a specific dispatchID if the job is not dispatched
    //
    
    virtual void undispatchToMainThread(DispatchID dispatchID, double maxDuration);

    //
    //  External Interface. virtual funcs are intended for app objects
    //  which inherit from this one (e.g. mixed with QApplication or
    //  NSApplication).
    //

    virtual void stopPlaybackTimer();
    virtual void startPlaybackTimer();
    void timerCB();

    //
    //  Memory Available for Caching
    //

    size_t availableMemory() { return m_availableMemory; }
    size_t usedMemory() { return m_usedMemory; }

    //
    //  Will return false if you should *not* allocate the memory
    //  being asked for. In that case -- free something up!
    //

    bool requestMemory(size_t bytes);

    //
    //  When freeing up memory tell the app you did so.
    //

    void freeMemory(size_t bytes);

    //
    //  Figure out what "kind" of file this is
    //

    enum FileKind
    {
        UnknownFileKind,
        ImageFileKind,
        MovieFileKind,
        CDLFileKind,
        LUTFileKind,
        DirectoryFileKind,
        RVFileKind,
        EDLFileKind
    };

    FileKind fileKind(const std::string& file);

    std::string userGenericEventOnAll(const std::string& eventName,
                                      const std::string& contents,
                                      const std::string& sender="");

    static void cacheEnvVars();

    static std::string mapToVar(const std::string& in);
    static std::string mapFromVar(const std::string& in);

    static int parseInFiles(int argc, char *argv[]);

    static std::string mapToFrom(const std::string& in, PropEnvMap& em, bool wantLowerCase, bool wantPathSanitize);

    static OptionMap& optionMap() { return m_optionMap; }

    static bool hasOption(const OptionMap::key_type& key) { return m_optionMap.count(key) > 0; }

    template <class T>
    static void setOptionValue(const OptionMap::key_type& key, const T& value);

    static void setOptionValueFromEnvironment(const OptionMap::key_type& key, const std::string& envVar);

    template <class T>
    static T optionValue(const OptionMap::key_type& key, const T& defaultValue);

  protected:
    virtual bool receive(Notifier*,
                         Notifier*, 
                         Notifier::MessageId, 
                         Notifier::MessageData*);

    void lock()   { pthread_mutex_lock(&m_memoryLock); }
    void unlock() { pthread_mutex_unlock(&m_memoryLock); }

    //static void progress(const TwkUtil::Interrupt::Computation&);

  protected:
    static PropEnvMap   m_toVarMap;
    static PropEnvMap   m_fromVarMap;

    static PropEnvMap   m_osVarMap;

    static PropEnvMap   m_fromLINUX;
    static PropEnvMap   m_fromOSX;
    static PropEnvMap   m_fromWINDOWS;

    static OptionMap    m_optionMap;
  
  public:
    enum   HostOS 
    { 
        RVMac, 
        RVWin, 
        RVLinux 
    };
    static HostOS       m_rvHostOS;


  protected:
    size_t             m_availableMemory;
    size_t             m_usedMemory;
    pthread_mutex_t    m_memoryLock;
    std::vector<bool>  m_wasPlaying;
    VoidSignal         m_startTimerSignal;
    VoidSignal         m_stopTimerSignal;
    StringSignal       m_optionSetSignal;
    StringVectorSignal m_createNewSessionFromFilesSignal;
    NodeManager*       m_nodeManager;
};

inline Application* App() { return Application::instance(); }

template <class T>
void Application::setOptionValue(const Application::OptionMap::key_type& key, const T& value)
{
    m_optionMap[key] = value;
    if (App()) App()->m_optionSetSignal(key);
}

template <class T>
T Application::optionValue(const Application::OptionMap::key_type& key,
                           const T& defaultValue)
{
    if (m_optionMap.find(key) == m_optionMap.end())
    {
        return defaultValue;
    }
    else
    {
        return boost::any_cast<T>(m_optionMap[key]);
    }
}

} // IPCore


#endif // __IPCore__Application__h__
