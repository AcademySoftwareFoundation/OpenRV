//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkFB__FrameBuffer__h__
#define __TwkFB__FrameBuffer__h__
#include <TwkFB/dll_defs.h>
#include <TwkFB/Attribute.h>
#include <TwkMath/Chromaticities.h>
#include <assert.h>
#include <atomic>
#include <ostream>
#include <string>
#include <vector>
#include <sstream>

namespace TwkFB
{

    //
    //  FrameBuffer
    //
    //  This class represents an image framebuffer -- a block of contiguous
    //  interleaved channel pixels.
    //
    //  FrameBuffer can only handle homogeneously typed channels. For example,
    //  you cannot have 3 float channels and one unsigned char.  But you can
    //  have 4 float channels or 3 unsigned char channels.
    //
    //  A planar image can be represented as a linked list of FrameBuffer
    //  objects. As such, the FrameBuffer at the head of the list represents
    //  the whole image. I.e. don't add attributes to a plane other than the
    //  first plane and expect the attributes to be used. Use appendPlane() to
    //  attach a FrameBuffer plane to an existing one. Its possible to have a
    //  contiguous chunk of memory containing multiple planes addressed by
    //  multiple FrameBuffer planes as long as only one of them "owns" the
    //  data.
    //
    //  Framebuffer may store a 3D "image", but currently no reformating or
    //  other operations operate on that data structure. You have to use the
    //  third constructor (which includes a depth parameter) to store the data
    //  in that format.
    //
    //  A Framebuffer can represent a crop of a larger image. This is called
    //  the "uncrop" image. The FB contains information about the uncrop image
    //  size and the placement of the pixels in the uncropped image. Some file
    //  formats (like EXR) may automatically set these values upon reading. The
    //  uncrop image is similar to the EXR concept of display window except
    //  that there is no separate origin for the display window; it always has
    //  its origin at (0,0).
    //
    //

    namespace ColorSpace
    {

        //
        //  The names of the main colorspace attributes. Other than the matrix,
        //  each of thes should have one of the ColorSpace values below.
        //
        //  The Primaries attribute is not used directly, it indicates the name
        //  of the color space primaries. E.g, Rec709. Images with non-Rec709
        //  primaries should include the actual values in
        //  Red/Green/Blue/WhitePrimaries attributes. The name becomes useful in
        //  conjunction with the actual values when there are multiple values
        //  for the primaries. By providing the name, users can see the intended
        //  space
        //  -- reasonable color people (!?) may differ in their interpretation
        //  of the exact primary values (for historic or histrionic reasons).
        //
        //  The TransferFunction indicates which non-linear (or not) function
        //  should be used to convert to linear. In the case of data thats
        //  already linear (e.g., exr) the value should be ColorSpace::Linear or
        //  ColorSpace::Identity. If TransferFunction is not defined its assumed
        //  to be ColorSpace::Identity.
        //
        //  Conversion is the name (like Rec709) of the conversion matrix that
        //  goes from YCbCr->RGB. This is similar to the Primaries attr in that
        //  it is not directly used. ConversionMatrix holds the actual matrix.
        //  The value Rec601Full is used exclusively as a conversion.
        //
        //  Note that these are tags on the FB there is no transform applied to
        //  the pixels when a tag is set on the FB.
        //
        TWKFB_EXPORT std::string Primaries(); // ColorSpace/Primaries
        TWKFB_EXPORT std::string
        TransferFunction();                    // ColorSpace/TransferFunction
        TWKFB_EXPORT std::string Conversion(); // ColorSpace/Conversion
        TWKFB_EXPORT std::string
        ChromaPlacement();                // ColorSpace/ChromaPlacement
        TWKFB_EXPORT std::string Range(); // ColorSpace/Range

        TWKFB_EXPORT std::string RedPrimary();     // ColorSpace/RedPrimary
        TWKFB_EXPORT std::string GreenPrimary();   // ColorSpace/GreenPrimary
        TWKFB_EXPORT std::string BluePrimary();    // ColorSpace/BluePrimary
        TWKFB_EXPORT std::string WhitePrimary();   // ColorSpace/WhitePrimary
        TWKFB_EXPORT std::string AdoptedNeutral(); // ColorSpace/AdoptedNeutral

        TWKFB_EXPORT std::string
        ConversionMatrix(); // ColorSpace/ConversionMatrix

        TWKFB_EXPORT std::string RGBtoXYZMatrix();  // ColorSpace/RGBtoXYZMatrix
        TWKFB_EXPORT std::string LinearScale();     // Simple scale
        TWKFB_EXPORT std::string FilmStyleMatrix(); // FilmStyle Matrix.
        TWKFB_EXPORT std::string
        FilmStyleInverseMatrix(); // Inverse of FilmStyle Matrix.

