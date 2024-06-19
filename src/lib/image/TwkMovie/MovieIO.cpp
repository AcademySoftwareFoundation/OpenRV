//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
// 
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
TWKMOVIE_EXPORT void TwkMovie_GenericIO_setDebug(bool b)
{
    TwkMovie_GenericIO_debug = b;
}

namespace TwkMovie {
using namespace std;
using namespace TwkUtil;
typedef MovieIO *create_t(float);
typedef void destroy_t(MovieIO*);
typedef bool *codecIsAllowed_t(std::string, bool);

static const char* capabilityNames[] = 
{
    "AttributeRead", "AttributeWrite", 
    "Read", "Write",
    "AudioRead", "AudioWrite"
};

std::string
MovieIO::MovieTypeInfo::capabilitiesAsString() const
{
    ostringstream str;

    for (int i=0, count=0; i < 6; i++)
    {
        if (capabilities & (1 << i)) 
        {
            if (count) str << ", ";
            str << capabilityNames[i];
            count++;
        }
    }

    return str.str();
}

MovieIO::MovieIO(const std::string& identifier, const std::string& key)
    : m_identifier(identifier),
      m_key(key)
{
}
    
MovieIO::~MovieIO()
{
}

MovieReader*
MovieIO::movieReader() const
{
    //throw UnsupportedException();
    return NULL;
}

MovieWriter*
MovieIO::movieWriter() const
{
    return NULL;
}

void
MovieIO::addType(const std::string& e, unsigned int c)
{
    m_exts.push_back( MovieTypeInfo(e,c) );
}


void
MovieIO::addType(const std::string& e, 
                 const std::string& d,
                 unsigned int c,
                 const StringPairVector& comps,
                 const StringPairVector& acomps)
{
    m_exts.push_back(MovieTypeInfo(e,d,c,comps,acomps));
}

void
MovieIO::addType(const std::string& e, 
                 const std::string& d,
                 unsigned int c,
                 const StringPairVector& comps,
                 const StringPairVector& acomps,
                 const ParameterVector& eparams,
                 const ParameterVector& dparams)
{
    m_exts.push_back(MovieTypeInfo(e,d,c,comps,acomps,eparams,dparams));
}

string 
MovieIO::about() const
{
    return "(no info available)";
}

const MovieIO::MovieTypeInfos&
MovieIO::extensionsSupported() const
{
    return m_exts;
}

bool 
MovieIO::canAttemptBruteForceRead() const
{
    for (int i=0; i < m_exts.size(); i++)
    {
        if (m_exts[i].capabilities & MovieBruteForceIO) return true;
    }

    return false;
}

bool 
MovieIO::supportsExtension(std::string extension, unsigned int capabilities) const
{
    for (int i=0; i < m_exts.size(); i++)
    {
        if (!strcasecmp(m_exts[i].extension.c_str(), extension.c_str()))
        {
            if ((m_exts[i].capabilities & capabilities) == capabilities ||
                capabilities == AnyCapability)
            {
                return true;
            }
        }
    }

    return false;
}

void
MovieIO::getMovieInfo(const std::string& filename, MovieInfo&) const
{
    TWK_THROW_STREAM(IOException, "getMovieInfo not implemented");
}

bool 
MovieIO::getBoolAttribute(const std::string& name) const 
{ 
    return m_boolMap[name];
}

void 
MovieIO::setBoolAttribute(const std::string& name, bool value) 
{ 
    m_boolMap[name] = value;
}

int 
MovieIO::getIntAttribute(const std::string& name) const 
{
    return m_intMap[name];
}

void 
MovieIO::setIntAttribute(const std::string& name, int value) 
{
    m_intMap[name] = value;
}

string 
MovieIO::getStringAttribute(const std::string& name) const 
{
    return m_stringMap[name];
}

void 
MovieIO::setStringAttribute(const std::string& name, const std::string& value) 
{
    m_stringMap[name] = value;
}

double 
MovieIO::getDoubleAttribute(const std::string& name) const 
{
    return m_doubleMap[name];
}

void 
MovieIO::setDoubleAttribute(const std::string& name, double value) const 
{
    m_doubleMap[name] = value;
}

void 
MovieIO::setMovieAttributesOn(Movie* mov) const
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

void 
MovieIO::copyAttributesFrom(const MovieIO* movio)
{
    m_boolMap   = movio->boolMap();
    m_intMap    = movio->intMap();
    m_stringMap = movio->stringMap();
    m_doubleMap = movio->doubleMap();
}

//----------------------------------------------------------------------


string 
ProxyMovieIO::about() const
{
    ostringstream str;
    str << "Proxy for " << identifier() << " - key: " << sortKey();
    return str.str();
}


//----------------------------------------------------------------------

GenericIO::Plugins* GenericIO::m_plugins = 0;
bool GenericIO::m_loadedAll = false;
bool GenericIO::m_dnxhdDecodingAllowed = true;

void 
GenericIO::init()
{
    if (!m_plugins) m_plugins = new Plugins();
}

void 
GenericIO::shutdown()
{
    if (m_plugins) 
    {
        for (Plugins::iterator i = plugins().begin();
             i != plugins().end();
             ++i)
        {
            delete *i;
        }
        delete m_plugins;
        m_plugins = 0;
    }

    m_loadedAll = false;
}

const GenericIO::Plugins& 
GenericIO::allPlugins()
{
    if (!m_plugins) m_plugins = new Plugins; 
    return *m_plugins;
}

bool
GenericIO::dnxhdDecodingAllowed()
{
    return m_dnxhdDecodingAllowed;
}

void
GenericIO::setDnxhdDecodingAllowed(bool b)
{
    m_dnxhdDecodingAllowed = b;
}

GenericIO::Plugins& 
GenericIO::plugins()
{
    if (!m_plugins) m_plugins = new Plugins; 
    return *m_plugins;
}

vector<string> 
GenericIO::getPluginFileList(const string& pathVar)
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


MovieIO*
GenericIO::loadPlugin(const string& file)
{
    if (void *handle = dlopen(file.c_str(), RTLD_LAZY))
    {
        create_t*  plugCreate  = (create_t *)dlsym( handle, "create" );
        destroy_t* plugDestroy = (destroy_t *)dlsym( handle, "destroy" );
        codecIsAllowed_t* plugCodecIsAllowed = (codecIsAllowed_t *)dlsym( handle, "codecIsAllowed" );

        if (!plugCreate)
        {
            //
            //  Hack to get around quicktime on windows
            //

            plugCreate  = (create_t *)dlsym( handle, "create2" );
        }
            
        if (!plugCreate || !plugDestroy)
        {
            dlclose( handle );
            
            cerr << "ERROR: ignoring movie plugin " << file 
                 << ": missing create() or destroy(): "
                 << dlerror() << endl;
        }
        else
        {
            try
            {
                if (MovieIO *plugin = plugCreate(FB_PLUGIN_VERSION))
                {
                    plugin->setPluginFile(file);
                    plugin->init();

                    if (TwkMovie_GenericIO_debug)
		    {
			cerr << "INFO: plugin loaded: " << file
                             << " ID " << plugin->identifier()
                             << ", description '" << plugin->about()
                             << "'" << endl;

			//
			//  Check for _ARGS-style args env var
			//
			string var = plugin->identifier();
			transform (var.begin (), var.end (), var.begin (), (int(*)(int)) toupper);
			var = var + "_ARGS";
			if (getenv(var.c_str())) cerr << "INFO: " << var << ": " << getenv(var.c_str()) << endl;
		    }

                    return plugin;
                }
            }
            catch (...)
            {
                cerr << "WARNING: exception thrown while creating plugin "
                     << file
                     << ", ignoring"
                     << endl;

                dlclose(handle);
            }
        }
    }
    else
    {
        cerr << "ERROR: cannot open movie plugin " 
             << file
             << ": "
             << dlerror() << endl;
    }

    return 0;
}

void
GenericIO::loadPlugins(const string& envVar)
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
                cout << "INFO: loading plugin " << pluginFiles[i] 
                     << endl;

