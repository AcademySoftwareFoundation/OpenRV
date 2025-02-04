//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkFB__IO__h__
#define __TwkFB__IO__h__
#include <TwkFB/dll_defs.h>
#include <TwkFB/FrameBuffer.h>
#include <vector>
#include <set>
#include <string>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

namespace TwkFB
{

    //
    //  Prototypes for plugin interface
    //  -------------------------------
    //
    //  If you implement a plugin, you will need to have a function called
    //  "create" which is extern "C" and has the signature:
    //
    //      extern "C" FrameBufferIO* create (float fb_plugin_version);
    //
    //  You will also need a function "destory" of type:
    //
    //      extern "C" void destroy(FrameBufferIO*);
    //
    //

#define FB_PLUGIN_VERSION 4.0

    //
    //  This struct is passed by FrameBufferIO::getImageInfo() and is also
    //  a base class for the Movie file info. In Movie.h
    //

    struct TWKFB_EXPORT FBInfo
    {
        //
        //  The union of image format concepts of a "channel", "layer",
        //  and "view" is a bit tricky. In EXR its not necessarily a
        //  simple hierarchy as one might expect. You can have channels
        //  that belong to no view or layer, channels expected to
        //  virtually belong to multiple layers (e.g. a single alpha that
        //  applies to all layers) and channels that belong to no view.
        //

        struct ChannelInfo
        {
            std::string name;
            FrameBuffer::DataType type;
        };

        typedef std::vector<std::string> StringVector;
        typedef std::vector<ChannelInfo> ChannelInfoVector;

        struct LayerInfo
        {
            std::string name;
            ChannelInfoVector channels; // pointers into
                                        // ChannelInfoVector
        };

        typedef std::vector<LayerInfo> LayerInfoVector;

        struct ViewInfo
        {
            std::string name;
            LayerInfoVector layers;
            ChannelInfoVector otherChannels; // pointers into
                                             // ChannelInfoVector
        };

        typedef std::vector<ViewInfo> ViewInfoVector;

        bool operator==(const TwkFB::FBInfo& i)
        {
            return width == i.width && height == i.height
                   && uncropWidth == i.uncropWidth
                   && uncropHeight == i.uncropHeight && uncropX == i.uncropX
                   && uncropY == i.uncropY && numChannels == i.numChannels
                   && !i.viewInfos.empty() && !this->viewInfos.empty()
                   && i.viewInfos.front().name == this->viewInfos.front().name;
        }

        bool operator!=(const TwkFB::FBInfo& i) { return !operator==(i); }

        FBInfo()
            : width(0)
            , height(0)
            , uncropWidth(0)
            , uncropHeight(0)
            , uncropX(0)
            , uncropY(0)
            , pixelAspect(1.0f)
            , numChannels(0)
            , dataType(FrameBuffer::__NUM_TYPES__)
            , orientation(FrameBuffer::BOTTOMLEFT)
        {
        }

        FBInfo(int w, int h, int n, FrameBuffer::DataType d,
               FrameBuffer::Orientation o)
            : width(w)
            , height(h)
            , uncropWidth(w)
            , uncropHeight(h)
            , uncropX(0)
            , uncropY(0)
            , pixelAspect(1.0f)
            , numChannels(n)
            , dataType(d)
            , orientation(o)
        {
        }

        FBInfo(int w, int h, int uw, int uh, int ux, int uy, int n,
               FrameBuffer::DataType d, FrameBuffer::Orientation o)
            : width(w)
            , height(h)
            , uncropWidth(uw)
            , uncropHeight(uh)
            , uncropX(ux)
            , uncropY(uy)
            , pixelAspect(1.0f)
            , numChannels(n)
            , dataType(d)
            , orientation(o)
        {
        }

        FBInfo(int w, int h, float pA, int n, FrameBuffer::DataType d,
               FrameBuffer::Orientation o)
            : width(w)
            , height(h)
            , uncropWidth(w)
            , uncropHeight(h)
            , uncropX(0)
            , uncropY(0)
            , pixelAspect(pA)
            , numChannels(n)
            , dataType(d)
            , orientation(o)
        {
        }