        //
        //  Color Space Values
        //
        //  The base colorspace as well as the transfer function could be one of
        //  these.
        //
        TWKFB_EXPORT std::string
        sRGB(); // IEC 61966-2-1 (for primaries same as Rec709)
        TWKFB_EXPORT std::string Rec709();  // ITU.BT-709 Y'CbCr
        TWKFB_EXPORT std::string Rec601();  // ITU.BT-601 Y'CbCr
        TWKFB_EXPORT std::string Rec2020(); // ITU.BT-2020 Y'CbCr
        TWKFB_EXPORT std::string
        AdobeRGB();                     // Adobe RGB space (for monitors mostly)
        TWKFB_EXPORT std::string P3();  // DCI P3
        TWKFB_EXPORT std::string XYZ(); // XYZ primaries
        TWKFB_EXPORT std::string ArriWideGamut(); // ARRI ALEXA Wide Gamut
        TWKFB_EXPORT std::string
        ArriSceneReferred(); // ARRI ALEXA Wide Gamut (scene referred)
        TWKFB_EXPORT std::string SonySGamut();   // SONY S-Gamut
        TWKFB_EXPORT std::string SMPTE_C();      // SMPTE-C
        TWKFB_EXPORT std::string SMPTE240M();    // SMPTE-240M
        TWKFB_EXPORT std::string CineonLog();    // Cineon/DPX Log
        TWKFB_EXPORT std::string ArriLogC();     // ARRI LogC
        TWKFB_EXPORT std::string ArriLogCFilm(); // ARRI LogC Film
        TWKFB_EXPORT std::string SonySLog();     // SONY S-Log
        TWKFB_EXPORT std::string ViperLog();     // Viper Camera
        TWKFB_EXPORT std::string RedSpace();  // Red Camera RedSpace primaries
        TWKFB_EXPORT std::string RedColor();  // Red Camera RedColor primaries
        TWKFB_EXPORT std::string RedColor2(); // Red Camera RedColor2 primaries
        TWKFB_EXPORT std::string RedColor3(); // Red Camera RedColor3 primaries
        TWKFB_EXPORT std::string RedColor4(); // Red Camera RedColor4 primaries
        TWKFB_EXPORT std::string
        DragonColor(); // Red Camera DragonColor primaries
        TWKFB_EXPORT std::string
        DragonColor2(); // Red Camera DragonColor2 primaries
        TWKFB_EXPORT std::string
        RedWideGamut();                    // Red Camera Wide Gamut primaries
        TWKFB_EXPORT std::string RedLog(); // Red Camera Log
        TWKFB_EXPORT std::string
        RedLogFilm(); // Red Camera Log Film (CineonLog version)
        TWKFB_EXPORT std::string Panalog(); // Panavision Camera
        TWKFB_EXPORT std::string Linear();  // Linear Light
        TWKFB_EXPORT std::string Gamma18(); // Gamma with exponent 1.8
        TWKFB_EXPORT std::string Gamma22(); // Gamma with exponent 2.2
        TWKFB_EXPORT std::string Gamma24(); // Gamma with exponent 2.4
        TWKFB_EXPORT std::string Gamma26(); // Gamma with exponent 2.6
        TWKFB_EXPORT std::string Gamma28(); // Gamma with exponent 2.8
        TWKFB_EXPORT std::string
        GammaExponent(); // Gamma with exponent ColorSpace/Gamma
        TWKFB_EXPORT std::string
        ACES(); // ACES (Academy Color Encoding Sepcification)
        TWKFB_EXPORT std::string CIEXYZ();     // CIE XYZ
        TWKFB_EXPORT std::string ICCProfile(); // Provided ICC Profile
        TWKFB_EXPORT std::string CTL();        // Color Transform Language
        TWKFB_EXPORT std::string Generic();  // Use available color space params
        TWKFB_EXPORT std::string None();     // Unknown
        TWKFB_EXPORT std::string Identity(); // Transfer function only
        TWKFB_EXPORT std::string NonColorData(); // FB is not really color data
        TWKFB_EXPORT std::string
        VideoRange(); // Video Range Video (16-235/240) in 8 bit
        TWKFB_EXPORT std::string
        FilmRange(); // Film Range Video (4-1019) in 10 bit
        TWKFB_EXPORT std::string FullRange();  // Full Range Video
        TWKFB_EXPORT std::string Left();       // ChromaPlacement
        TWKFB_EXPORT std::string Center();     // ChromaPlacement
        TWKFB_EXPORT std::string TopLeft();    // ChromaPlacement
        TWKFB_EXPORT std::string Top();        // ChromaPlacement
        TWKFB_EXPORT std::string BottomLeft(); // ChromaPlacement
        TWKFB_EXPORT std::string Bottom();     // ChromaPlacement

        //
        //  Log Params
        //
        TWKFB_EXPORT std::string BlackPoint(); // CineonLog Parameter
        TWKFB_EXPORT std::string WhitePoint(); // CineonLog Parameter
        TWKFB_EXPORT std::string
        BreakPoint(); // CineonLog Parameter (defines SoftClip)
        TWKFB_EXPORT std::string Rolloff(); //

        TWKFB_EXPORT std::string LogCBlackSignal();
        TWKFB_EXPORT std::string LogCEncodingOffset();
        TWKFB_EXPORT std::string LogCEncodingGain();
        TWKFB_EXPORT std::string LogCGraySignal();
        TWKFB_EXPORT std::string LogCBlackOffset();
        TWKFB_EXPORT std::string LogCLinearSlope();
        TWKFB_EXPORT std::string LogCLinearOffset();
        TWKFB_EXPORT std::string
        LogCLinearCutPoint(); // Cutoff used for LogC to Linear
        TWKFB_EXPORT std::string
        LogCCutPoint(); // Cutoff used for Linear to LogC

        //
        //  ICC data
        //
        TWKFB_EXPORT std::string
        ICCProfileDescription(); // ColorSpace/ICC Profile Description
        TWKFB_EXPORT std::string
        ICCProfileData(); // ColorSpace/ICC Profile Data
        TWKFB_EXPORT std::string
        ICCProfileVersion(); // ColorSpace/ICC Profile Version

