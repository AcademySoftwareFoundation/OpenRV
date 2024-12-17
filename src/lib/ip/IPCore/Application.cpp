//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/Application.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/Exception.h>
#include <IPCore/Session.h>
#include <IPCore/NodeManager.h>
#include <TwkCMS/ColorManagementSystem.h>
#include <TwkFB/IO.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/File.h>
#include <TwkMovie/MovieIO.h>
#include <CDL/cdl_utils.h>
#include <LUT/ReadLUT.h>
#include <iostream>
#include <stl_ext/stl_ext_algo.h>
#include <stl_ext/string_algo.h>
#include <stdlib.h>

#ifndef PLATFORM_WINDOWS
extern char** environ;
#endif

namespace IPCore
{
    using namespace TwkUtil;
    using namespace std;

    NOTIFIER_MESSAGE_IMP(Application, startTimerMessage,
                         "app start timer message")
    NOTIFIER_MESSAGE_IMP(Application, stopTimerMessage,
                         "app stop timer message")
    NOTIFIER_MESSAGE_IMP(Application, createNewSessionFromFiles,
                         "app create new session from files")

    Application::PropEnvMap Application::m_toVarMap;
    Application::PropEnvMap Application::m_fromVarMap;

    Application::PropEnvMap Application::m_osVarMap;
    Application::HostOS Application::m_rvHostOS;

    Application::PropEnvMap Application::m_fromLINUX;
    Application::PropEnvMap Application::m_fromOSX;
    Application::PropEnvMap Application::m_fromWINDOWS;

    Application::OptionMap Application::m_optionMap;

    Application::Application()
        : TwkApp::Application()
    {
        //
        //  Calculate the usable RAM for capping cache
        //  memory usage.
        //

        m_availableMemory = SystemInfo::usableMemory();
        m_availableMemory -= m_availableMemory / 6;
        m_usedMemory = 0;
        pthread_mutex_init(&m_memoryLock, 0);

        //
        //  Initialize NodeManager
        //

        m_nodeManager = new NodeManager;
    }

    Application::~Application()
    {
        AudioRenderer::cleanup();
        delete m_nodeManager;
    }

    void Application::loadOptionNodeDefinitions()
    {
        string nodePath = optionValue<string>("nodePath", "");
        if (nodePath != "")
            m_nodeManager->loadDefinitionsAlongPathVar(nodePath);

        string nodeOverrideFile =
            optionValue<string>("nodeDefinitionOverride", "");
        if (nodeOverrideFile != "")
            m_nodeManager->loadDefinitions(nodeOverrideFile);
    }

    void Application::createNewSessionFromFiles(const StringVector& files)
    {
        send(createNewSessionFromFiles(), &files);
        m_createNewSessionFromFilesSignal(files);
    }

    Application::DispatchID
    Application::dispatchToMainThread(Application::DispatchCallback callback)
    {
        callback(0);
        return 0;
    }

    void Application::undispatchToMainThread(DispatchID /*dispatchID*/,
                                             double /*maxDuration*/)
    {
        // do nothing
    }

    void Application::startPlay(Session* s)
    {
        for (int i = 0; i < documents().size(); i++)
        {
            Session* session = static_cast<Session*>(documents()[i]);
            if (session->isUpdating() && session != s)
                return;
        }

        startPlaybackTimer();
        send(startTimerMessage(), this);
        m_startTimerSignal();
    }

    void Application::stopAll()
    {
        size_t s = documents().size();
        m_wasPlaying.resize(s);

        for (int i = 0; i < s; i++)
        {
            Session* session = static_cast<Session*>(documents()[i]);
            m_wasPlaying[i] = session->isPlaying();
            if (session->isUpdating())
                session->stopCompletely();
        }
    }

    void Application::stopPlaybackTimer() {}

    void Application::startPlaybackTimer() {}

    void Application::resumeAll()
    {
        size_t s = documents().size();
        m_wasPlaying.resize(s);

        for (int i = 0; i < s; i++)
        {
            Session* session = static_cast<Session*>(documents()[i]);
            if (m_wasPlaying[i])
                session->play();
        }
    }

