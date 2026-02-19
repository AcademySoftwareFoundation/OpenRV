//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
// blablabla
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkMovie/MovieIO.h>
#include <TwkMovie/Exception.h>
#include <TwkUtil/File.h>
#include <TwkUtil/PathConform.h>
#include <stdarg.h>
#include <iostream>
#include <assert.h>
#include <stl_ext/string_algo.h>
#include <dlfcn.h>
#include <mutex>

#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

static std::mutex plugin_mutex;

static bool TwkMovie_GenericIO_debug = false;

TWKMOVIE_EXPORT void TwkMovie_GenericIO_setDebug(bool b) { TwkMovie_GenericIO_debug = b; }

namespace TwkMovie
{
    using namespace std;
    using namespace TwkUtil;
    typedef MovieIO* create_t(float);
    typedef void destroy_t(MovieIO*);
    typedef bool* codecIsAllowed_t(std::string, bool);

    static const char* capabilityNames[] = {"AttributeRead", "AttributeWrite", "Read", "Write", "AudioRead", "AudioWrite"};

    //
    // GenericIO::Preloader implementation
    //
    GenericIO::Preloader::Reader::Reader(std::string_view filename, const Movie::ReadRequest& request)
    {
        // other members initialized to default in the class definition
        m_filename = filename;
        m_request = request;
    }

    void GenericIO::Preloader::Reader::clearFilename() { m_filename = "**** filename cleared ****"; }

    const std::string& GenericIO::Preloader::Reader::filename() const { return m_filename; }

    const Movie::ReadRequest& GenericIO::Preloader::Reader::request() const { return m_request; }

    void GenericIO::Preloader::Reader::setPriority(bool priority) { m_priority = priority; }

    bool GenericIO::Preloader::Reader::isPriority() const { return m_priority; }

    void GenericIO::Preloader::Reader::setStatus(Status status) { m_status = status; }

    bool GenericIO::Preloader::Reader::isRemove() const { return (m_status == Status::REMOVE); }

    bool GenericIO::Preloader::Reader::isPending() const { return (m_status == Status::PENDING); }

    bool GenericIO::Preloader::Reader::isLoading() const { return (m_status == Status::LOADING); }

    bool GenericIO::Preloader::Reader::isFinishedLoading() const { return (m_status == Status::LOADED) || (m_status == Status::LOADERROR); }

    void GenericIO::Preloader::Reader::load()
    {
        try
        {
            m_movieReader = GenericIO::preloadOpenMovieReader(filename(), request(), true);

            if (m_movieReader != nullptr)
                setStatus(Reader::Status::LOADED);
            else
                setStatus(Reader::Status::LOADERROR);
        }
        catch (...)
        {
            setStatus(Reader::Status::LOADERROR);
        }
    }

    GenericIO::Preloader::Preloader()
        : m_maxThreads(32)
        , m_exitRequested(false)
        , m_threadCount(0)
        , m_ceilingThreadCount(0)
        , m_completedCount(0)
        , m_notReadyCount(0)
        , m_getCount(0)
    {
    }

    GenericIO::Preloader::~Preloader() { shutdown(); }

    void GenericIO::Preloader::init() { m_schedulerThread = std::thread(&Preloader::schedulerThreadFunc, this); }

    void GenericIO::Preloader::shutdown()
    {
        {
            // lock in this local scope
            std::unique_lock<std::mutex> lock(m_schedulerThread_mutex);
            m_exitRequested = true;
            m_schedulerThread_cv.notify_all();
            // unlock at local scope
        }

        // wait for the scheduler thread to exit
        if (m_schedulerThread.joinable())
        {
            m_schedulerThread.join();
        }

        // wait for reader threads to complete if any ongoing.
        for (auto& reader : m_readers)
        {
            if (reader->m_thread.joinable())
                reader->m_thread.join();
        }
    }

    void GenericIO::Preloader::finalizeCompletedThreads()
    {
        bool update = false;

        // Finalize completed threads.
        for (auto& reader : m_readers)
        {
            if (reader->isPriority())
            {
                reader->setPriority(false);
            }

            if (reader->isFinishedLoading() || reader->isRemove())
            {
                if (reader->m_thread.joinable())
                {
                    reader->m_thread.join();
                    reader->m_thread = std::thread();
                    m_threadCount--;
                    m_completedCount++;
                    update = true;
                }
            }
        }

        // Finally, remove from the list any readers that have been fetched
        // from the getReader method and that have been marked for removal.

        auto range = std::remove_if(m_readers.begin(), m_readers.end(),
                                    [&update](const std::shared_ptr<Reader>& reader)
                                    {
                                        bool remove = reader->isRemove();
                                        if (remove)
                                            update = true;
                                        return remove;
                                    });

        m_readers.erase(range, m_readers.end());

        /*
        if (update)
            std::cerr << "PRELOADER STATS DONE(" << m_completedCount << ") GET("
                      << m_getCount << ") NR(" << m_notReadyCount << ") TCNT("
                      << m_threadCount << ") TCEIL(" << m_ceilingThreadCount
                      << ")" << std::endl;
        */
    }