        FBInfo(int w, int h, int uw, int uh, int ux, int uy, float pA, int n,
               FrameBuffer::DataType d, FrameBuffer::Orientation o)
            : width(w)
            , height(h)
            , uncropWidth(uw)
            , uncropHeight(uh)
            , uncropX(ux)
            , uncropY(uy)
            , pixelAspect(pA)
            , numChannels(n)
            , dataType(d)
            , orientation(o)
        {
        }

        FBInfo& operator=(const FBInfo& i)
        {
            width = i.width;
            height = i.height;
            uncropWidth = i.uncropWidth;
            uncropHeight = i.uncropHeight;
            uncropX = i.uncropX;
            uncropY = i.uncropY;
            pixelAspect = i.pixelAspect;
            numChannels = i.numChannels;
            dataType = i.dataType;
            orientation = i.orientation;
            views = i.views;
            layers = i.layers;
            viewInfos = i.viewInfos;
            channelInfos = i.channelInfos;
            defaultView = i.defaultView;
            proxy.copyFrom(&i.proxy);
            return *this;
        }

        FBInfo(const FBInfo& i) { *this = i; }

        int width;
        int height;
        int uncropWidth;
        int uncropHeight;
        int uncropX;
        int uncropY;
        float pixelAspect;
        int numChannels;
        FrameBuffer::DataType dataType;
        FrameBuffer::Orientation orientation;
        StringVector views;
        std::string defaultView;
        StringVector layers;
        ChannelInfoVector channelInfos; // all channels
        ViewInfoVector viewInfos;       // view hierarchy
        FrameBuffer proxy;              /// maybe preview pixes and image attrs
    };

    //
    //  FrameBufferIO
    //
    //  Implement this class to support a new image type. If you are
    //  implementing a plugin. The "create" function should create a
    //  new one of these (your derived class).
    //

    class TWKFB_EXPORT FrameBufferIO
    {
    public:
        //
        //  Types
        //

        typedef std::pair<std::string, std::string> StringPair;
        typedef std::vector<StringPair> StringPairVector;
        typedef std::vector<std::string> StringVector;
        typedef std::vector<FrameBuffer*> FrameBufferVector;
        typedef std::vector<const FrameBuffer*> ConstFrameBufferVector;

        enum Capabilities
        {
            NoCapability = 0,

            AttributeRead = 1 << 0,  // can read attributes
            AttributeWrite = 1 << 1, // can write attributes

            ImageRead = 1 << 2,       // read whole image info fb
            ImageWrite = 1 << 3,      // write whole image info fb
            ProxyRead = 1 << 4,       // can read small RGB version
            PlanarRead = 1 << 5,      // can read planar images
            PlanarWrite = 1 << 6,     // can write planar images
            MultiResolution = 1 << 7, // can read (or generate) multiple
                                      // resolutions
            CropRead = 1 << 8,        // can read a crop of the file

            Int8Capable = 1 << 9,
            Int16Capable = 1 << 10,
            Float16Capable = 1 << 11,
            Float32Capable = 1 << 12,
            Float64Capable = 1 << 13,

            MultiImage = 1 << 14, // can handle many images in one file
            MultiLayer = 1 << 15, // multiple named image layers
                                  // (implies MultiImage)

            BruteForceIO = 1 << 16, // Safe if brute force I/O attempted
            AnyCapabilities = ~0x0
        };

        //
        //  Description of the image type. Extension + Description +
        //  Capabilities
        //

        struct ImageTypeInfo
        {
            ImageTypeInfo(const std::string& ext, const std::string& desc,
                          unsigned int c)
                : extension(ext)
                , description(desc)
                , capabilities(c)
            {
            }

            ImageTypeInfo(const std::string& ext, const std::string& desc,
                          unsigned int c, const StringPairVector& compressors);

