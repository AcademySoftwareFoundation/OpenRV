//******************************************************************************
// Copyright (c) 2005 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifndef __TwkMovie__MovieIO__h__
#define __TwkMovie__MovieIO__h__
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/MovieReader.h>
#include <TwkMovie/MovieWriter.h>
#include <TwkMovie/dll_defs.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

namespace TwkMovie {

/// Provides class instances for generic reading and writing

///
/// By implemented a MovieIO object for your MovieReader and
/// MovieWriter classes, the GenericIO class can automatically
/// determine which classes to use by looking at the file extension or
/// possible magic numbers in the file (if your MovieReader can do
/// that).
///
/// Each format that your classes can handle is given a set a
/// capabilities such as read/write which are used to select a plugin.
/// 

class TWKMOVIE_EXPORT MovieIO
{
public:
    //
    //  Types
    //

    struct Parameter
    {
        Parameter(const std::string& n,
                  const std::string& d,
                  const std::string& c)
            : name(n), description(d), codec(c) {}
        Parameter() {}

        std::string name;
        std::string description;
        std::string codec;
    };

    typedef std::pair<std::string, std::string> StringPair;
    typedef std::vector<StringPair>             StringPairVector;
    typedef std::vector<std::string>            StringVector;
    typedef std::vector<Parameter>              ParameterVector;
    typedef std::map<std::string, bool>         BoolMap;
    typedef std::map<std::string, int>          IntMap;
    typedef std::map<std::string, std::string>  StringMap;
    typedef std::map<std::string, double>       DoubleMap;

    /// Bit fields which when packed together describe class capabilities

    enum Capabilities
    {
        NoCapability      = 0,
        AttributeRead     = 1 << 0, /// can read attributes
        AttributeWrite    = 1 << 1, /// can write attributes
        MovieRead         = 1 << 2, /// read whole image info fb
        MovieWrite        = 1 << 3, /// write whole image info fb
        MovieReadAudio    = 1 << 4,
        MovieWriteAudio   = 1 << 5,
        MovieMultiTrack   = 1 << 6, /// can handle multiple tracks
        MovieBruteForceIO = 1 << 7,

        AnyCapability = 0xffffffff
    };

    ///
    /// Used internally to hold information about the file format.
    ///

    struct MovieTypeInfo
    {
        MovieTypeInfo(const std::string& ext, unsigned int c)
            : extension(ext),
              capabilities(c) {}

        MovieTypeInfo(const std::string& ext, 
                      const std::string& desc,
                      unsigned int c,
                      const StringPairVector& cdecs,
                      const StringPairVector& acdecs)
            : extension(ext), 
              description(desc),
              capabilities(c),
              codecs(cdecs),
              audioCodecs(acdecs) {}

        MovieTypeInfo(const std::string& ext, 
                      const std::string& desc,
                      unsigned int c,
                      const StringPairVector& cdecs,
                      const StringPairVector& acdecs,
                      const ParameterVector& eparams,
                      const ParameterVector& dparams)
            : extension(ext), 
              description(desc),
              capabilities(c),
              codecs(cdecs),
              audioCodecs(acdecs),
              encodeParameters(eparams),
              decodeParameters(dparams) {}

        bool operator == (const std::string& e) 
            { return !strcasecmp(e.c_str(), extension.c_str()); }

        std::string         extension;
        std::string         description;
        unsigned int        capabilities;
        StringPairVector    codecs;
        StringPairVector    audioCodecs;
        ParameterVector     decodeParameters; // (name, description)
        ParameterVector     encodeParameters; // (name, description)

        std::string  capabilitiesAsString() const;
    };

    typedef std::vector<MovieTypeInfo> MovieTypeInfos;

    //
    //  Constructor
    //

    MovieIO(const std::string& identifier="", const std::string& key="mz");
    virtual ~MovieIO();


    /// Initializes the plugin; 
    /// This call can be used to perform any initializations 
    /// tasks that might be needed by the plugin's SDK. 
    virtual void init() { };

    /// A string describing the plugin type and version

    virtual std::string about() const;