    void GenericIO::Preloader::addReader(const std::string_view filename, const Movie::ReadRequest& request)
    {
        std::unique_lock<std::mutex> lock(m_schedulerThread_mutex);

        //        std::cerr << "PRELOADER ADD " << filename << std::endl;

        auto newReader = std::make_shared<Preloader::Reader>(filename, request);

        m_readers.push_back(newReader);

        // Wake up the scheduler thread in order to process the
        // newly added readers.
        m_schedulerThread_cv.notify_all();
    }

    bool GenericIO::Preloader::hasPendingReaders()
    {
        for (auto reader : m_readers)
        {
            if (reader->isPending())
            {
                if (m_threadCount < m_maxThreads)
                    return true;
            }
        }

        return false;
    }

    void GenericIO::Preloader::waitForFinishedLoading(std::shared_ptr<Reader> reader)
    {
        // no need to create the lock and wait if it's already ready
        if (reader->isFinishedLoading())
            return;

        std::unique_lock<std::mutex> lock(m_mainThread_mutex);
        m_mainThread_cv.wait(lock, [&reader] { return reader->isFinishedLoading(); });
    }

    MovieReader* GenericIO::Preloader::getReader(std::string_view filename, const MovieInfo& info, Movie::ReadRequest& request)
    {
        //        std::cerr << "PRELOADER GET " << filename << std::endl;

        m_getCount++;

        std::shared_ptr<Reader> reader = nullptr;

        //
        // Check to see if we have a reader with the corresponding filename in
        // the queue. (Also check to make sure we don't get an iterator to a
        // reader that is scheduled to be deleted (eg: Status::REMOVE)
        //
        // Do this while locking a limited scope to search in m_Readers.
        // In principle, locking here is un-necessary, but we're doing it
        // to be safer.
        //
        {
            std::unique_lock<std::mutex> lock(m_schedulerThread_mutex);

            std::list<std::shared_ptr<Reader>>::iterator it =
                std::find_if(m_readers.begin(), m_readers.end(),
                             [&filename](const auto& reader) { return (reader->filename() == filename) && (!reader->isRemove()); });

            if (it != m_readers.end())
                reader = *it;
        }

        if (reader)
        {
            //
            // is the reader still pending? if so that means we're asking for
            // the reader even before the scheduler thread has had a chance to
            // schedule it. Maybe the max thread cound is reached, or the
            // scheduler thread has not had a chance to wake up -- either way,
            // escalate the priority and wake up the scheduler thread so that it
            // can schedule the reader now.
            //
            if (reader->isPending() || reader->isLoading())
            {
                // if so, we don't have enough threads and our maxThreads
                reader->setPriority(true);
                m_notReadyCount++;

                // Wake up the scheduler thread so that it may
                // schedule this reader.
                m_schedulerThread_cv.notify_all();
                waitForFinishedLoading(reader);
            }

            // Wait for the reader to be finished loading.

            MovieReader* movieReader = reader->m_movieReader;

            // We're essentially done with this reader now.
            // We can set its filename to a non-filename, and set its status to
            // be removed, and then wake up the scheduler thread so that
            // it may perform some cleanup operations such as closing out
            // (joining) the thread and so that the reader may be removed from
            // the list.
            reader->clearFilename();
            reader->setStatus(Reader::Status::REMOVE);

            // Wake up the scheduler thread so that it may remove
            // the Reader from the list (from here, it's no longer safe to
            // access the "reader" variable)
            m_schedulerThread_cv.notify_all();

            // if movieReader is null, it's because there was a read error.
            if (movieReader)
            {
                movieReader->postPreloadOpen(info, request);
            }
            else
            {
                std::cerr << "PRELOADER READ ERROR: " << reader->filename() << std::endl;
            }

            return movieReader;
        }

        return nullptr;
    }

    void GenericIO::Preloader::schedulerThreadFunc()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(m_schedulerThread_mutex);

            // unblock the wait if we have something to do.
            m_schedulerThread_cv.wait(lock,
                                      [this]
                                      {
                                          // clean up any completed reader
                                          // thread states after they have
                                          // completed (successfully or not)
                                          finalizeCompletedThreads();

                                          // resume execution if quitting or if
                                          // anything to schedule
                                          return m_exitRequested || hasPendingReaders();
                                      });