        //
        //  CTL
        //
        TWKFB_EXPORT std::string
        CTLProgramName();                      // ColorSpace/CTL Program Name
        TWKFB_EXPORT std::string CTLProgram(); // ColorSpace/CTL Program

        //
        //  Gamma. This really should not be used. Quicktime/JPEG needs it.
        //
        TWKFB_EXPORT std::string Gamma(); // ColorSpace/Gamma

    } // namespace ColorSpace

    class TWKFB_EXPORT FrameBuffer
    {
    public:
        //
        //  Types
        //

        typedef std::vector<std::string> StringVector;
        typedef std::vector<FBAttribute*> AttributeVector;
        typedef std::stringstream HashStream;
        typedef std::vector<int> Samplings;
        typedef unsigned int UInt;
        typedef unsigned int U32;
        typedef TwkMath::Chromaticities<float> Chromaticities;

        //
        // Image data type
        //
        enum CoordinateTypes
        {
            NormalizedCoordinates,
            PixelCoordinates
        };

        //
        //  Data type for pixel channels.
        //
        //  NOTE: Some pieces of code depend on the order of DataType (use
        //  > and < on the elements)
        //
        //  PACKED_R10_G10_B10_X2 is the GL version of Cineon 10 bit.
        //  PACKED_X2_B10_G10_R10 is the same but backwards. This is the
        //  preferred 10 bit format.
        //
        //  PACKED_Cb8_Y8_Cr8_Y8 is a storage format (2vuy or YUVS) which
        //  quicktime can generate.  The data type is always single
        //  channel, the pixel size is sizeof(short). The FB should be set
        //  to REC_709 (think of the pixels as being compressed
        //  REC_709). copyConvert() to UCHAR will generate REC_709 3
        //  channel R G B.
        //

        enum DataType
        {
            BIT,
            UCHAR,
            USHORT,
            UINT,
            HALF,
            FLOAT,
            DOUBLE,

            PACKED_R10_G10_B10_X2, // these are all 1 channel (hence PACKED)
            PACKED_X2_B10_G10_R10,
            PACKED_Cb8_Y8_Cr8_Y8,
            PACKED_Y8_Cb8_Y8_Cr8,

            __NUM_TYPES__
        };

        static size_t sizeOfDataType(DataType);

        //
        //  Orientation: the image is always treated as if its BOTTOMLEFT
        //  (bottom-left origin). So pixel look ups will not take this
        //  into account. This is mostly for display purposes. Some APIs,
        //  like quicktime, always make TOPLEFT images. TIFF files can be
        //  any one of the four orientations.
        //
        //  3D "images" are currently considered front->back in depth
        //  orientation.
        //

        enum Orientation
        {
            NATURAL,     // bottom left origin
            TOPLEFT,     // top-left origin
            TOPRIGHT,    // Oiy
            BOTTOMRIGHT, // Oiy
            __NUM_ORIENTATION__,
            BOTTOMLEFT = NATURAL
        };

        //
        //  10 Bit packed pixel structure
        //

        struct Pixel10
        {
            union
            {
                struct
                {
#if defined(__LITTLE_ENDIAN__)
                    UInt unused : 2;
                    UInt blue : 10;
                    UInt green : 10;
                    UInt red : 10;
#else
                    UInt red : 10;
                    UInt green : 10;
                    UInt blue : 10;
                    UInt unused : 2;
#endif
                };

                struct
                {
#if defined(__LITTLE_ENDIAN__)
                    UInt unused2 : 2;
                    UInt blue_least : 2;
                    UInt blue_most : 8;
                    UInt green_least : 2;
                    UInt green_most : 8;
                    UInt red_least : 2;
                    UInt red_most : 8;
#else
                    UInt red_most : 8;
                    UInt red_least : 2;
                    UInt green_most : 8;
                    UInt green_leaast : 2;
                    UInt blue_most : 8;
                    UInt blue_least : 2;
                    UInt unused2 : 2;
#endif
                };

                U32 pixelWord;
            };
        };

        //
        //  Reversed 10 Bit packed pixel structure
        //

        struct Pixel10Rev
        {
            union
            {
                struct
                {
#if defined(__LITTLE_ENDIAN__)
                    UInt red : 10;
                    UInt green : 10;
                    UInt blue : 10;
                    UInt unused : 2;
#else
                    UInt unused : 2;
                    UInt blue : 10;
                    UInt green : 10;
                    UInt red : 10;
#endif
                };

                struct
                {
#if defined(__LITTLE_ENDIAN__)
                    UInt red_least : 2;
                    UInt red_most : 8;
                    UInt green_leaast : 2;
                    UInt green_most : 8;
                    UInt blue_least : 2;
                    UInt blue_most : 8;
                    UInt unused2 : 2;
#else
                    UInt unused2 : 2;
                    UInt blue_most : 8;
                    UInt blue_least : 2;
                    UInt green_most : 8;
                    UInt green_least : 2;
                    UInt red_most : 8;
                    UInt red_least : 2;
#endif
                };

                U32 pixelWord;
            };
        };

        //
        //  new delete. FBs may be allocated with a special parallel
        //  allocator on some platforms (windows).
        //

        static void* operator new(size_t s);
        static void operator delete(void* p, size_t s);

        //
        //  Constructors
        //
        //  NOTE: all of the constructors call FrameBuffer::restructure()
        //  to do the actual work.
        //

        FrameBuffer();
        FrameBuffer(const FrameBuffer&);
        ~FrameBuffer();