    void Application::stopPlay(Session* s)
    {
        for (int i = 0; i < documents().size(); i++)
        {
            Session* session = static_cast<Session*>(documents()[i]);
            if (session->isUpdating())
                return;
        }

        stopPlaybackTimer();
        send(stopTimerMessage(), this);
        m_stopTimerSignal();
    }

    void Application::timerCB()
    {
        for (int i = 0; i < documents().size(); i++)
        {
            Session* s = static_cast<Session*>(documents()[i]);
            s->update();
        }
    }

    bool Application::receive(Notifier* originator, Notifier* sender,
                              Notifier::MessageId id,
                              Notifier::MessageData* data)
    {
        return TwkApp::Application::receive(originator, sender, id, data);
    }

    bool Application::requestMemory(size_t bytes)
    {
        lock();
        bool ok = false;

        if ((m_availableMemory - m_usedMemory) > bytes)
        {
            m_usedMemory += bytes;
            ok = true;
        }

        unlock();

        return ok;
    }

    void Application::freeMemory(size_t bytes)
    {
        lock();
        m_usedMemory -= bytes;
        unlock();
    }

    static void checkExists(const string& filename)
    {
        if (!fileExists(filename.c_str()))
        {
            TWK_THROW_STREAM(ReadFailedExc,
                             "File " << filename << " does not exist");
        }
    }

    Application::FileKind Application::fileKind(const std::string& filename)
    {
        string ext = extension(filename);

        //
        //  Check for image
        //

        if (isDirectory(filename.c_str()))
        {
            return DirectoryFileKind;
        }
        else if (ext == "rv")
        {
            checkExists(filename);
            return RVFileKind;
        }
        else if (ext == "rvedl" || ext == "edl")
        {
            checkExists(filename);
            return EDLFileKind;
        }
        else if (CDL::isCDLFile(filename))
        {
            return CDLFileKind;
        }
        else if (LUT::isLUTFile(filename))
        {
            return LUTFileKind;
        }
        else if (TwkFB::GenericIO::findByExtension(ext))
        {
            return ImageFileKind;
        }
        else if (TwkMovie::GenericIO::findByExtension(ext))
        {
            return MovieFileKind;
        }
        else if (TwkFB::GenericIO::findByBruteForce(filename))
        {
            return ImageFileKind;
        }
        else if (TwkMovie::GenericIO::findByBruteForce(filename))
        {
            return MovieFileKind;
        }
        else
        {
            checkExists(filename);
            return UnknownFileKind;
        }
    }

    string Application::userGenericEventOnAll(const string& eventName,
                                              const string& contents,
                                              const string& sender)
    {
        string r;

        for (int i = 0; i < documents().size(); i++)
        {
            Session* session = static_cast<Session*>(documents()[i]);
            r = session->userGenericEvent(eventName, contents, sender);
        }

        return r;
    }

    Session* Application::session(const string& name) const
    {
        for (int i = 0; i < documents().size(); i++)
        {
            Session* session = static_cast<Session*>(documents()[i]);
            if (session->name() == name)
                return session;
        }

        return 0;
    }

    static Application::HostOS getHostOS()
    {
#ifdef PLATFORM_DARWIN
        return Application::RVMac;
#endif

#ifdef PLATFORM_WINDOWS
        return Application::RVWin;
#endif

#ifdef PLATFORM_LINUX
        return Application::RVLinux;
#endif
    }

    static void path_sanitation(string& s)
    {
        // transform all backslash into slash
        size_t start_pos = 0;
        while ((start_pos = s.find("\\", start_pos)) != std::string::npos)
        {
            s.replace(start_pos, 1, "/");
            start_pos += 1;
        }

        // get rid of double slashes
        start_pos = 0;
        while ((start_pos = s.find("//", start_pos)) != std::string::npos)
        {
            // windows shares start with two slashes and URLs have two
            // legitimate slashes, so skip those.
            if (start_pos > 0 && s.substr(start_pos - 1, 1) != ":")
            {
                s.replace(start_pos, 2, "/");
            }
            start_pos += 2;
        }

        // make sure string doesnt end in a slash
        size_t last = s.find_last_of("/");
        if (last + 1 == s.size())
        {
            s = s.substr(0, s.size() - 1);
        }
    }

