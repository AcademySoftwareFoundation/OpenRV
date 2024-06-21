//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#include <TwkFB/IO.h>
#include <TwkFB/Exception.h>
#include <TwkUtil/File.h>
#include <iostream>
#include <assert.h>
#include <stdarg.h>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <dlfcn.h>
#include <sys/types.h>
#include <mutex>

/* AJG - stricmp */
#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

static std::mutex plugin_mutex;

static bool TwkFB_GenericIO_debug = false;
TWKFB_EXPORT void TwkFB_GenericIO_setDebug(bool b)
{
    TwkFB_GenericIO_debug = b;
}

namespace TwkFB {
using namespace std;
using namespace TwkUtil;

typedef FrameBufferIO* create_t(float);
typedef void destroy_t(FrameBufferIO*);

static const char* capabilityNames[] = 
{
    "AttributeRead", "AttributeWrite",
    "ImageRead", "ImageWrite", "ProxyRead",
    "PlanarRead", "PlanarWrite", "TileRead", "TileWrite",
    "Int8Capable", "Int16Capable", 
    "Float16Capable", "Float32Capable", "Float64Capable",
    "AnyCapabilities"
};

FrameBufferIO::ImageTypeInfo::ImageTypeInfo(const std::string& ext, 
                                            const std::string& desc,
                                            unsigned int c,
                                            const StringPairVector& compressors)
    : extension(ext), 
      description(desc),
      capabilities(c) 
{
    copy(compressors.begin(),
         compressors.end(),
         back_inserter(compressionSchemes));
}

FrameBufferIO::ImageTypeInfo::ImageTypeInfo(const std::string& ext, 
                                            const std::string& desc,
                                            unsigned int c,
                                            const StringPairVector& compressors,
                                            const StringPairVector& encodeParams,
                                            const StringPairVector& decodeParams)
    : extension(ext), 
      description(desc),
      capabilities(c),
      compressionSchemes(compressors),
      encodeParameters(encodeParams),
      decodeParameters(decodeParams)
{
}

std::string
FrameBufferIO::ImageTypeInfo::capabilitiesAsString() const
{
    ostringstream str;

    for (int i=0, count=0; i < 11; i++)
    {
        if (capabilities & (1 << i)) 
        {
            if (count) str << ", ";
            str << capabilityNames[i];
            count++;
        }
    }

    if (!compressionSchemes.empty())
    {
        str << ", compressors = ";

        for (int i=0; i < compressionSchemes.size(); i++)
        {
            if (i) str << ", ";
            str << compressionSchemes[i].first;

            if (compressionSchemes[i].second != "")
            {
                str << " (" << compressionSchemes[i].second << ")";
            }
        }
    }

    return str.str();
}


FrameBufferIO::FrameBufferIO(const string& id, const string& k)
    : m_identifier(id),
      m_key(k)
{
}
    
FrameBufferIO::~FrameBufferIO()
{
}

void
FrameBufferIO::addType(const std::string& e, 
                       const std::string& d,
                       unsigned int c)
{
    m_exts.push_back( ImageTypeInfo(e,d,c) );
}

void
FrameBufferIO::addType(const std::string& e, 
                       const std::string& d,
                       unsigned int c,
                       const StringPairVector& comps)
{
    m_exts.push_back( ImageTypeInfo(e,d,c,comps) );
}

void
FrameBufferIO::addType(const std::string& e, 
                       const std::string& d,
                       unsigned int c,
                       const StringPairVector& comps,
                       const StringPairVector& dparams,
                       const StringPairVector& eparams)
{
    m_exts.push_back( ImageTypeInfo(e,d,c,comps,dparams,eparams) );
}

void
FrameBufferIO::addType(const std::string& e, 
                       unsigned int c)
{
    m_exts.push_back( ImageTypeInfo(e,"",c) );
}

void 
FrameBufferIO::readImage(FrameBuffer& fb,
                         const string& filename,
                         const ReadRequest& request) const
{
    throw UnsupportedException();
}

void 
FrameBufferIO::readImages(FrameBufferVector& fbs,
                          const string& filename,
                          const ReadRequest& request) const
{
    if (fbs.empty()) fbs.push_back(new FrameBuffer());
    readImage(*fbs.front(), filename, request);
}

void 
FrameBufferIO::writeImage(const FrameBuffer& img,
                          const string& filename,
                          const WriteRequest& request) const
{
    throw UnsupportedException();
}

void 
FrameBufferIO::writeImages(const ConstFrameBufferVector& fbs,
                           const string& filename,
                           const WriteRequest& request) const
{
    if (fbs.size() == 1)
    {
        writeImage(*fbs.front(), filename, request);
    }
    else
    {
        throw UnsupportedException();
    }
}

void 
FrameBufferIO::writeImages(const FrameBufferVector &fbs,
                           const std::string& filename,
                           const WriteRequest& request) const
{
    ConstFrameBufferVector outfbs(fbs.size());
    copy(fbs.begin(), fbs.end(), outfbs.begin());
    writeImages(outfbs, filename, request);
}

string FrameBufferIO::about() const
{
    return "(no info available)";
}

const FrameBufferIO::ImageTypeInfos&
FrameBufferIO::extensionsSupported() const
{
    return m_exts;
}

bool 
FrameBufferIO::supportsExtension(std::string extension,
                                 unsigned int capabilities) const
{
    for (int i=0; i < m_exts.size(); i++)
    {
        if (!strcasecmp(m_exts[i].extension.c_str(), extension.c_str()))
        {
            if (capabilities == AnyCapabilities) return true;
            if ((m_exts[i].capabilities & capabilities) == capabilities) return true;
        }
    }

    return false;
}

bool 
FrameBufferIO::canAttemptBruteForceRead() const
{
    for (int i=0; i < m_exts.size(); i++)
    {
        if (m_exts[i].capabilities & BruteForceIO) return true;
    }

    return false;
}

void 
FrameBufferIO::getImageInfo(const string& filename, FBInfo& info) const
{
    info.width  = 0;
    info.height = 0;
}

bool FrameBufferIO::getBoolAttribute(const std::string& name) const { return false; }
void FrameBufferIO::setBoolAttribute(const std::string& name, bool value) { }
int FrameBufferIO::getIntAttribute(const std::string& name) const { return false; }
void FrameBufferIO::setIntAttribute(const std::string& name, int value) { }
string FrameBufferIO::getStringAttribute(const std::string& name) const { return ""; }
void FrameBufferIO::setStringAttribute(const std::string& name, const std::string& value) { }
double FrameBufferIO::getDoubleAttribute(const std::string& name) const { return 0.0; }
void FrameBufferIO::setDoubleAttribute(const std::string& name, double value) const {}

GenericIO::Plugins* GenericIO::m_plugins = 0;
bool GenericIO::m_loadedAll = false;

string ProxyFBIO::about() const
{
    ostringstream str;
    str << "Proxy for " << identifier() << " - key: " << sortKey();
    return str.str();
}

void 
GenericIO::init()
{
    if (!m_plugins) m_plugins = new Plugins;
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
    return findInPath(".*\\.so", pluginPath);
#endif

#ifdef PLATFORM_WINDOWS
    return findInPath(".*\\.dll", pluginPath);
#endif

#ifdef PLATFORM_APPLE_MACH_BSD
    return findInPath(".*\\.dylib", pluginPath);
#endif
}

FrameBufferIO*
GenericIO::loadPlugin(const string& file)
{
    if (void *handle = dlopen(file.c_str(), RTLD_LAZY))
    {
        create_t*  plugCreate  = (create_t *)dlsym( handle, "create" );
        destroy_t* plugDestroy = (destroy_t *)dlsym( handle, "destroy" );
            
        if (!plugCreate || !plugDestroy)
        {
            dlclose( handle );
            
            cerr << "ERROR: ignoring FB plugin " << file 
                 << ": missing create() or destroy(): "
                 << dlerror() << endl;
        }
        else
        {
            try
            {
                if (FrameBufferIO *plugin = plugCreate(FB_PLUGIN_VERSION))
                {
                    plugin->setPluginFile(file);

		    if (TwkFB_GenericIO_debug)
		    {
			cerr << "INFO: plugin loaded:  " << file
                             << ", ID " << plugin->identifier()
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
        cerr << "ERROR: cannot open fb plugin " 
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

                if (FrameBufferIO* plug = loadPlugin(pluginFiles[i]))
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

void
GenericIO::addPlugin(FrameBufferIO* plugin)
{
    while (plugins().find(plugin) != plugins().end())
    {
        plugin->m_key += "_";
    }

    plugins().insert(plugin);
}

FrameBufferIO*
GenericIO::loadFromProxy(Plugins::iterator i)
{
    ProxyFBIO* pio = dynamic_cast<ProxyFBIO*>(*i);

    if (FrameBufferIO* newio = loadPlugin(pio->pathToPlugin()))
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

const FrameBufferIO *
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
                FrameBufferIO* io = *i;
        
                if (io && io->supportsExtension(extension, capabilities))
                {
                    if (ProxyFBIO* pio = dynamic_cast<ProxyFBIO*>(io))
                    {
                        if (TwkFB_GenericIO_debug)
                        {
                            cout << "INFO: " << extension
                                 << " requires plugin " << basename(pio->pathToPlugin())
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

const FrameBufferIO*
GenericIO::findByBruteForce(const std::string& filename,
                            unsigned int capabilities)
{
    //if (!m_loadedAll) loadPlugins("TWK_FB_PLUGIN_PATH");

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
                if (*i && (*i)->canAttemptBruteForceRead())
                {
                    if (dynamic_cast<ProxyFBIO*>(*i))
                    {
                        loadFromProxy(i);
                        restart = true;
                        break; // restart
                    }

                    try
                    {
                        FBInfo info;

                        (*i)->getImageInfo(filename, info);
                                
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

void
GenericIO::readImages(FrameBufferVector& fbs,
                      const std::string& filename, 
                      const FrameBufferIO::ReadRequest& request,
                      const char* fmt)
{
    string format;
    if (fmt) format = fmt;
    else format = extension(filename);

    if (const FrameBufferIO* io = findByExtension(format, FrameBufferIO::ImageRead))
    {
        io->readImages(fbs, filename, request);
    }
    else
    {
        throw UnsupportedException();
    }
}

void
GenericIO::writeImages(const ConstFrameBufferVector& fbs, 
                       const string& filename, 
                       const FrameBufferIO::WriteRequest& request,
                       const char *fmt )
{
    string format = fmt ? fmt : extension(filename);

    if (const FrameBufferIO* io = findByExtension(format, FrameBufferIO::ImageWrite))
    {
        io->writeImages(fbs, filename, request);
    }
    else
    {
        throw UnsupportedException();
    }
}

void
GenericIO::writeImages(const FrameBufferVector& fbs, 
                       const string& filename, 
                       const FrameBufferIO::WriteRequest& request,
                       const char *fmt )
{
    string format = fmt ? fmt : extension(filename);

    if (const FrameBufferIO* io = findByExtension(format, FrameBufferIO::ImageWrite))
    {
        io->writeImages(fbs, filename, request);
    }
    else
    {
        throw UnsupportedException();
    }
}

void
GenericIO::compileExtensionSet(ExtensionSet& exts)
{
    if (!plugins().empty())
    {
        for (Plugins::iterator i = plugins().begin();
             i != plugins().end();
             ++i)
        {
            const FrameBufferIO::ImageTypeInfos& ins = (*i)->extensionsSupported();

            for (int q=0; q < ins.size(); q++)
            {
                string ext = ins[q].extension;
                exts.insert(ext);

                for (int j=0; j < ext.size(); j++)
                {
                    if (islower(ext[j])) ext[j] = toupper(ext[j]);
                }
            
                exts.insert(ext);
            }
        }
    }
}


}  //  End namespace TwkFB

#ifdef _MSC_VER
#undef strcasecmp
#endif