            ImageTypeInfo(const std::string& ext, const std::string& desc,
                          unsigned int c, const StringPairVector& compressors,
                          const StringPairVector& decodeParams,
                          const StringPairVector& encodeParams);

            bool operator==(const std::string& e)
            {
                return !strcasecmp(e.c_str(), extension.c_str());
            }

            std::string extension;
            std::string description;
            unsigned int capabilities;
            StringPairVector compressionSchemes;
            StringPairVector decodeParameters; // (name, description)
            StringPairVector encodeParameters; // (name, description)

            std::string capabilitiesAsString() const;
        };

        typedef std::vector<ImageTypeInfo> ImageTypeInfos;

        struct ReadRequest
        {
            ReadRequest()
                : allChannels(false)
                , useInputStructure(false)
                , preferSubsampledPlanar(false)
                , bruteForce(false)
                , x0(0)
                , y0(0)
                , x1(0)
                , y1(0)
                , resolution(0.0f)
            {
            }

            bool allChannels;            // read all channels in each layer/view
            bool useInputStructure;      // use input fb structure
            bool preferSubsampledPlanar; // if possible

            //
            //  If true, this is a brute force read attempt and so the
            //  file extensions probably do not match the content.
            //

            bool bruteForce;

            //
            //  To read a subset or derezed version of the image you can
            //  set these to non-0. If the reader does succeed in reading
            //  a subset than the returned FB's identifier hash should
            //  include that information.
            //
            //  Resolution is in the range (0.0, 1.0]. A non 1.0 value
            //  means the request is asking for a scaled down version of
            //  the image.
            //
            //  NOTE: asking for a smaller resolution or crop of the image
            //  is reader dependent. Not all readers are expected to have
            //  this ability.
            //

            float resolution;

            int x0;
            int y0;
            int x1;
            int y1;

            //
            //  If either is non-empty return the layer/view
            //  requested. Note: layers are things like diffuse, views are
            //  things like left or right eye. Most of this is motivated
            //  by EXR. These should correspond to the views and layers
            //  advertised by the image info structure.
            //
            //  If channels is not empty, select those channels with the
            //  constraint that they come from any layers/views if they
            //  are also present.
            //

            StringVector layers;
            StringVector views;
            StringVector channels;

            //
            //  If decode parameters exist
            //

            StringPairVector parameters;
        };

        struct WriteRequest
        {
            WriteRequest()
                : keepPlanar(false)
                , keepColorSpace(true)
                , preferCommonFormat(true)
                , quality(0.8)
                , pixelAspect(1.0)
            {
            }

            bool preferCommonFormat; // use the most "common" form (could
                                     // override above)
            bool keepPlanar;         // don't convert to packed pixels
            bool keepColorSpace;     // don't convert to REC-709 primaries if
                                     // possible
            std::string compression; // compression scheme
            float quality;           // 0 -> 1 for lossy quality
            float pixelAspect; // if not 1.0, add to file metadata if format
                               // permits
            StringVector args; // writer extra args
            StringPairVector parameters;
        };

        //
        //  Constructor
        //

        FrameBufferIO(const std::string& identifier = "",
                      const std::string& key = "mz");
        virtual ~FrameBufferIO();

        //
        //  About this "plugin"
        //

        virtual std::string about() const;

        const std::string& identifier() const { return m_identifier; }

        const std::string& sortKey() const { return m_key; }

        //
        //  ImageRead capability says you can call these. May throw
        //  exception.
        //
        //  readImage() reads image in the file (for files that can only
        //  hold a single layer/view). The reader should use the FB passed
        //  in. Depending on the request, the reader may restructure the
        //  fb.
        //
        //  readImages() is the more general (optional) call. It should
        //  fill the vector of FBs with all of the requested images out of
        //  the file. You only need to implement this function if the
        //  image format supports multiple layers/views like EXR or
        //  TIFF. If you don't need it, implement readImage() instead.
        //  Like readImage(), the reader should use any passed in FBs. If
        //  necessary, the reader can restructure these fbs. If more fbs
        //  are required, add them.
        //
        //  NOTE: The "View" attribute should be set by the reader for
        //  "left" or "right" for stereo.
        //
        //  NOTE: The "Layer" attribute should be set by the reader
        //  for image layers other than the default: for example "diffuse"
        //  or "specular".
        //
        //  NOTE: The multiple image interface is NOT intended to handle
        //  movie file formats. There's a completely different set of
        //  classes for time based images!
        //

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;