    const std::string& identifier() const { return m_identifier; }
    const std::string& sortKey() const { return m_key; }

    ///
    ///  MovieRead capabaility says you can call this May throw
    ///  exception.
    ///

    virtual MovieReader* movieReader() const;

    ///
    ///  MovieWrite capabaility says you can call this.
    ///  May throw exception.
    ///

    virtual MovieWriter* movieWriter() const;
    
    ///
    ///  Returns a list of lower-case extensions the plugin can
    ///  support
    ///

    const MovieTypeInfos& extensionsSupported() const;

    ///
    /// Query whether an extension is supported by this MovieIO.
    ///

    bool supportsExtension(std::string extension, 
                           unsigned int capabilities=AnyCapability) const;

    //
    //  Do any of this plugins accepted formats allow for attempting
    //  brute force reading? 
    //

    bool canAttemptBruteForceRead() const;
    
    ///
    /// Returns info about image in given filename, preferably reading
    /// only the movie header. This function should throw if it can't
    /// read the movie or the movie file is not compatible. The IO
    /// system will repeatedly call getMovieInfo() to find a reader
    /// that can handle files which it does not understand (for
    /// example a quicktime movie file with the wrong extension).
    ///

    virtual void getMovieInfo(const std::string& filename, MovieInfo&) const;

    //
    //  Set the plugin file that created this
    //

    void setPluginFile(const std::string& file)
        { m_pluginFile = file; }

    const std::string& pluginFile() const { return m_pluginFile; }


    //
    //  Set/Get default movie attribute values
    //

    bool getBoolAttribute(const std::string& name) const;
    void setBoolAttribute(const std::string& name, bool value);
    int getIntAttribute(const std::string& name) const;
    void setIntAttribute(const std::string& name, int value);
    std::string getStringAttribute(const std::string& name) const;
    void setStringAttribute(const std::string& name, const std::string& value);
    double getDoubleAttribute(const std::string& name) const;
    void setDoubleAttribute(const std::string& name, double value) const;

    void setMovieAttributesOn(Movie*) const;
    void copyAttributesFrom(const MovieIO*);

    const BoolMap& boolMap() const { return m_boolMap; }
    const IntMap& intMap() const { return m_intMap; }
    const StringMap& stringMap() const { return m_stringMap; }
    const DoubleMap& doubleMap() const { return m_doubleMap; }

protected:
    void addType(const std::string&, unsigned int);
    void addType(const std::string& extension, 
                 const std::string& description,
                 unsigned int capabilities,
                 const StringPairVector& videoCodecs,
                 const StringPairVector& audioCodecs);
    void addType(const std::string& extension, 
                 const std::string& description,
                 unsigned int capabilities,
                 const StringPairVector& videoCodecs,
                 const StringPairVector& audioCodecs,
                 const ParameterVector& encodeParams,
                 const ParameterVector& decodeParams);

    MovieTypeInfos m_exts;
    std::string m_identifier;
    std::string m_key;
    std::string m_pluginFile;
    mutable BoolMap   m_boolMap;
    mutable IntMap    m_intMap;
    mutable StringMap m_stringMap;
    mutable DoubleMap m_doubleMap;
    

    friend class GenericIO;
};

/// A proxy for lazy load movie plugins

class TWKMOVIE_EXPORT ProxyMovieIO : public MovieIO
{
public:
    ProxyMovieIO(const std::string& identifier,
                 const std::string& sortKey,
                 const std::string& realPlugin) 
        : MovieIO(identifier, sortKey), m_file(realPlugin) {}
    virtual ~ProxyMovieIO() {}
    virtual std::string about() const;

    void add(const std::string& ext,
             const std::string& desc,
             unsigned int capabilities,
             const StringPairVector& vcodecs,
             const StringPairVector& acodecs)
        { addType(ext, desc, capabilities, vcodecs, acodecs); }

    void add(const std::string& ext,
             const std::string& desc,
             unsigned int capabilities,
             const StringPairVector& vcodecs,
             const StringPairVector& acodecs,
             const ParameterVector& eparams,
             const ParameterVector& dparams)
        { addType(ext, desc, capabilities, vcodecs, acodecs, eparams, dparams); }