                if (MovieIO* plug = loadPlugin(pluginFiles[i]))
                {
                    addPlugin(plug);
                }
            }
        }

        m_loadedAll = true;
    }
}

bool
GenericIO::alreadyLoaded(const std::string& pluginFile)
{
    for (Plugins::iterator i = plugins().begin();
         i != plugins().end();
         ++i)
    {
        if ((*i)->pluginFile() == pluginFile) return true;
    }

    return false;
}

MovieIO*
GenericIO::loadFromProxy(Plugins::iterator i)
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
        cout << "WARNING: "
             << pio->pathToPlugin()
             << " failed to load"
             << endl;

        plugins().erase(i);
        delete pio;
        return 0;
    }
}

void
GenericIO::addPlugin(MovieIO* plugin)
{
    while (plugins().find(plugin) != plugins().end())
    {
        plugin->m_key += "_";
    }

    plugins().insert(plugin);
}

static bool
findNoCaseOnWindows (string s1, string s2)
{
    string news2 = s2;
    #ifdef PLATFORM_WINDOWS
	for (int i = 0; i < news2.size(); ++i)
	{
	    if (news2[i] == '\\') news2[i] = '/';
	    else                  news2[i] = toupper(news2[i]);
	}
    #endif

    return (string::npos != news2.find(s1));
}