        //
        //  Most general constructor for frame buffer
        //
        //  It is assumed that the memory for the data was allocated with
        //  FrameBuffer::allocateLargeBlock().
        //
        //  Note: Using that constructor, the FrameBuffer instance will manage
        //  the data buffer.
        //

        FrameBuffer(CoordinateTypes coordinateType, int width, int height,
                    int depth, DataType dataType, unsigned char* data,
                    const StringVector* channelNames);

        //
        //  In the case that data is provided, the FrameBuffer will take
        //  ownership. It is assumed that the memory was allocated with
        //  FrameBuffer::allocateLargeBlock().
        //

        FrameBuffer(int width, int height, int numChannels, DataType dataType,
                    unsigned char* data = NULL,
                    const StringVector* channelNames = NULL,
                    Orientation orient = BOTTOMLEFT, int extraScanlines = 0,
                    int extraScanlinePixels = 0);

        //
        //  If the data parameter is supplied and deleteOnDestruction is
        //  true than the data parameter memory should have been allocated
        //  with FrameBuffer::allocateLargeBlock() below. If
        //  deleteOnDestruction is false the caller is responsible for
        //  deallocation. If the FrameBuffer allocated the data and
        //  deleteOnDestruction is also false then you need to use
        //  Frame::deallocateLargeBlock() on the data.
        //
        //

        FrameBuffer(int width, int height, int numChannels, DataType dataType,
                    unsigned char* data, const StringVector* channelNames,
                    Orientation orient, bool deleteOnDestruction,
                    int extraScanlines = 0, int extraScanlinePixels = 0);

        //
        //  Most general constructor (Allows 3D "image"). NOTE: to set
        //  uncrop you have to call uncrop() after you create the FB
        //

        FrameBuffer(CoordinateTypes coordinateType, int width, int height,
                    int depth, int numChannels, DataType dataType,
                    unsigned char* data, const StringVector* channelNames,
                    Orientation orient, bool deleteOnDestruction,
                    int extraScanlines = 0, int extraScanlinePixels = 0);

        //
        //  This is the function all of the constructors call. You can
        //  call this after the FrameBuffer is created to change its
        //  characteristics. The data will be lost (or garbage) after you
        //  call this. If data is passed in and deleteOnDestruction is
        //  true, then the memory should have been allocated with the
        //  function FrameBuffer::allocateLargeBlock()
        //
        //  The extraScanlines parameter causes the FrameBuffer to
        //  allocate additional scanlines past the end of the image. This
        //  is useful for image readers that need additional "working
        //  space" in order to be efficient.
        //
        //  NOTE: the uncrop parameters are not passed in here. When
        //  restructure occurs any uncrop information is lost. So you need
        //  to call uncrop() again.
        //
        //  If data is supplied and deletePointer is also suppled and
        //  deleteOnDestruction is true then the framebuffer will delete
        //  deletePointer on destruction instead of the data pointer. This
        //  makes it possible to hand over a chunk of memory (like a file
        //  blob) that has the image embedded in it but you didn't know
        //  until you read it. Another way to think about the
        //  deletePointer is that its equivalent to "pre-padding".
        //

        void restructure(int width = 640, int height = 480, int depth = 0,
                         int numChannels = 3, DataType dataType = UCHAR,
                         unsigned char* data = NULL,
                         const StringVector* channelNames = NULL,
                         Orientation orient = BOTTOMLEFT,
                         bool deleteOnDestruction = true,
                         int extraScanlines = 0, int extraScanlinePixels = 0,
                         unsigned char* deletePointer = NULL,
                         bool clearAttributes = true);

        //
        //  Restructure Planar.
        //
        //  The number of planes created is determined by
        //  planeNames.size() / numChannels. The width and height are
        //  applied to the primary plane (this). The sizes of all
        //  planes are determined by xSamplings and ySamplings. There
        //  should be planeNames.size() entries for xSamplings and
        //  ySamplings.  All planes will have the same dataType and
        //  orientation.
        //
        //  Samplings are divisors. So a value of 2 means 1/2 of the width
        //  or height. A value of 1 means the same as the width or height.
        //

        void restructurePlanar(int width, int height,
                               const Samplings& xSamplings,
                               const Samplings& ySamplings,
                               const StringVector& planeNames,
                               DataType dataType, Orientation orient,
                               size_t numChannels = 1);

        //
        //  Same as above, except samplings are all 1
        //

        void restructurePlanar(int width, int height,
                               const StringVector& planeNames,
                               DataType dataType, Orientation orient);

        //
        //  Alternate Pixel Memory Management
        //
        //  Allocate/Deallocate memory which can be passed as the "data"
        //  parameter to FrameBuffer::restructure() or one of the
        //  constructors when deleteOnDestruction is true. This memory
        //  will be aligned (usually to the OS page) and may come from a
        //  special allocator. Memory allocated by
        //  FrameBuffer::allocateLargeBlock() must be deallocated with
        //  FrameBuffer::deallocateLargeBlock().
        //

        static void* allocateLargeBlock(size_t);
        static void deallocateLargeBlock(void*);

        //
        //  This is equivalent to calling restructure with data from FB
        //  followed by copyAttributesTo followed by setIdentifier. The
        //  passed in FB cannot be deleted before this FB.
        //

        void shallowCopy(FrameBuffer* fb);

        //
        //  Copy
        //

        FrameBuffer* copy() const;
        FrameBuffer* copyPlane() const;
        FrameBuffer* referenceCopy() const;