        virtual void readImages(FrameBufferVector& fbs,
                                const std::string& filename,
                                const ReadRequest& request) const;

        //
        //  ImageWrite capabaility says you can call these.  May throw
        //  exception. The writer should respect the "View" and
        //  "Layer" attributes.
        //
        //  You do not need to implement writeImages() if the write
        //  capabaility does not indicate multiple image handling (e.g,
        //  EXR and TIFF both support multiple images in a file). The
        //  default implementation will check to make sure only a single
        //  image is being written.
        //
        //  Implement writeImage() if the reader only handles a single
        //  image per-file.
        //

        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;

        virtual void writeImages(const ConstFrameBufferVector& fbs,
                                 const std::string& filename,
                                 const WriteRequest& request) const;

        //
        //  This is a convenience function which will make a
        //  ConstFrameBufferVector for you out of of a FrameBufferVector
        //  and call the above.
        //

        void writeImages(const FrameBufferVector& fbs,
                         const std::string& filename,
                         const WriteRequest& request) const;

        //
        //  Returns a list of lower-case extensions the plugin can support
        //

        const ImageTypeInfos& extensionsSupported() const;
        bool
        supportsExtension(std::string extension,
                          unsigned int capabilities = AnyCapabilities) const;

        //
        //  Do any of this plugins accepted formats allow for attempting
        //  brute force reading?
        //

        bool canAttemptBruteForceRead() const;

        //
        //  Returns info about image in given filename, preferably WITHOUT
        //  reading the whole image. If the reader cannot get information
        //  on the image (its not the right format) than it should throw:
        //
        //      std::invalid_argument
        //
        //  This function will be called if the IO system is hunting
        //  around for a reader that can handle an unknown or possibly
        //  misnamed file (for example a tif file called .jpg).
        //

        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

        //
        //  Set the plugin file that created this
        //

        void setPluginFile(const std::string& file) { m_pluginFile = file; }

        const std::string& pluginFile() const { return m_pluginFile; }

        //
        //  "values" are named fields in the derived classes. Since we
        //  can't look into their headers but we know they exist use this
        //  dynamic API to get at them
        //

        virtual bool getBoolAttribute(const std::string& name) const;
        virtual void setBoolAttribute(const std::string& name, bool value);
        virtual int getIntAttribute(const std::string& name) const;
        virtual void setIntAttribute(const std::string& name, int value);
        virtual std::string getStringAttribute(const std::string& name) const;
        virtual void setStringAttribute(const std::string& name,
                                        const std::string& value);
        virtual double getDoubleAttribute(const std::string& name) const;
        virtual void setDoubleAttribute(const std::string& name,
                                        double value) const;

    protected:
        void addType(const std::string&, const std::string&, unsigned int);

        void addType(const std::string&, const std::string&, unsigned int,
                     const StringPairVector& compressors);

        void addType(const std::string&, const std::string&, unsigned int,
                     const StringPairVector& compressors,
                     const StringPairVector& dparams,
                     const StringPairVector& eparams);

        void addType(const std::string&, unsigned int);

        ImageTypeInfos m_exts;
        std::string m_pluginFile;
        std::string m_identifier;
        std::string m_key;

        friend class GenericIO;
    };

    /// Proxy for lazy load image i/o plugins

    class TWKFB_EXPORT ProxyFBIO : public FrameBufferIO
    {
    public:
        ProxyFBIO(const std::string& identifier, const std::string& sortKey,
                  const std::string& realPlugin)
            : FrameBufferIO(identifier, sortKey)
            , m_file(realPlugin)
        {
        }