            // if asked to stop, exit the scheduler thread (done by
            // GenericIO::shutdown)
            if (m_exitRequested)
                return;

            // Next, check if we need to schedule anything new.

            // Do two passes scanning readers to see if any should be scheduled.
            //
            // First pass: find priority readers and schedule them regqardless
            // if we're maxed out. (this will just add +1 thread) Second pass:
            // find non-priority readers and schedule them if room.
            //
            for (int i = 0; i < 2; i++)
            {
                for (auto& reader : m_readers)
                {
                    // don't consider readers that have been handled
                    // or are in the process of being handled by a
                    // loading thread
                    if (!reader->isPending())
                        continue;

                    // filter for 1st pass (only consider priority readers)
                    if ((i == 0) && !reader->isPriority())
                        continue;

                    // filter for 2nd pass (only consider non-priority readers)
                    if ((i == 1) && reader->isPriority())
                        continue;

                    // if we're doing the 2nd pass (non-priority readers) and
                    // we're at the limit of concurrent threads, don't schedule
                    // any more until the number of threads comes down
                    if ((i == 1) && (m_threadCount >= m_maxThreads))
                        continue;

                    // we have confirmed we can schedule a new reader thread.
                    reader->setStatus(Reader::Status::LOADING);
                    m_threadCount++;
                    if (m_ceilingThreadCount < m_threadCount)
                        m_ceilingThreadCount = m_threadCount;

                    // Start the loader thread
                    reader->m_thread = std::thread(&Preloader::loaderThreadFunc, this, reader);
                }
            }
        }
    }

    void GenericIO::Preloader::loaderThreadFunc(std::shared_ptr<Reader> reader)
    {

        // execute the load function of the reader in this thread
        reader->load();

        // Wake up scheduler thread because we want it to schedule
        // a new thread if there are pending readers.
        m_schedulerThread_cv.notify_all();

        // Wake up main thread, because the main thread might
        // be waiting for this reader to complete in getReader().
        m_mainThread_cv.notify_all();
    }

    //
    // MovieIO implementation
    //
    std::string MovieIO::MovieTypeInfo::capabilitiesAsString() const
    {
        ostringstream str;

        for (int i = 0, count = 0; i < 6; i++)
        {
            if (capabilities & (1 << i))
            {
                if (count)
                    str << ", ";
                str << capabilityNames[i];
                count++;
            }
        }

        return str.str();
    }

    MovieIO::MovieIO(const std::string& identifier, const std::string& key)
        : m_identifier(identifier)
        , m_key(key)
    {
    }

    MovieIO::~MovieIO() {}

    MovieReader* MovieIO::movieReader() const
    {
        // throw UnsupportedException();
        return NULL;
    }

    MovieWriter* MovieIO::movieWriter() const { return NULL; }

    void MovieIO::addType(const std::string& e, unsigned int c) { m_exts.push_back(MovieTypeInfo(e, c)); }

    void MovieIO::addType(const std::string& e, const std::string& d, unsigned int c, const StringPairVector& comps,
                          const StringPairVector& acomps)
    {
        m_exts.push_back(MovieTypeInfo(e, d, c, comps, acomps));
    }

    void MovieIO::addType(const std::string& e, const std::string& d, unsigned int c, const StringPairVector& comps,
                          const StringPairVector& acomps, const ParameterVector& eparams, const ParameterVector& dparams)
    {
        m_exts.push_back(MovieTypeInfo(e, d, c, comps, acomps, eparams, dparams));
    }

    string MovieIO::about() const { return "(no info available)"; }

    const MovieIO::MovieTypeInfos& MovieIO::extensionsSupported() const { return m_exts; }

    bool MovieIO::canAttemptBruteForceRead() const
    {
        for (int i = 0; i < m_exts.size(); i++)
        {
            if (m_exts[i].capabilities & MovieBruteForceIO)
                return true;
        }

        return false;
    }

    bool MovieIO::supportsExtension(std::string extension, unsigned int capabilities) const
    {
        for (int i = 0; i < m_exts.size(); i++)
        {
            if (!strcasecmp(m_exts[i].extension.c_str(), extension.c_str()))
            {
                if ((m_exts[i].capabilities & capabilities) == capabilities || capabilities == AnyCapability)
                {
                    return true;
                }
            }
        }

        return false;
    }

    void MovieIO::getMovieInfo(const std::string& filename, MovieInfo&) const
    {
        TWK_THROW_STREAM(IOException, "getMovieInfo not implemented");
    }

    bool MovieIO::getBoolAttribute(const std::string& name) const { return m_boolMap[name]; }

    void MovieIO::setBoolAttribute(const std::string& name, bool value) { m_boolMap[name] = value; }

    int MovieIO::getIntAttribute(const std::string& name) const { return m_intMap[name]; }

    void MovieIO::setIntAttribute(const std::string& name, int value) { m_intMap[name] = value; }

    string MovieIO::getStringAttribute(const std::string& name) const { return m_stringMap[name]; }

    void MovieIO::setStringAttribute(const std::string& name, const std::string& value) { m_stringMap[name] = value; }

    double MovieIO::getDoubleAttribute(const std::string& name) const { return m_doubleMap[name]; }

    void MovieIO::setDoubleAttribute(const std::string& name, double value) const { m_doubleMap[name] = value; }

    void MovieIO::setMovieAttributesOn(Movie* mov) const
    {
        for (BoolMap::const_iterator i = m_boolMap.begin(); i != m_boolMap.end(); ++i)
        {
            mov->setBoolAttribute((*i).first, (*i).second);
        }

        for (IntMap::const_iterator i = m_intMap.begin(); i != m_intMap.end(); ++i)
        {
            mov->setIntAttribute((*i).first, (*i).second);
        }

        for (StringMap::const_iterator i = m_stringMap.begin(); i != m_stringMap.end(); ++i)
        {
            mov->setStringAttribute((*i).first, (*i).second);
        }

        for (DoubleMap::const_iterator i = m_doubleMap.begin(); i != m_doubleMap.end(); ++i)
        {
            mov->setDoubleAttribute((*i).first, (*i).second);
        }
    }

    void MovieIO::copyAttributesFrom(const MovieIO* movio)
    {
        m_boolMap = movio->boolMap();
        m_intMap = movio->intMap();
        m_stringMap = movio->stringMap();
        m_doubleMap = movio->doubleMap();
    }

    //----------------------------------------------------------------------

    string ProxyMovieIO::about() const
    {
        ostringstream str;
        str << "Proxy for " << identifier() << " - key: " << sortKey();
        return str.str();
    }

    //----------------------------------------------------------------------

    GenericIO::Plugins* GenericIO::m_plugins = 0;
    bool GenericIO::m_loadedAll = false;
    bool GenericIO::m_dnxhdDecodingAllowed = true;
    GenericIO::Preloader GenericIO::m_preloader;

    void GenericIO::init()
    {
        if (!m_plugins)
            m_plugins = new Plugins();

        m_preloader.init();
    }

    void GenericIO::shutdown()
    {
        m_preloader.shutdown();

        if (m_plugins)
        {
            for (Plugins::iterator i = plugins().begin(); i != plugins().end(); ++i)
            {
                delete *i;
            }
            delete m_plugins;
            m_plugins = 0;
        }

        m_loadedAll = false;
    }

    const GenericIO::Plugins& GenericIO::allPlugins()
    {
        if (!m_plugins)
            m_plugins = new Plugins;
        return *m_plugins;
    }

    bool GenericIO::dnxhdDecodingAllowed() { return m_dnxhdDecodingAllowed; }

    void GenericIO::setDnxhdDecodingAllowed(bool b) { m_dnxhdDecodingAllowed = b; }

    GenericIO::Plugins& GenericIO::plugins()
    {
        if (!m_plugins)
            m_plugins = new Plugins;
        return *m_plugins;
    }

    vector<string> GenericIO::getPluginFileList(const string& pathVar)
    {
        string pluginPath;

        if (getenv(pathVar.c_str()))
        {
            pluginPath = getenv(pathVar.c_str());
        }
        else
        {
            pluginPath = ".";
        }

#ifdef PLATFORM_LINUX
        return findInPath(".*\\.so$", pluginPath);
#endif

#ifdef PLATFORM_WINDOWS
        return findInPath(".*\\.dll$", pluginPath);
#endif

#ifdef PLATFORM_DARWIN
        return findInPath(".*\\.dylib$", pluginPath);
#endif
    }

    MovieIO* GenericIO::loadPlugin(const string& file)
    {
        if (void* handle = dlopen(file.c_str(), RTLD_LAZY))
        {
            create_t* plugCreate = (create_t*)dlsym(handle, "create");
            destroy_t* plugDestroy = (destroy_t*)dlsym(handle, "destroy");
            codecIsAllowed_t* plugCodecIsAllowed = (codecIsAllowed_t*)dlsym(handle, "codecIsAllowed");

            if (!plugCreate)
            {
                //
                //  Hack to get around quicktime on windows
                //

                plugCreate = (create_t*)dlsym(handle, "create2");
            }

            if (!plugCreate || !plugDestroy)
            {
                dlclose(handle);

                cerr << "ERROR: ignoring movie plugin " << file << ": missing create() or destroy(): " << dlerror() << endl;
            }
            else
            {
                try
                {
                    if (MovieIO* plugin = plugCreate(FB_PLUGIN_VERSION))
                    {
                        plugin->setPluginFile(file);
                        plugin->init();

                        if (TwkMovie_GenericIO_debug)
                        {
                            cerr << "INFO: plugin loaded: " << file << " ID " << plugin->identifier() << ", description '"
                                 << plugin->about() << "'" << endl;

                            //
                            //  Check for _ARGS-style args env var
                            //
                            string var = plugin->identifier();
                            transform(var.begin(), var.end(), var.begin(), (int (*)(int))toupper);
                            var = var + "_ARGS";
                            if (getenv(var.c_str()))
                                cerr << "INFO: " << var << ": " << getenv(var.c_str()) << endl;
                        }

                        return plugin;
                    }
                }
                catch (...)
                {
                    cerr << "WARNING: exception thrown while creating plugin " << file << ", ignoring" << endl;

                    dlclose(handle);
                }
            }
        }
        else
        {
            cerr << "ERROR: cannot open movie plugin " << file << ": " << dlerror() << endl;
        }

        return 0;
    }

    void GenericIO::loadPlugins(const string& envVar)
    {
        if (!m_loadedAll)
        {
            vector<string> pluginFiles = getPluginFileList(envVar);

            if (pluginFiles.empty())
            {
                cout << "INFO: no image plugins found" << endl;
                cout << "INFO: " << getenv(envVar.c_str()) << endl;
            }

            for (int i = 0; i < pluginFiles.size(); i++)
            {
                if (!alreadyLoaded(pluginFiles[i]))
                {
                    cout << "INFO: loading plugin " << pluginFiles[i] << endl;

                    if (MovieIO* plug = loadPlugin(pluginFiles[i]))
                    {
                        addPlugin(plug);
                    }
                }
            }

            m_loadedAll = true;
        }
    }

    bool GenericIO::alreadyLoaded(const std::string& pluginFile)
    {
        for (Plugins::iterator i = plugins().begin(); i != plugins().end(); ++i)
        {
            if ((*i)->pluginFile() == pluginFile)
                return true;
        }

        return false;
    }

    MovieIO* GenericIO::loadFromProxy(Plugins::iterator i)
    {
        ProxyMovieIO* pio = dynamic_cast<ProxyMovieIO*>(*i);

        if (MovieIO* newio = loadPlugin(pio->pathToPlugin()))
        {
            newio->m_key = pio->sortKey();
            plugins().erase(i);
            plugins().insert(newio);
            delete pio;
            return newio;
        }
        else
        {
            cout << "WARNING: " << pio->pathToPlugin() << " failed to load" << endl;

            plugins().erase(i);
            delete pio;
            return 0;
        }
    }

    void GenericIO::addPlugin(MovieIO* plugin)
    {
        while (plugins().find(plugin) != plugins().end())
        {
            plugin->m_key += "_";
        }

        plugins().insert(plugin);
    }

    static bool findNoCaseOnWindows(string s1, string s2)
    {
        string news2 = s2;
#ifdef PLATFORM_WINDOWS
        for (int i = 0; i < news2.size(); ++i)
        {
            if (news2[i] == '\\')
                news2[i] = '/';
            else
                news2[i] = toupper(news2[i]);
        }
#endif

        return (string::npos != news2.find(s1));
    }

    static bool QTmissing(bool print)
    {
        static bool first = true;
        static bool QTnotInPath = false;

        if (first)
        {
            const char* path = getenv("PATH");
            if (path && !findNoCaseOnWindows("/QTSYSTEM", path))
                QTnotInPath = true;
            first = false;
        }
        if (print && QTnotInPath)
        {
            cerr << "ERROR: Please install Apple QuickTime, it is required to "
                    "view movies on Windows."
                 << endl;
        }

        return QTnotInPath;
    }

    const MovieIO* GenericIO::findByExtension(const string& extension, unsigned int capabilities)
    {
        if (!plugins().empty())
        {
            std::lock_guard<std::mutex> guard(plugin_mutex);
            for (bool restart = true; restart;)
            {
                restart = false;

                for (Plugins::iterator i = plugins().begin(); i != plugins().end(); ++i)
                {
                    MovieIO* io = *i;

                    if (io && io->supportsExtension(extension, capabilities))
                    {
                        if (ProxyMovieIO* pio = dynamic_cast<ProxyMovieIO*>(io))
                        {
                            if (TwkMovie_GenericIO_debug)
                            {
                                cout << "INFO: " << extension << " supported by plugin " << basename(pio->pathToPlugin()) << endl;
                            }

                            if ((io = loadFromProxy(i)))
                            {
                                return io;
                            }
                            else
                            {
                                restart = true;
                                break;
                            }
                        }
                        else
                        {
                            return io;
                        }
                    }
                }
            }
        }

        return NULL;
    }

    int GenericIO::findAllByExtension(const string& extension, unsigned int capabilities, MovieIOSet& ioSet)
    {
        ioSet.clear();

        if (!plugins().empty())
        {
            std::lock_guard<std::mutex> guard(plugin_mutex);
            for (bool restart = true; restart;)
            {
                restart = false;

                for (Plugins::iterator i = plugins().begin(); i != plugins().end(); ++i)
                {
                    MovieIO* io = *i;

                    if (io && io->supportsExtension(extension, capabilities))
                    {
                        if (ProxyMovieIO* pio = dynamic_cast<ProxyMovieIO*>(io))
                        {
                            if (TwkMovie_GenericIO_debug)
                            {
                                cout << "INFO: " << extension << " supported by plugin " << basename(pio->pathToPlugin()) << endl;
                            }

                            if ((io = loadFromProxy(i)))
                            {
                                ioSet.insert(io);
                            }
                            restart = true;
                            break;
                        }
                        else
                        {
                            ioSet.insert(io);
                        }
                    }
                }
            }
        }

        return ioSet.size();
    }

    const MovieIO* GenericIO::findByBruteForce(const std::string& filename, unsigned int capabilities)
    {
        if (!plugins().empty())
        {
            cerr << "INFO: trying brute force to find an image reader for " << basename(filename) << endl;

            std::lock_guard<std::mutex> guard(plugin_mutex);
            for (bool restart = true; restart;)
            {
                restart = false;

                for (Plugins::iterator i = plugins().begin(); i != plugins().end(); ++i)
                {
#if defined(PLATFORM_WINDOWS) && defined(ARCH_IA32)
                    if ((*i)->identifier() == "Apple Quicktime" && QTmissing(false))
                        continue;
#endif

                    if (*i && (*i)->canAttemptBruteForceRead())
                    {
                        if (dynamic_cast<ProxyMovieIO*>(*i))
                        {
                            loadFromProxy(i);
                            restart = true;
                            break; // restart
                        }

                        try
                        {
                            MovieInfo info;

                            (*i)->getMovieInfo(filename, info);

                            cerr << "INFO: " << basename(filename) << " is being read by: " << (*i)->about() << endl;

                            return (*i);
                        }
                        catch (...)
                        {
                        }
                    }
                }
            }
        }

        return NULL;
    }

    MovieReader* GenericIO::movieReader(const std::string& filename, bool tryBruteForce)
    {
        unsigned int image = MovieIO::MovieRead;
        unsigned int audio = MovieIO::MovieReadAudio;
        string ext = extension(filename);

        MovieReader* m = 0;
        const MovieIO* io = 0;

        if ((io = findByExtension(ext, image | audio)))
        {
            m = io->movieReader();
        }
        else if ((io = findByExtension(ext, image)))
        {
            m = io->movieReader();
        }
        else if ((io = findByExtension(ext, audio)))
        {
            m = io->movieReader();
        }
        else if (tryBruteForce)
        {
            if ((io = findByBruteForce(filename, image | audio)))
            {
                m = io->movieReader();
            }
            else if ((io = findByBruteForce(filename, image)))
            {
                m = io->movieReader();
            }
            else if ((io = findByBruteForce(filename, audio)))
            {
                m = io->movieReader();
            }
        }

        if (m)
            io->setMovieAttributesOn(m);

        return m;
    }

    namespace
    {

        bool checkFDLimitOK(const string& filename)
        {
#ifndef PLATFORM_WINDOWS
            int fd = ::open("/", O_RDONLY);

            if (fd == -1)
            {
                if (errno == 24)
                {
                    struct rlimit rlim;
                    getrlimit(RLIMIT_NOFILE, &rlim);
                    TWK_THROW_EXC_STREAM("ERROR: Failed to open for reading: " << filename << endl
                                                                               << "    The per-process limit on open files ("
                                                                               << rlim.rlim_cur << ") has" << endl
                                                                               << "    been reached.  To open a larger number of "
                                                                                  "media files, speak to your"
                                                                               << endl
                                                                               << "    IT person about increasing that limit.");
                }
            }
            else
            {
                close(fd);
            }
#endif

            return true;
        }

        MovieReader* preloadTryOpen(const MovieIO* io, const std::string& filename, const Movie::ReadRequest& request)
        {
            //  cerr << "tryOpen '" << filename << "' with '" << io->about() <<
            //  "'" << endl;

            MovieReader* m = io->movieReader();
            if (m)
            {
                try
                {
                    io->setMovieAttributesOn(m);
                    m->preloadOpen(filename, request);
                }
                catch (std::exception& exc)
                {
                    checkFDLimitOK(filename);
                    //  XXX should delete, but need to robustify plugins first
                    //  XXX This leaks "m".
                    // delete m;
                    m = 0;
                    if (TwkMovie_GenericIO_debug)
                    {
                        cerr << "WARNING: " << io->identifier() << " failed to open " << filename << ": " << exc.what() << endl;
                    }
                }
                catch (...)
                {
                    checkFDLimitOK(filename);
                    // XXX This leaks "m"
                    // delete m;
                    m = 0;
                    if (TwkMovie_GenericIO_debug)
                    {
                        cerr << "WARNING: " << io->identifier() << " failed to open " << filename << endl;
                    }
                }
            }
            else
            {
                checkFDLimitOK(filename);
            }
            return m;
        }

        MovieReader* tryOpen(const MovieIO* io, const std::string& filename, const MovieInfo& mi, Movie::ReadRequest request)
        {
            //  cerr << "tryOpen '" << filename << "' with '" << io->about() <<
            //  "'" << endl;

            MovieReader* m = io->movieReader();
            if (m)
            {
                try
                {
                    io->setMovieAttributesOn(m);
                    m->open(filename, mi, request);
                }
                catch (std::exception& exc)
                {
                    checkFDLimitOK(filename);
                    //  XXX should delete, but need to robustify plugins first
                    //  XXX This leaks "m".
                    // delete m;
                    m = 0;
                    if (TwkMovie_GenericIO_debug)
                    {
                        cerr << "WARNING: " << io->identifier() << " failed to open " << filename << ": " << exc.what() << endl;
                    }
                }
                catch (...)
                {
                    checkFDLimitOK(filename);
                    // XXX This leaks "m"
                    // delete m;
                    m = 0;
                    if (TwkMovie_GenericIO_debug)
                    {
                        cerr << "WARNING: " << io->identifier() << " failed to open " << filename << endl;
                    }
                }
            }
            else
            {
                checkFDLimitOK(filename);
            }
            return m;
        }

    }; // namespace

    MovieReader* GenericIO::preloadOpenMovieReader(const std::string& filename, const Movie::ReadRequest& request, bool tryBruteForce)
    {
        //
        // This method is really similar to openMoviePlayer, except that
        // the preloadTryOpen() method does not use MovieInfo and Request.
        // This method is meant to be called from the loading thread func
        // of the Preloader, not from the main thread. When the main thread
        // tries to get a movie reader, it needs to call openMovieReader,
        // and it's that method that will check if there's something in the
        // preload queue. If there's nothing in the preload queue,
        // openMovieReader() will open the file in sync mode
        // just as before.
        //
        unsigned int image = MovieIO::MovieRead;
        unsigned int audio = MovieIO::MovieReadAudio;
        string ext = extension(filename);
        if (ext == "")
            ext = basename(filename); // Try filename if there is no ext

        MovieReader* m = 0;
        MovieIOSet ioSet;
        if (findAllByExtension(ext, image | audio, ioSet) || findAllByExtension(ext, image, ioSet) || findAllByExtension(ext, audio, ioSet))
        {
            for (MovieIOSet::iterator mio = ioSet.begin(); mio != ioSet.end(); ++mio)
            {
                if ((m = preloadTryOpen(*mio, filename, request)))
                    return m;
            }
        }
        else if (TwkUtil::pathIsURL(filename) && findAllByExtension("mov", image | audio, ioSet))
        {
            for (MovieIOSet::iterator mio = ioSet.begin(); mio != ioSet.end(); ++mio)
            {
                if ((m = preloadTryOpen(*mio, filename, request)))
                    return m;
            }
        }
        else if (tryBruteForce)
        {
            const MovieIO* io;
            if (((io = findByBruteForce(filename, image | audio))) || ((io = findByBruteForce(filename, image)))
                || ((io = findByBruteForce(filename, audio))))
            {
                if ((m = preloadTryOpen(io, filename, request)))
                    return m;
            }
        }

        TWK_THROW_EXC_STREAM("unsupported media type.");
    }

    GenericIO::Preloader& GenericIO::getPreloader() { return m_preloader; }

    MovieReader* GenericIO::openMovieReader(const std::string& filename, const MovieInfo& mi, Movie::ReadRequest& request,
                                            bool tryBruteForce)
    {
        MovieReader* m = 0;

        // First query the preloader if there's a preloaded file waiting
        // If yes, the fetch it from the preloader.
        // Otherwise, open as usual.
        if ((m = getPreloader().getReader(filename, mi, request)))
        {
            return m;
        }

        unsigned int image = MovieIO::MovieRead;
        unsigned int audio = MovieIO::MovieReadAudio;
        string ext = extension(filename);
        if (ext == "")
            ext = basename(filename); // Try filename if there is no ext

        MovieIOSet ioSet;
        if (findAllByExtension(ext, image | audio, ioSet) || findAllByExtension(ext, image, ioSet) || findAllByExtension(ext, audio, ioSet))
        {
            for (MovieIOSet::iterator mio = ioSet.begin(); mio != ioSet.end(); ++mio)
            {
                MovieInfo infoCopy = mi;
                if ((m = tryOpen(*mio, filename, infoCopy, request)))
                    return m;
            }
        }
        else if (TwkUtil::pathIsURL(filename) && findAllByExtension("mov", image | audio, ioSet))
        {
            for (MovieIOSet::iterator mio = ioSet.begin(); mio != ioSet.end(); ++mio)
            {
                MovieInfo infoCopy = mi;
                if ((m = tryOpen(*mio, filename, infoCopy, request)))
                    return m;
            }
        }
        else if (tryBruteForce)
        {
            const MovieIO* io;
            if ((io = findByBruteForce(filename, image | audio)) || (io = findByBruteForce(filename, image))
                || (io = findByBruteForce(filename, audio)))
            {
                if ((m = tryOpen(io, filename, mi, request)))
                    return m;
            }
        }

        TWK_THROW_EXC_STREAM("unsupported media type.");
    }

    MovieWriter* GenericIO::movieWriter(const string& filename)
    {
        unsigned int image = MovieIO::MovieWrite;
        unsigned int audio = MovieIO::MovieWriteAudio;
        string ext = extension(filename);

        if (const MovieIO* io = findByExtension(ext, image | audio))
        {
            return io->movieWriter();
        }
        else if (const MovieIO* io = findByExtension(ext, image))
        {
            return io->movieWriter();
        }
        else if (const MovieIO* io = findByExtension(ext, audio))
        {
            return io->movieWriter();
        }

        cerr << "ERROR: TwkMovie: No plugins support (write) format: " << ext << endl;

        return 0;
    }

    void GenericIO::outputParameters(const MovieIO::ParameterVector& params, const string& codec, const string& header)
    {
        int count = 0;

        for (size_t j = 0; j < params.size(); j++)
        {
            if (params[j].codec == codec)
            {
                if (count == 0)
                {
                    cout << "         " << header << endl;
                }

                cout << "            " << params[j].name << " " << params[j].description << endl;

                count++;
            }
        }
    }

    void GenericIO::outputFormats()
    {
        const Plugins& mbps = plugins();

        for (Plugins::iterator i = plugins().begin(); i != plugins().end(); ++i)
        {
            const MovieIO::MovieTypeInfos& exts = (*i)->extensionsSupported();

            for (int q = 0; q < exts.size(); q++)
            {
                const MovieIO::MovieTypeInfo& info = exts[q];

                cout << "format \"" << info.extension << "\""
                     << " - " << info.description << " (" << info.capabilitiesAsString() << ")" << endl;

                outputParameters(info.encodeParameters, "", "encode parameters");
                outputParameters(info.decodeParameters, "", "decode parameters");

                if (info.codecs.size())
                {
                    cout << "   image codecs: " << endl;

                    for (int j = 0; j < info.codecs.size(); j++)
                    {
                        cout << "      " << info.codecs[j].first;

                        if (info.codecs[j].second != "")
                        {
                            cout << " (" << info.codecs[j].second << ")";
                        }

                        cout << endl;

                        outputParameters(info.encodeParameters, info.codecs[j].first, "encode parameters");

                        outputParameters(info.decodeParameters, info.codecs[j].first, "decode parameters");
                    }

                    cout << endl;
                }

                if (info.audioCodecs.size())
                {
                    cout << "   audio codecs: " << endl;

                    for (int j = 0; j < info.audioCodecs.size(); j++)
                    {
                        cout << "      " << info.audioCodecs[j].first;

                        if (info.audioCodecs[j].second != "")
                        {
                            cout << " (" << info.audioCodecs[j].second << ")";
                        }

                        cout << endl;

                        outputParameters(info.encodeParameters, info.audioCodecs[j].first, "encode parameters");

                        outputParameters(info.decodeParameters, info.audioCodecs[j].first, "decode parameters");
                    }

                    cout << endl;
                }
            }
        }
    }

} // namespace TwkMovie

#ifdef _MSC_VER
#undef strcasecmp
#endif