    void Application::cacheEnvVars()
    {
        StringVector envList;
#ifndef PLATFORM_WINDOWS
        for (char** e = environ; *e != 0; ++e)
        {
            //  cerr << "environ '" << *e << "'" << endl;
            envList.push_back(*e);
        }
#else
        //
        //  Acording to microsoft, use of environ is depricated
        //  and may give incorrect results.  The below is the
        //  approved method according to MSDN.
        //
        char* l_EnvStr = GetEnvironmentStrings();

        LPTSTR l_str = l_EnvStr;

        while (true)
        {
            if (*l_str == 0)
                break;
            envList.push_back(l_str);
            while (*l_str != 0)
                l_str++;
            l_str++;
        }
        FreeEnvironmentStrings(l_EnvStr);
#endif

        for (int i = 0; i < envList.size(); ++i)
        {
            StringVector parts;
            stl_ext::tokenize(parts, envList[i], "=");

            if (2 == parts.size() && 0 == parts[0].find("RV_PATHSWAP_"))
            {
                string envName = string("${") + parts[0] + "}";

                m_toVarMap[parts[1]] = envName;
                m_fromVarMap[envName] = parts[1];
            }

            if (2 == parts.size() && 0 == parts[0].find("RV_OS_PATH_"))
            {
                m_osVarMap[parts[0]] = parts[1];
            }
        }

        // determine host OS and allow for envvar override
        m_rvHostOS = getHostOS();
        char* override = getenv("RV_PATH_OS_OVERRIDE");
        if (override)
        {
            m_rvHostOS = (HostOS)atoi(override);
        }

        // load OS path maps

        for (PropEnvMap::const_iterator i = m_osVarMap.begin();
             i != m_osVarMap.end(); ++i)
        {
            string k = i->first;
            string v = i->second;
            path_sanitation(v);

            switch (m_rvHostOS)
            {
            case RVMac:
                if (i->first.find("RV_OS_PATH_WINDOWS") != string::npos)
                {
                    k = k.replace(0, strlen("RV_OS_PATH_WINDOWS"),
                                  "RV_OS_PATH_OSX");
                    if (m_osVarMap.count(k))
                    {
                        m_fromWINDOWS[v] = m_osVarMap[k];
                    }
                }
                if (i->first.find("RV_OS_PATH_LINUX") != string::npos)
                {
                    k = k.replace(0, strlen("RV_OS_PATH_LINUX"),
                                  "RV_OS_PATH_OSX");
                    if (m_osVarMap.count(k))
                    {
                        m_fromLINUX[v] = m_osVarMap[k];
                    }
                }
                break;
            case RVWin:
                if (i->first.find("RV_OS_PATH_OSX") != string::npos)
                {
                    string k = i->first;
                    k = k.replace(0, strlen("RV_OS_PATH_OSX"),
                                  "RV_OS_PATH_WINDOWS");
                    if (m_osVarMap.count(k))
                    {
                        m_fromOSX[v] = m_osVarMap[k];
                    }
                }
                if (i->first.find("RV_OS_PATH_LINUX") != string::npos)
                {
                    string k = i->first;
                    k = k.replace(0, strlen("RV_OS_PATH_LINUX"),
                                  "RV_OS_PATH_WINDOWS");
                    if (m_osVarMap.count(k))
                    {
                        m_fromLINUX[v] = m_osVarMap[k];
                    }
                }
                break;
            case RVLinux:
                if (i->first.find("RV_OS_PATH_OSX") != string::npos)
                {
                    string k = i->first;
                    k = k.replace(0, strlen("RV_OS_PATH_OSX"),
                                  "RV_OS_PATH_LINUX");
                    if (m_osVarMap.count(k))
                    {
                        m_fromOSX[v] = m_osVarMap[k];
                    }
                }
                if (i->first.find("RV_OS_PATH_WINDOWS") != string::npos)
                {
                    string k = i->first;
                    k = k.replace(0, strlen("RV_OS_PATH_WINDOWS"),
                                  "RV_OS_PATH_LINUX");
                    if (m_osVarMap.count(k))
                    {
                        m_fromWINDOWS[v] = m_osVarMap[k];
                    }
                }
                break;
            }
        }
    }