        virtual ~ProxyFBIO() {}

        virtual std::string about() const;

        void add(const std::string& ext, const std::string& desc,
                 unsigned int capabilities, const StringPairVector& codecs,
                 const StringPairVector& dparams,
                 const StringPairVector& eparams)
        {
            addType(ext, desc, capabilities, codecs, dparams, eparams);
        }

        const std::string& pathToPlugin() const { return m_file; }

    private:
        std::string m_file;
    };

#ifndef TWK_PUBLIC
    //
    //  class GenericIO
    //
    //  This class should not be instantiated. Just call the static functions.
    //

    class TWKFB_EXPORT GenericIO
    {
    public:
        //
        //  Types
        //

        struct IOComp
        {
            bool operator()(const FrameBufferIO* a,
                            const FrameBufferIO* b) const
            {
                return a->sortKey().compare(b->sortKey()) < 0;
            }
        };

        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef std::vector<FrameBuffer*> FrameBufferVector;
        typedef std::vector<const FrameBuffer*> ConstFrameBufferVector;
        typedef std::set<FrameBufferIO*, IOComp> Plugins;
        typedef std::vector<std::string> StringVector;
        typedef std::set<std::string> ExtensionSet;

        //
        //  Initialize plugins;
        //  This needs to be called in the application main thread in main.
        //

        static void init();

        //
        //  Shutdown and free plugins;
        //  This needs to be called in the application main thread in main.
        //

        static void shutdown();

        //
        //  Get *all* the available plugins
        //

        static const Plugins& allPlugins();

        //
        //  Find appropriate plugin & read image file.  Throws exception
        //  on error
        //

        static void readImages(FrameBufferVector& fbs,
                               const std::string& filename,
                               const FrameBufferIO::ReadRequest& request,
                               const char* ext = 0);

        //
        //  Find appropriate plugin (based on extension) & write image
        //  file.  If format is specified, it is used INSTEAD of filename
        //  extension. Throws exception on error
        //

        static void writeImages(const ConstFrameBufferVector& fbs,
                                const std::string& filename,
                                const FrameBufferIO::WriteRequest& request,
                                const char* fmt = 0);

        static void writeImages(const FrameBufferVector& fbs,
                                const std::string& filename,
                                const FrameBufferIO::WriteRequest& request,
                                const char* fmt = 0);

        //
        //  If you want to statically link in plugins call this to add them
        //  from your main
        //

        static void addPlugin(FrameBufferIO*);

        //
        //  Find a specific IO object
        //

        static const FrameBufferIO* findByExtension(
            const std::string& extension,
            unsigned int capabilities = FrameBufferIO::AnyCapabilities);

        //
        //  Find a specific IO object by brute force: call getImageInfo()
        //  on each IO plugin in order until one doesn't throw and return
        //  that one.
        //

        static const FrameBufferIO* findByBruteForce(
            const std::string& filename,
            unsigned int capabilities = FrameBufferIO::AnyCapabilities);

        //
        //  Load a specific plugin file
        //

        static FrameBufferIO* loadPlugin(const std::string& pathToPlugin);

        //
        //  Load all on-disk plugins
        //

        static void loadPlugins(const std::string& envVar);

        //
        //  Returns true if the plugin has already been loaded
        //

        static bool alreadyLoaded(const std::string& plugfile);

        //
        //  Finds a list of potential TwkFB*.so plugins
        //

        static StringVector getPluginFileList(const std::string& envVar);

        //
        //  Compiles a set of extenions from all of the current plugins
        //  including upper case versions
        //

        static void compileExtensionSet(ExtensionSet&);

    private:
        GenericIO() {}

        static FrameBufferIO* loadFromProxy(Plugins::iterator);
        static bool m_loadedAll;

        static Plugins& plugins();

        static Plugins* m_plugins;
    };
#endif

} // namespace TwkFB

#endif // __TwkFB__IO__h__

#ifdef _MSC_VER
#undef strcasecmp
#endif