        void copyFrom(const FrameBuffer*);
        void referenceCopyFrom(const FrameBuffer*);

        FrameBuffer& operator=(const FrameBuffer& fb)
        {
            copyFrom(&fb);
            return *this;
        }

        //
        //  Changing data ownership. This is inherently a bad idea. Avoid
        //  this part of the API if possible.
        //

        void ownData();
        void relinquishDataAndReset();

        //
        //  Planar FrameBuffers are a linked list of FrameBuffers. A Plane
        //  could be a layer (with RGB for each plane for example), or it
        //  might be single channels (like Y, RY, BY).
        //
        //  Delete all planes can only be called on the first plane. The
        //  result will be the deletion of all planes except the first
        //  plane.
        //

        void appendPlane(FrameBuffer*);
        void removePlane(FrameBuffer*);
        size_t numPlanes() const;
        void deleteAllPlanes();

        const FrameBuffer* nextPlane() const { return m_nextPlane; }

        const FrameBuffer* firstPlane() const
        {
            return m_firstPlane ? m_firstPlane : this;
        }

        const FrameBuffer* previousPlane() const { return m_previousPlane; }

        FrameBuffer* nextPlane() { return m_nextPlane; }

        FrameBuffer* firstPlane() { return m_firstPlane ? m_firstPlane : this; }

        FrameBuffer* previousPlane() { return m_previousPlane; }

        const bool isRootPlane() const { return m_firstPlane == 0; }

        //
        //  State Access
        //

        CoordinateTypes coordinateType() const { return m_coordinateType; }

        int width() const { return m_width; }

        int height() const { return m_height; }

        DataType dataType() const { return m_dataType; }

        int numChannels() const { return m_numChannels; }

        int bytesPerChannel() const { return m_bytesPerChannel; }

        int pixelSize() const { return m_pixelSize; }

        size_t scanlineSize() const { return m_scanlineSize; }

        size_t scanlinePaddedSize() const { return m_scanlinePaddedSize; }

        int scanlinePixelPadding() const { return m_scanlinePixelPadding; }

        int extraScanlines() const { return m_slopHeight - m_height; }

        size_t dataSize() const { return m_dataSize; }

        Orientation orientation() const { return m_orientation; }

        void setOrientation(Orientation o) { m_orientation = o; }

        bool uncrop() const { return m_uncrop; }

        bool needsUncrop() const
        {
            return m_uncrop
                   && (m_uncropWidth != m_width || m_uncropHeight != m_height
                       || m_uncropX != 0 || m_uncropY != 0);
        }

        int uncropWidth() const { return m_uncropWidth; }

        int uncropHeight() const { return m_uncropHeight; }

        int uncropX() const { return m_uncropX; }

        int uncropY() const { return m_uncropY; }

        void setUncrop(int w, int h, int x, int y);
        void setUncrop(const FrameBuffer* fb); // copy uncrop settings
        void setUncropActive(bool);
        float uncroppedAspect() const;

        void setScanlineSize(size_t scanlineSize)
        {
            m_scanlineSize = scanlineSize;
        }

        void setScanlinePaddedSize(size_t scanlinePaddedSize)
        {
            m_scanlinePaddedSize = scanlinePaddedSize;
        }

        //
        //  Display dimensions incorporate the uncropped dimensions and
        //  the pixel aspect
        //

        size_t allocSize() const { return m_allocSize; }

        size_t totalImageSize() const; // allocSize of all planes

        bool isNullImage() const { return m_width == 0; }

        bool isYUV() const { return m_yuv; }

        bool isYRYBY() const { return m_yryby; }

        bool isYRYBYPlanar() const;
        bool isYUVPlanar() const;
        bool isYUVBiPlanar() const;
        bool isYA2C2Planar() const;
        bool isRGBPlanar() const;

        bool isPlanar() const { return numPlanes() != 1; }

        int depth() const { return m_depth; } // 1 means 2D only

        size_t planeSize() const { return m_planeSize; }

        //
        //  Pixel Aspect Ratio
        //

        void setPixelAspectRatio(float pa)
        {
            m_pixelAspect = pa;
            attribute<float>("PixelAspectRatio") = pa;
        }

        float pixelAspectRatio() const { return m_pixelAspect; }

        //
        // Pixel/scanline/whole image access functions.  Can be used to
        // access pixels into any appropriate class/struct.  Por ejemplo:
        //
        //     img->pixel<Col4f>( x, y )
        //     img->scanline<short>( y )
        //     img->pixels<unsigned char>()
        //
        // Optionally, you can request an individual channel from a pixel, like
        // so:
        //
        //     img->pixel<float>( x, y, 1 )
        //
        // Note that because the pixel function returns a reference, it doesn't
        // like to be templated to a pointer--this is bad: img->pixel<char
        // *>(x,y) Use this instead:  &img->pixel<char>(x,y)
        //
        //  NOTE: There are currently no 3D functions for FrameBuffer.
        //
        //  The scanline<>() functions can get at the additionally allocated
        //  scanlines. The pixel functions cannot.
        //

        template <typename T> T& pixel(int x, int y, int c = 0);
        template <typename T> T& pixel(int x, int y, int c = 0) const;

        template <typename T> T* scanline(int y);
        template <typename T> const T* scanline(int y) const;

        template <typename T> T* pixels();
        template <typename T> const T* pixels() const;

        template <typename T> T* begin() { return pixels<T>(); }

        template <typename T> T* end();

        template <typename T> const T* begin() const { return pixels<T>(); }