static bool
QTmissing(bool print)
{
    static bool first = true;
    static bool QTnotInPath = false;

    if (first)
    {
        const char* path = getenv("PATH");
        if (path && !findNoCaseOnWindows("/QTSYSTEM", path)) QTnotInPath = true;
        first = false;
    }
    if (print && QTnotInPath) 
    {
	cerr << "ERROR: Please install Apple QuickTime, it is required to view movies on Windows." << endl;
    }

    return QTnotInPath;
}

const MovieIO *
GenericIO::findByExtension(const string& extension, 
                           unsigned int capabilities)
{
    if (!plugins().empty())
    {
        std::lock_guard<std::mutex> guard(plugin_mutex);
        for (bool restart = true; restart;)
        {
            restart = false;

            for (Plugins::iterator i = plugins().begin();
                 i != plugins().end();
                 ++i)
            {
                MovieIO* io = *i;

                if (io && io->supportsExtension(extension, capabilities))
                {
                    if (ProxyMovieIO* pio = dynamic_cast<ProxyMovieIO*>(io))
                    {
			if (TwkMovie_GenericIO_debug)
                        {
                            cout << "INFO: " << extension
                                 << " supported by plugin " << basename(pio->pathToPlugin())
                                 << endl;
                        }

                        if (io = loadFromProxy(i))
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

int
GenericIO::findAllByExtension(const string& extension, 
                              unsigned int capabilities,
                              MovieIOSet& ioSet)
{
    ioSet.clear();

    if (!plugins().empty())
    {
        std::lock_guard<std::mutex> guard(plugin_mutex);
        for (bool restart = true; restart;)
        {
            restart = false;

            for (Plugins::iterator i = plugins().begin();
                 i != plugins().end();
                 ++i)
            {
                MovieIO* io = *i;

                if (io && io->supportsExtension(extension, capabilities))
                {
                    if (ProxyMovieIO* pio = dynamic_cast<ProxyMovieIO*>(io))
                    {
			if (TwkMovie_GenericIO_debug)
                        {
                            cout << "INFO: " << extension
                                 << " supported by plugin " << basename(pio->pathToPlugin())
                                 << endl;
                        }

                        if (io = loadFromProxy(i))
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


const MovieIO*
GenericIO::findByBruteForce(const std::string& filename,
                            unsigned int capabilities)
{
    if (!plugins().empty())
    {
        cerr << "INFO: trying brute force to find an image reader for " 
             << basename(filename) << endl;

        std::lock_guard<std::mutex> guard(plugin_mutex);
        for (bool restart = true; restart;)
        {
            restart = false;

            for (Plugins::iterator i = plugins().begin();
                 i != plugins().end();
                 ++i)
            {
                #if defined(PLATFORM_WINDOWS) && defined(ARCH_IA32)
                    if ((*i)->identifier() == "Apple Quicktime" && QTmissing(false)) continue;
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
                                
                        cerr << "INFO: " << basename(filename)
                             << " is being read by: "
                             << (*i)->about()
                             << endl;
                                
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

MovieReader*
GenericIO::movieReader(const std::string& filename, bool tryBruteForce)
{
    unsigned int image = MovieIO::MovieRead;
    unsigned int audio = MovieIO::MovieReadAudio;
    string ext = extension(filename);

    MovieReader* m = 0;
    const MovieIO* io = 0;

    if (io = findByExtension(ext, image|audio))
    {
        m = io->movieReader();
    }
    else if (io = findByExtension(ext, image))
    {
        m = io->movieReader();
    }
    else if (io = findByExtension(ext, audio))
    {
        m = io->movieReader();
    }
    else if (tryBruteForce)
    {
        if (io = findByBruteForce(filename, image|audio))
        {
            m = io->movieReader();
        }
        else if (io = findByBruteForce(filename, image))
        {
            m = io->movieReader();
        }
        else if (io = findByBruteForce(filename, audio))
        {
            m = io->movieReader();
        }
    }

    if (m) io->setMovieAttributesOn(m);

    return m;
}

namespace 
{

bool
checkFDLimitOK(const string& filename)
{
#ifndef PLATFORM_WINDOWS
    int fd = ::open("/", O_RDONLY);

    if (fd == -1) 
    {
        if (errno == 24) 
        {
            struct rlimit rlim;
            getrlimit (RLIMIT_NOFILE, &rlim);
            TWK_THROW_EXC_STREAM("ERROR: Failed to open for reading: " << filename << endl << 
                                 "    The per-process limit on open files (" << rlim.rlim_cur << ") has" << endl <<
                                 "    been reached.  To open a larger number of media files, speak to your"  << endl << 
                                 "    IT person about increasing that limit.");
        }
    }
    else 
    {
        close(fd);
    }
#endif
    
    return true;
}

MovieReader*
tryOpen(const MovieIO* io, const std::string& filename, const MovieInfo& mi,
    Movie::ReadRequest request)
{
    //  cerr << "tryOpen '" << filename << "' with '" << io->about() << "'" << endl;
    
    MovieReader *m = io->movieReader();
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
            //delete m;
            m = 0;
            if (TwkMovie_GenericIO_debug)
            {
                cerr << "WARNING: " << io->identifier() << " failed to open "
                     << filename << ": " << exc.what() << endl;
            }
        }
        catch(...)
        {
            checkFDLimitOK(filename);
            //delete m;
            m = 0;
            if (TwkMovie_GenericIO_debug)
            {
                cerr << "WARNING: " << io->identifier() << " failed to open "
                     << filename << endl;
            }
        }
    }
    else
    {
        checkFDLimitOK(filename);
    }
    return m;
}

};

MovieReader*
GenericIO::openMovieReader(const std::string& filename, const MovieInfo& mi,
    Movie::ReadRequest& request, bool tryBruteForce) 
{
    unsigned int image = MovieIO::MovieRead;
    unsigned int audio = MovieIO::MovieReadAudio;
    string ext = extension(filename);
    if (ext == "") ext = basename(filename); // Try filename if there is no ext

    MovieReader* m = 0;
    MovieIOSet ioSet;
    if (findAllByExtension(ext, image|audio, ioSet) ||
        findAllByExtension(ext, image, ioSet) ||
        findAllByExtension(ext, audio, ioSet))
    {
        for (MovieIOSet::iterator mio = ioSet.begin(); mio != ioSet.end(); ++mio)
        {
            MovieInfo infoCopy = mi;
            if (m = tryOpen (*mio, filename, infoCopy, request)) return m;
        }
    }
    else if ( TwkUtil::pathIsURL( filename ) && 
              findAllByExtension( "mov", image|audio, ioSet) )
    {
        for (MovieIOSet::iterator mio = ioSet.begin(); mio != ioSet.end(); ++mio)
        {
            MovieInfo infoCopy = mi;
            if (m = tryOpen (*mio, filename, infoCopy, request)) return m;
        }
    }
    else if (tryBruteForce)
    {
        const MovieIO* io;
        if ((io = findByBruteForce(filename, image|audio)) ||
            (io = findByBruteForce(filename, image)) ||
            (io = findByBruteForce(filename, audio)))
        {
            if (m = tryOpen (io, filename, mi, request)) return m;
        }
    }

    TWK_THROW_EXC_STREAM("unsupported media type.");
}


MovieWriter* 
GenericIO::movieWriter(const string& filename)
{
    unsigned int image = MovieIO::MovieWrite;
    unsigned int audio = MovieIO::MovieWriteAudio;
    string ext = extension(filename);

    if (const MovieIO* io = findByExtension(ext, image|audio))
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

void
GenericIO::outputParameters(const MovieIO::ParameterVector& params,
                            const string& codec,
                            const string& header)
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

            cout << "            " << params[j].name 
                 << " " << params[j].description 
                 << endl;

            count++;
        }
    }
}

void 
GenericIO::outputFormats()
{
    const Plugins& mbps = plugins();
    
    for (Plugins::iterator i = plugins().begin();
         i != plugins().end();
         ++i)
    {
        const MovieIO::MovieTypeInfos& exts = 
            (*i)->extensionsSupported();

        for (int q=0; q < exts.size(); q++)
        {
            const MovieIO::MovieTypeInfo& info = exts[q];

            cout << "format \""
                 << info.extension << "\""
                 << " - "
                 << info.description
                 << " (" 
                 << info.capabilitiesAsString()
                 << ")"
                 << endl;

            outputParameters(info.encodeParameters, "", "encode parameters");
            outputParameters(info.decodeParameters, "", "decode parameters");

            if (info.codecs.size())
            {
                cout << "   image codecs: " << endl;
                    
                for (int j=0; j < info.codecs.size(); j++)
                {
                    cout << "      "
                         << info.codecs[j].first;

                    if (info.codecs[j].second != "")
                    {
                        cout << " (" << info.codecs[j].second
                             << ")";
                    }

                    cout << endl;

                    outputParameters(info.encodeParameters, 
                                     info.codecs[j].first,
                                     "encode parameters");

                    outputParameters(info.decodeParameters, 
                                     info.codecs[j].first,
                                     "decode parameters");
                }

                cout << endl;
            }

            if (info.audioCodecs.size())
            {
                cout << "   audio codecs: " << endl;
                    
                for (int j=0; j < info.audioCodecs.size(); j++)
                {
                    cout << "      "
                         << info.audioCodecs[j].first;

                    if (info.audioCodecs[j].second != "")
                    {
                        cout << " (" << info.audioCodecs[j].second
                             << ")";
                    }

                    cout << endl;

                    outputParameters(info.encodeParameters, 
                                     info.audioCodecs[j].first,
                                     "encode parameters");

                    outputParameters(info.decodeParameters, 
                                     info.audioCodecs[j].first,
                                     "decode parameters");
                }

                cout << endl;
            }
        }
    }
}



}  //  End namespace TwkFB

#ifdef _MSC_VER
#undef strcasecmp
#endif