    const std::string& pathToPlugin() const { return m_file; }
    
private:
    std::string m_file;
};


/// Manages MovieIO plugins and selects one based on a file to read/write

///
/// This class should not be instantiated. Just call the static
/// functions.
///

class TWKMOVIE_EXPORT GenericIO
{
  public:
    //
    //  Types
    //

    struct IOComp
    {
        bool operator() (const MovieIO* a, const MovieIO* b) const
            { return a->sortKey().compare(b->sortKey()) < 0; }
    };

    typedef std::set<MovieIO*, IOComp>         Plugins;
    typedef std::vector<std::string>           StringVector;
    typedef std::set<const MovieIO*, IOComp>   MovieIOSet;

    ///
    ///  Initialize plugins statics.
    ///  This needs to be called in the application main thread in main.
    /// 

    static void init();

    ///
    ///  Shutdown and free generic movieio plugins.
    ///  This needs to be called in the application main thread in main.
    /// 

    static void shutdown();

    ///
    ///  Find appropriate plugin & read image file.  Throws exception
    ///  on error
    ///

    static MovieReader *movieReader(const std::string& filename,
                                    bool tryBruteForce=true);

    ///
    ///  Find appropriate plugin & read image file.  Throws exception
    ///  on error.  Returned MovieReader has been "opened".  This version
    ///  will "fall back" if more than one plugin claims support for the 
    ///  given filename.
    ///

    static MovieReader* openMovieReader(const std::string& filename, 
                                        const MovieInfo& mi,
                                        Movie::ReadRequest& request,
                                        bool tryBruteForce=true);
    ///
    ///  Find appropriate plugin (based on extension) & write image
    ///  file.  If format is specified, it is used INSTEAD of filename
    ///  extension. Throws exception on error
    ///

    static MovieWriter* movieWriter(const std::string& filename);

    ///
    ///  If you want to statically link in plugins call this to add them
    ///  from your main
    ///

    static void addPlugin(MovieIO*);

    ///
    /// All of the currently existing plugins
    ///

    static MovieIO* loadPlugin(const std::string& file);

    ///
    ///  Load all on-disk plugins
    ///

    static void loadPlugins(const std::string& envVar);

    //
    //  Returns true if the plugin has already been loaded
    //

    static bool alreadyLoaded(const std::string& plugfile);

    static MovieIO* loadFromProxy(Plugins::iterator i);

    ///
    /// All of the currently existing plugins
    ///

    static const Plugins& allPlugins();

    ///
    ///  Find a specific IO object
    ///

    static const MovieIO *findByExtension(const std::string& extension,
                                          unsigned int capabilities=MovieIO::AnyCapability);

    ///
    ///  Find all IO objects that report support for the given extension/cababilities.
    ///

    static int findAllByExtension(const std::string& extension,
                                  unsigned int capabilities,
                                  MovieIOSet& ioVector);


    ///
    ///  Find a specific IO object by brute force: call getMovieInfo()
    ///  on each IO plugin in order until one doesn't throw and return
    ///  that one.
    ///

    static const MovieIO* findByBruteForce(const std::string& filename,
                                           unsigned int capabilities=MovieIO::AnyCapability);

    ///
    ///  Finds a list of potential TwkFB*.so plugins
    ///

    static StringVector getPluginFileList(const std::string& envVar);

    ///
    ///  Output formats, compressors, etc to cout
    ///

    static void outputFormats();

    //
    //  Licensed DNxHD decoding.
    //

    static bool dnxhdDecodingAllowed();
    static void setDnxhdDecodingAllowed(bool b);

  private:
    GenericIO() {}
    static void outputParameters(const MovieIO::ParameterVector&,
                                 const std::string&,
                                 const std::string&);

    static Plugins& plugins();

  private:
    static Plugins* m_plugins;
    static bool m_loadedAll;
    static bool m_dnxhdDecodingAllowed;
};


} // TwkMovie

#endif // __TwkMovie__MovieIO__h__

#ifdef _MSC_VER
#undef strcasecmp 
#endif