        template <typename T> const T* end() const;

        //
        //  Generic set/get pixel functions.  The permute function
        //  provides a way to rearrange the values returned by
        //  getPixel4f() so that that:
        //
        //  channel 0 => R index (or Y in 1/2 channel images)
        //  channel 1 => G index (or Y in 1/2 channel images)
        //  channel 2 => B index (or Y in 1/2 channel images)
        //  channel 3 => A index (or -1 in images without A)
        //
        //  If the image is YUV, than
        //
        //  channel 0 => Y index
        //  channel 1 => U index
        //  channel 2 => V index
        //  channel 3 => A index (or -1 in image without A)
        //
        //  If the image is Y RY BY
        //
        //  channel 0 => Y  index
        //  channel 1 => RY index
        //  channel 2 => BY index
        //  channel 3 => A  index (or -1 in image without A)
        //
        //  NOTE: These functions operate on the plane on which they are
        //  called only. If you want to retrieve a raw or REC 709 RGB
        //  value for the general case, you should use the functions in
        //  Operations.h to do so.
        //

        const int* pixel4Permute() const { return m_colorPermute; }

        void setPixel3f(float r, float g, float b, int x, int y);
        void setPixel4f(float r, float g, float b, float a, int x, int y);

        void getPixel4f(int x, int y, float* p) const;
        void getPixelRGB4f(int x, int y, float* p) const;

        void getPixelBilinear4f(float x, float y, float* p) const;
        void getPixelBilinearRGB4f(float x, float y, float* p) const;

        //
        //  Internal DataType
        //

        void convertTo(DataType dataType);
        std::string dataTypeStr() const;

        //
        //  Channel Management
        //

        //
        //  insertChannel() - Insert a new channel before given channel
        //  (default=after last exsiting)
        //

        void insertChannel(const std::string& name, int channel = -1);

        template <typename T>
        void insertChannelByType(const std::string& name, int channel = -1);

        void removeChannel(int channel);
        void removeChannel(const std::string& name);

        template <typename T> void removeChannelByType(int channel);

        int channel(const std::string& name) const;
        bool hasChannel(const std::string& name) const;

        const std::string& channelName(int c) const
        {
            return m_channelNames[c];
        }

        void setChannelName(int channel, const std::string& name);

        const StringVector& channelNames() const { return m_channelNames; }

        //
        //  Attribute Management
        //  --------------------
        //

        //
        //  Does attribute exist?
        //

        bool hasAttribute(const std::string& name) const;

        //
        //  Return attribute value by type and name
        //
        //      float v = fb.attribute<float>("myattr");
        //
        //  For vector attributes, use vectorAttribute():
        //
        //      const vector<string>& sv = fb.vectorAttribute<string>("views");
        //

        template <typename T> const T& attribute(const std::string& name) const;

        template <typename T> T& attribute(const std::string& name);

        template <typename T>
        const std::vector<T>& vectorAttribute(const std::string& name) const;

        template <typename T>
        std::vector<T>& vectorAttribute(const std::string& name);

        //
        //  Create and add a new attribute to the FB.
        //  The insertion operation checks if the new attribute is equivalent to
        //  an attribute already in the container, and if so, removes the old
        //  one before adding the new one.
        //
        //      fb.newAttribute<int>("newint", 1);
        //  -or-
        //      fb.newAttribute("simple_int", int(1));
        //

        template <typename T>
        TypedFBAttribute<T>* newAttribute(const std::string& name, T value);

        template <typename T>
        TypedFBVectorAttribute<T>* newAttribute(const std::string& name,
                                                const std::vector<T>& value);

        //
        //  Find an attribute by name
        //

        const FBAttribute* findAttribute(const std::string& name) const;
        FBAttribute* findAttribute(const std::string& name);

        //
        //  All attributes
        //

        const AttributeVector& attributes() const { return m_attributes; }

        AttributeVector& attributes() { return m_attributes; }

        //
        //  Delete all attributes. FB will not longer have *any* attributes
        //  after calling this.
        //

        void clearAttributes();

        //
        //  Copy the attributes from this FB to another FB
        //

        void copyAttributesTo(FrameBuffer*) const;

        void appendAttributesAndPrefixTo(FrameBuffer*,
                                         const std::string& prefix) const;

        //
        //  Add an attribute to the FB.
        //  This will add the new attribute without looking for any equivalent
        //  attribute in the container. To ensure no duplicates, prefer
        //  using newAttribute().
        //

        void addAttribute(FBAttribute* a) { m_attributes.push_back(a); }

        //
        //  Delete the specified attribute
        //

        void deleteAttribute(const FBAttribute* a);

        //
        //  Color Space Attribtues
        //
        //  See top of this file for all the related constants and
        //  definitions. These functions hide the creation and lookup of
        //  regular image attributes.
        //

        void setPrimaryColorSpace(const std::string& name);
        void setTransferFunction(const std::string& tfun);
        void setPrimaries(float xWhite, float yWhite, float xRed, float yRed,
                          float xGreen, float yGreen, float xBlue, float yBlue);
        void setPrimaries(const Chromaticities&);
        void setAdoptedNeutral(float x, float y);
        void setMatrixAttribute(std::string name, float r0c0, float r0c1,
                                float r0c2, float r0c3, float r1c0, float r1c1,
                                float r1c2, float r1c3, float r2c0, float r2c1,
                                float r2c2, float r2c3, float r3c0, float r3c1,
                                float r3c2, float r3c3);
        void setRGBToXYZMatrix(float r0c0, float r0c1, float r0c2, float r0c3,
                               float r1c0, float r1c1, float r1c2, float r1c3,
                               float r2c0, float r2c1, float r2c2, float r2c3,
                               float r3c0, float r3c1, float r3c2, float r3c3);
        void setConversion(const std::string& c);
        void setRange(const std::string& c);
        void setChromaPlacement(const std::string& c);
        void setGamma(float exponent);
        void setICCprofile(const void* prof, size_t len);
        const DataContainer* iccProfile() const;