    //
    //  Case-independent find
    //
    static int myFind(string lookingIn, string lookingFor,
                      Application::HostOS rvHostOS, bool wantLowerCase = false)
    {
        // this lowercaseing used to be done with defines for windows only,
        // but since there is no platform specific API calls, use our new enum
        // instead. allows for testing RV_OS pathswapping on all platforms from
        // one platform
        if (rvHostOS == Application::RVWin || wantLowerCase)
        {
            for (int i = 0; i < lookingIn.size(); ++i)
                lookingIn[i] = tolower(lookingIn[i]);
            for (int i = 0; i < lookingFor.size(); ++i)
                lookingFor[i] = tolower(lookingFor[i]);
        }
        return lookingIn.find(lookingFor);
    }

    // added a wantLowerCase arg to enable case insenstive matching of windows
    // based paths default false assignments allows previous API to be
    // unchanged, especially for write cases.
    string Application::mapToFrom(const string& s, PropEnvMap& em,
                                  bool wantLowerCase = false,
                                  bool wantPathSanitize = false)
    {
        string newS = s;
        string path;
        int maxSize = 0;
        int maxPos;
        for (PropEnvMap::const_iterator i = em.begin(); i != em.end(); ++i)
        {
            string k = i->first;

            //  Preserve the structure of the PATHSWAP environment variable
            if (i->second.find("RV_PATHSWAP_") == string::npos)
                path_sanitation(k);

            int pos = myFind(newS, k, m_rvHostOS, wantLowerCase);
            if (0 == pos && k.size() > maxSize)
            //
            //  We used to replace vars/strings "mid-string", but this gets
            //  pretty unpredictable, since names of root-level paths can also
            //  appear in the middle of the path, especially if they're short.
            //  So now only swap strings at the beginning.
            //
            //  if (string::npos != pos && i->first.size() > maxSize)
            {
                maxPos = pos;
                maxSize = k.size();
                path = k;
            }
        }
        if (0 != maxSize)
        {
            //  XXX No guarantee path is a valid key for em!!!
            newS.replace(maxPos, maxSize, em[path]);
        }

        if (wantPathSanitize)
        {
            path_sanitation(newS);
        }

        return newS;
    }

    string Application::mapFromVar(const string& in)
    {
        // clean up incoming string so that compare will work properly.
        // otherwise transform of backslash to slash in mapToFrom will
        // look like a RV_PATHSWAP occured.
        string in_clean = in;
        path_sanitation(in_clean);

        // for RV_PATHSWAP API
        string s = mapToFrom(in_clean, m_fromVarMap, false, true);
        if (s.compare(in_clean))
        {
            return s;
        }

        // for RV_OS_PATH_ API, try each map until you get a match.
        s = mapToFrom(in_clean, m_fromOSX, false, true);
        if (s.compare(in_clean))
        {
            return s;
        }

        s = mapToFrom(in_clean, m_fromLINUX, false, true);
        if (s.compare(in_clean))
        {
            return s;
        }

        return mapToFrom(in_clean, m_fromWINDOWS, true, true);
    }

    string Application::mapToVar(const string& in)
    {
        return mapToFrom(in, m_toVarMap);
    }

    void Application::setOptionValueFromEnvironment(const string& key,
                                                    const std::string& envVar)
    {
        if (const char* c = getenv(envVar.c_str()))
            setOptionValue<string>(key, c);
    }

} // namespace IPCore