        bool hasPrimaryColorSpace() const;
        bool hasTransferFunction() const;
        bool hasPrimaries() const;
        bool hasAdoptedNeutral() const;
        bool hasConversion() const;
        bool hasRange() const;
        bool hasRGBtoXYZMatrix() const;
        bool hasLogCParameters() const;

        const std::string& primaryColorSpace() const;
        const std::string& transferFunction() const;
        void primaries(const std::string& primaryName, float& x,
                       float& y) const;
        Chromaticities chromaticities() const;
        const std::string& conversion() const;
        const std::string& range() const;

        //
        //  Identifier -- this is a conceptual hash value for the whole
        //  image. You might try something like the full path to the image plus
        //  a frame number plus parameters to operations you specify.
        //
        //  There are some special cases:
        //
        //      * It cannot start with the char '|' this is reserved to
        //        indicate an identifier for a missing image in the
        //        cache. for example: |foo.0002.jpg\nfoo.001.jpg is parsed
        //        as: |MID|SID where MID is the missing id or as much of
        //        it as is known and SID is the subsititute identifer for
        //        an existing image which can be used in place of it. If
        //        SID is blank like |MID then no substitute exists. In
        //        this case foo.0002.jpg is missing and can be substituted
        //        with foo.001.jpg for display purposes.
        //
        //      * In addition to the above, if the identifier is
        //        referencing an image that is "out-of-range" it should
        //        start with a double '|' like: ||MID\nSID or ||MID
        //

        HashStream& idstream();
        std::string identifier() const;
        void setIdentifier(const std::string& s);

        //
        //  Debug output
        //

        void outputInfo(std::ostream&) const;
        void outputAttrs(std::ostream&) const;

    public:
        bool inCache() const { return m_cacheRef > 0; }

        bool isCacheLocked() const { return m_cacheLock; }

        void staticRef() const { m_staticRef++; }

        bool staticUnRef() const { return --m_staticRef == 0; }

        bool hasStaticRef() const { return m_staticRef != 0; }

    private:
        void lockCache() { m_cacheLock++; }

        void unlockCache()
        {
            m_cacheLock = m_cacheLock > 0 ? m_cacheLock - 1 : 0;
        }

    protected:
        void recalcStrides();
        void clear();
        void deleteNextPlane();

    private:
        //
        //  Data is stored as unsigned char for ease with new/delete,
        //  though data itself may be any type.  Note, channel data is
        //  ALWAYS interleaved: RGBARGBARGBA.....
        //

        CoordinateTypes m_coordinateType;
        bool m_deleteDataOnDestruction;
        unsigned char* m_deletePointer;
        unsigned char* m_data;
        int m_width;
        int m_height;
        int m_depth; // usually 1
        float m_pixelAspect;
        int m_slopHeight; // height including extra scanlines
        int m_numChannels;
        DataType m_dataType;
        int m_bytesPerChannel;
        Orientation m_orientation;
        size_t m_dataSize;
        size_t m_allocSize;
        bool m_yuv;
        bool m_yryby;
        int m_pixelSize;
        size_t m_scanlineSize;
        size_t m_scanlinePaddedSize;
        size_t m_planeSize;
        int m_scanlinePixelPadding;

        bool m_uncrop;
        int m_uncropWidth;
        int m_uncropHeight;
        int m_uncropX;
        int m_uncropY;

        //
        //  Planes
        //

        FrameBuffer* m_nextPlane;
        FrameBuffer* m_firstPlane;
        FrameBuffer* m_previousPlane;

        //
        //  HashStream is used to uniquely identify the image in the frame
        //  buffer. The time value and lock are used by the Cache.
        //

        HashStream m_idstream;
        size_t m_retrievalTime;
        size_t m_cacheLock;
        size_t m_cacheRef;

        mutable std::atomic<int> m_staticRef{0};

        //
        // Names of each channel
        // Attributes for the whole image
        //

        int m_colorPermute[4];
        StringVector m_channelNames;
        AttributeVector m_attributes;

        friend class Cache;
    };

    typedef std::vector<const FrameBuffer*> ConstFBVector;
    typedef std::vector<FrameBuffer*> FBVector;

    // *****************************************************************************

    template <typename T> inline T& FrameBuffer::pixel(int x, int y, int c)
    {
        assert(x >= 0 && x < m_width);
        assert(y >= 0 && y < m_height);

        size_t offset = (y * m_scanlinePaddedSize) + (x * m_pixelSize)
                        + (c * m_bytesPerChannel);

        return (T&)(m_data[offset]);
    }

    template <typename T>
    inline T& FrameBuffer::pixel(int x, int y, int c) const
    {
        assert(x >= 0 && x < m_width);
        assert(y >= 0 && y < m_height);

        size_t offset = (y * m_scanlinePaddedSize) + (x * m_pixelSize)
                        + (c * m_bytesPerChannel);

        return (T&)(m_data[offset]);
    }

    template <typename T> inline T* FrameBuffer::scanline(int y)
    {
        assert(y >= 0 && y < m_slopHeight);

        size_t offset = y * m_scanlinePaddedSize;

        return (T*)(&m_data[offset]);
    }

    template <typename T> inline const T* FrameBuffer::scanline(int y) const
    {
        assert(y >= 0 && y < m_slopHeight);

        size_t offset = y * m_scanlinePaddedSize;

        return (const T*)(&m_data[offset]);
    }

    template <typename T> inline T* FrameBuffer::pixels() { return (T*)m_data; }

    template <typename T> inline const T* FrameBuffer::pixels() const
    {
        return (const T*)m_data;
    }

    template <typename T> inline T* FrameBuffer::end()
    {
        size_t off = m_scanlinePaddedSize * m_height;
        return (T*)(m_data + off);
    }

    template <typename T> inline const T* FrameBuffer::end() const
    {
        size_t off = m_scanlinePaddedSize * m_height;
        return (const T*)(m_data + off);
    }

#ifndef TWK_PUBLIC

    template <typename T>
    void FrameBuffer::insertChannelByType(const std::string& name, int channel)
    {
        if (channel < 0 || channel > m_numChannels)
        {
            channel = m_numChannels;
        }

        int numChannels = m_numChannels + 1;
        T* newData = new T[m_width * m_height * numChannels];
        T* oldData = (T*)m_data;

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                int newOffset = (y * m_width * numChannels) + (x * numChannels);
                int oldOffset =
                    (y * m_width * m_numChannels) + (x * m_numChannels);

                for (int destC = 0, srcC = 0; destC < numChannels; ++destC)
                {
                    if (destC == channel)
                        continue;
                    newData[newOffset + destC] = oldData[oldOffset + srcC];
                    ++srcC;
                }
            }
        }
        delete[] m_data;
        m_data = (unsigned char*)newData;
        m_numChannels = numChannels;
        recalcStrides();

        m_channelNames.insert(m_channelNames.begin() + channel, name);
    }

    template <typename T> void FrameBuffer::removeChannelByType(int channel)
    {
        assert(channel >= 0 && channel < m_numChannels);

        int numChannels = m_numChannels - 1;
        T* newData = new T[m_width * m_height * numChannels];
        T* oldData = (T*)m_data;

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                int newOffset = (y * m_width * numChannels) + (x * numChannels);
                int oldOffset =
                    (y * m_width * m_numChannels) + (x * m_numChannels);

                for (int srcC = 0, destC = 0; srcC < numChannels; ++srcC)
                {
                    if (srcC == channel)
                        continue;
                    newData[newOffset + destC] = oldData[oldOffset + srcC];
                    ++destC;
                }
            }
        }
        delete[] m_data;
        m_data = (unsigned char*)newData;
        m_numChannels = numChannels;
        recalcStrides();

        m_channelNames.erase(m_channelNames.begin() + channel);
    }

    inline int FrameBuffer::channel(const std::string& name) const
    {
        for (size_t i = 0; i < m_channelNames.size(); ++i)
        {
            if (m_channelNames[i] == name)
            {
                return i;
            }
        }
        return -1;
    }

#endif

    template <typename T>
    const T& FrameBuffer::attribute(const std::string& name) const
    {
        if (const FBAttribute* a = findAttribute(name))
        {
            if (const TypedFBAttribute<T>* ta =
                    dynamic_cast<const TypedFBAttribute<T>*>(a))
            {
                return ta->value();
            }
        }

        throw name;
    }

    template <typename T> T& FrameBuffer::attribute(const std::string& name)
    {
        if (FBAttribute* a = findAttribute(name))
        {
            if (TypedFBAttribute<T>* ta = dynamic_cast<TypedFBAttribute<T>*>(a))
            {
                return ta->value();
            }
        }

        return newAttribute<T>(name, T())->value();
    }

    template <typename T>
    const std::vector<T>&
    FrameBuffer::vectorAttribute(const std::string& name) const
    {
        if (const FBAttribute* a = findAttribute(name))
        {
            if (const TypedFBVectorAttribute<T>* ta =
                    dynamic_cast<const TypedFBVectorAttribute<T>*>(a))
            {
                return ta->value();
            }
        }

        throw name;
    }

    template <typename T>
    std::vector<T>& FrameBuffer::vectorAttribute(const std::string& name)
    {
        if (FBAttribute* a = findAttribute(name))
        {
            if (TypedFBVectorAttribute<T>* ta =
                    dynamic_cast<TypedFBVectorAttribute<T>*>(a))
            {
                return ta->value();
            }
        }

        return newAttribute<T>(name, std::vector<T>())->value();
    }

    template <typename T>
    TypedFBAttribute<T>* FrameBuffer::newAttribute(const std::string& name,
                                                   T value)
    {
        if (FBAttribute* old = findAttribute(name))
            deleteAttribute(old);
        TypedFBAttribute<T>* a = new TypedFBAttribute<T>(name, value);
        m_attributes.push_back(a);
        return a;
    }

    template <typename T>
    TypedFBVectorAttribute<T>*
    FrameBuffer::newAttribute(const std::string& name,
                              const std::vector<T>& value)
    {
        if (FBAttribute* old = findAttribute(name))
            deleteAttribute(old);
        TypedFBVectorAttribute<T>* a =
            new TypedFBVectorAttribute<T>(name, value);
        m_attributes.push_back(a);
        return a;
    }

} //  End namespace TwkFB

#endif // __TwkFB__FrameBuffer__h__
