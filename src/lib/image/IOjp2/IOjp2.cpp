//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <IOjp2/IOjp2.h>
#include <TwkFB/Operations.h>
#include <TwkFB/Exception.h>
#include <TwkUtil/File.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/Str.h>
#include <TwkUtil/FileStream.h>
#include <fstream>

#ifdef WIN32
#if !defined(COMPILER) || (COMPILER != GCC && COMPILER != GCC4 && COMPILER != GCC44 && COMPILER != CLANG)
#define restrict
#endif
#else
// Ref.: https://en.wikipedia.org/wiki/Restrict
// C++ does not have standard support for restrict, but many compilers have equivalents that
// usually work in both C++ and C, such as the GCC's and Clang's __restrict__, and
// Visual C++'s __declspec(restrict). In addition, __restrict is supported by those three
// compilers. The exact interpretation of these alternative keywords vary by the compiler:
#define restrict __restrict
#endif

namespace TwkFB {
using namespace std;
using namespace TwkUtil;

// Error callback expecting a FILE* client object
static void jp2_error_callback( const char *msg, void *client_data )
{
    IOjp2* jp2 = (IOjp2*)client_data;
    jp2->m_error = true;
    cout << "ERROR:" << msg << endl;
}

// Warning callback expecting a FILE* client object
static void jp2_warning_callback( const char *msg, void *client_data )
{
    cout << "WARNING:" << msg << endl;
}

// Debug callback expecting no client object
static void jp2_info_callback( const char *msg, void *client_data ) 
{
    cout << "INFO:" << msg << endl;
}


IOjp2::IOjp2() : FrameBufferIO("IOjp2", "mz"), m_error(false)
{
    //
    //  Indicate which extensions this plugin will handle The
    //  comparison against. The extensions are case-insensitive so
    //  there's no reason to provide upper case versions.
    //

    unsigned int cap = ImageRead | BruteForceIO;// | ImageWrite | Int8Capable;

    addType("j2c", "JPEG-2000 Codestream", cap);
    addType("j2k", "JPEG-2000 Codestream", cap);
    addType("jpt", "JPT-stream (JPEG 2000, JPIP)", cap);
    addType("jp2", "JPEG-2000 Image", cap);

    //
    //  NOTE: if we had different compressor types the addType calls
    //  might look like this:
    //
    //  addType("imgext", "IMG image file", cap, "RLE", "ZIP", "LZW", NULL);
    //
    //  Where "RLE", ... "LZW" are all compressor types for this image
    //  file format. RVIO and other programs will use this information
    //  supplied by the user.
    //


    // configure the event callbacks (not required)
    m_eventMgr.error_handler = jp2_error_callback;
    m_eventMgr.warning_handler = jp2_warning_callback;
    m_eventMgr.info_handler = NULL; // jp2_info_callback;
}

IOjp2::~IOjp2() {}


string
IOjp2::about() const
{ 
    ostringstream str;
    str << "JP2 (OpenJPEG " <<  opj_version() << ")";
    return str.str();
}

void
IOjp2::throwIfError(const std::string& filename) const
{
    if (m_error)
    {
        m_error = false;
        TWK_THROW_STREAM(TwkFB::IOException, "JPEG: failed to open jpeg file \"" 
                         << filename);
    }
}


void
IOjp2::getImageInfo(const std::string& filename, FBInfo& fbi) const
{
    opj_dparameters_t decodeParams;
    opj_set_default_decoder_parameters( &decodeParams );
    
    // Only decode the header
    decodeParams.cp_limit_decoding = LIMIT_TO_MAIN_HEADER;

    opj_dinfo_t* decompressorHandle;
    const std::string ext = TwkUtil::lc( TwkUtil::extension( filename ) );
    if ( ext == "j2k" || ext == "j2c" ) decompressorHandle = opj_create_decompress( CODEC_J2K );
    else if ( ext == "jpt" ) decompressorHandle = opj_create_decompress( CODEC_JPT );
    else if ( ext == "jp2" ) decompressorHandle = opj_create_decompress( CODEC_JP2 );
    else TWK_THROW_STREAM(IOException, "JP2: unrecognized extension " << filename);

    opj_set_event_mgr( (opj_common_ptr)decompressorHandle, &m_eventMgr, (void *)this );

    opj_setup_decoder( decompressorHandle, &decodeParams );

    opj_cio_t* cio = 0;

    FileStream fmap(filename, FileStream::Buffering, 0, 0);
    void* rawdata = fmap.data();
    cio = opj_cio_open( (opj_common_ptr)decompressorHandle, 
                                   (unsigned char*)rawdata, 
                                   fmap.size() );

    if (!cio)
    {
        TWK_THROW_STREAM(IOException, "Can't open " << filename);
    }

    opj_image_t* image = opj_decode( decompressorHandle, cio );

    if (!image)
    {
        TWK_THROW_STREAM(IOException, "Unable to create decode object " << filename);
    }

    throwIfError( filename );

    fbi.dataType     = FrameBuffer::UCHAR;
    fbi.width        = image->x1;
    fbi.height       = image->y1;
    fbi.numChannels  = image->numcomps;
    fbi.orientation  = FrameBuffer::BOTTOMLEFT;

    switch (image->color_space)
    {
      case CLRSPC_SRGB:
          fbi.proxy.setPrimaryColorSpace(ColorSpace::Rec709());
          fbi.proxy.setTransferFunction(ColorSpace::sRGB());
          break;
      case CLRSPC_GRAY:
          fbi.proxy.setPrimaryColorSpace(ColorSpace::Rec709());
          fbi.proxy.setTransferFunction(ColorSpace::Linear());
          break;
      case CLRSPC_SYCC:
          fbi.proxy.setPrimaryColorSpace(ColorSpace::Rec709());
          fbi.proxy.setTransferFunction(ColorSpace::Rec601());
          break;
      case CLRSPC_UNSPECIFIED:
          fbi.proxy.attribute<string>("IOjp2/Note") = "Assuming XYZ primaries";
          fbi.proxy.attribute<string>("ColorSpace/Transfer") = "Gamma 2.6";
          fbi.proxy.setPrimaryColorSpace("UNSPECIFIED");
          fbi.proxy.setPrimaries(0.333333333f, 0.333333333f,
                                 1.0f, 0.0f,
                                 0.0f, 1.0f,
                                 0.0f, 0.0f);
          break;
      case CLRSPC_UNKNOWN:
          fbi.proxy.setPrimaryColorSpace("UNKNOWN");
          break;
      default:
          break;
    }

    fbi.proxy.attribute<int>("J2K/BitDepth") = image->comps[0].prec;

    opj_destroy_decompress( decompressorHandle );
    opj_image_destroy( image );
}


void
IOjp2::readImage(FrameBuffer& fb,
                  const std::string& filename,
                  const ReadRequest& request) const
{
    //
    //  Buffering should work on all platforms, all filesystems
    //

    FileStream fmap(filename, FileStream::Buffering, 0, 0);
    void* rawdata = fmap.data();

    opj_dparameters_t decodeParams;
    opj_set_default_decoder_parameters( &decodeParams );

    opj_dinfo_t* decompressorHandle;
    const std::string ext = TwkUtil::lc( TwkUtil::extension( filename ) );
    if ( ext == "j2k" || ext == "j2c" ) decompressorHandle = opj_create_decompress( CODEC_J2K );
    else if ( ext == "jpt" ) decompressorHandle = opj_create_decompress( CODEC_JPT );
    else if ( ext == "jp2" ) decompressorHandle = opj_create_decompress( CODEC_JP2 );
    else TWK_THROW_STREAM(IOException, "JP2: unrecognized extension " << filename);

    opj_set_event_mgr( (opj_common_ptr)decompressorHandle, &m_eventMgr, (void *)this );

    opj_setup_decoder( decompressorHandle, &decodeParams );

    opj_cio_t* cio = opj_cio_open( (opj_common_ptr)decompressorHandle, 
                                   (unsigned char*)rawdata, 
                                   fmap.size() );

    Timer decodeTimer;
    decodeTimer.start();

    opj_image_t* image = opj_decode( decompressorHandle, cio );

    decodeTimer.stop();

    vector<string> planeNames;
    static const char* rgbNames[] = {"R", "G", "B", "A"};
    static const char* yuvNames[] = {"Y", "U", "V", "A"};
    const bool yuv = image->color_space == CLRSPC_SYCC || image->color_space == CLRSPC_GRAY; 
          
    throwIfError( filename );

    FrameBuffer::DataType dataType;
    if ( image->comps[0].prec <= 8 ) dataType = FrameBuffer::UCHAR;
    else if ( image->comps[0].prec <= 16 ) dataType = FrameBuffer::USHORT;
    else TWK_THROW_STREAM(IOException, "JP2: unsupported bitdepth " << filename);

    const size_t depth       = 0;
    const size_t width       = image->x1;
    const size_t height      = image->y1;
    const size_t numChannels = std::min( 4, image->numcomps );

    for (size_t i = fb.numPlanes(); i < numChannels; i++)
    {
        fb.appendPlane(new FrameBuffer());
    }

    FrameBuffer* plane = &fb;

    Timer formatTimer;
    formatTimer.start();

    for (size_t i = 0; i < numChannels; i++, plane = plane->nextPlane())
    {
        vector<string> chNames(1);
        chNames[0] = yuv ? yuvNames[i] : rgbNames[i];

        plane->restructure(image->comps[i].w,
                           image->comps[i].h,
                           0,
                           1,
                           dataType,
                           0,
                           &chNames,
                           FrameBuffer::TOPLEFT);


        int* restrict d = image->comps[i].data;

        if (dataType == FrameBuffer::UCHAR)
        {
            unsigned char* restrict       p = plane->pixels<unsigned char>();
            const unsigned char* restrict e = p + plane->width() * plane->height();

            for (;p != e; p++, d++) *p = *d;
        }
        else if (dataType == FrameBuffer::USHORT)
        {
            unsigned short* restrict       p     = plane->pixels<unsigned short>();
            const unsigned short* restrict e     = p + plane->width() * plane->height();
            const int                      shift = 16 - image->comps[i].prec;

            for (;p != e; p++, d++) *p = ((unsigned short)*d) << shift;
        }

    }

    formatTimer.stop();

    if (image->icc_profile_buf)
    {
        fb.setPrimaryColorSpace(ColorSpace::ICCProfile());
        fb.setTransferFunction(ColorSpace::ICCProfile());
        fb.setICCprofile(image->icc_profile_buf, image->icc_profile_len);
    }

    switch (image->color_space)
    {
      case CLRSPC_SRGB:
          fb.setPrimaryColorSpace(ColorSpace::Rec709());
          fb.setTransferFunction(ColorSpace::sRGB());
          break;
      case CLRSPC_GRAY:
          fb.setPrimaryColorSpace(ColorSpace::Rec709());
          fb.setTransferFunction(ColorSpace::Linear());
          planeNames.resize(1);
          break;
      case CLRSPC_SYCC:
          fb.setPrimaryColorSpace(ColorSpace::Rec709());
          fb.setTransferFunction(ColorSpace::Rec601());
          break;
      case CLRSPC_UNSPECIFIED:
          fb.attribute<string>("IOjp2/Note") = "Assuming X'Y'Z' primaries";
          fb.setPrimaryColorSpace("UNSPECIFIED");
          fb.setPrimaries(0.333333333f, 0.333333333f,
                          1.0f, 0.0f,
                          0.0f, 1.0f,
                          0.0f, 0.0f);
          break;
      case CLRSPC_UNKNOWN:
          fb.setPrimaryColorSpace("UNKNOWN");
          break;
      default:
          break;
    }

    fb.attribute<float>("IOjp2/FormatTime") = formatTimer.elapsed();
    fb.attribute<float>("IOjp2/DecodeTime") = decodeTimer.elapsed();
    fb.attribute<int>("J2K/BitDepth") = image->comps[0].prec;

    opj_destroy_decompress( decompressorHandle );
    opj_image_destroy( image );
}


// void
// IOjp2::writeImage(const FrameBuffer& img,
//                    const std::string& filename,
//                    const WriteRequest& request) const
// {
//     struct jpeg_compress_struct cinfo;
//     struct jpeg_error_mgr jerr;
// 
//     const FrameBuffer* outfb = &img;
// 
//     //
//     //  Convert to UCHAR packed if not already.
//     //
// 
//     
//     if (img.isPlanar())
//     {
//         const FrameBuffer* fb = outfb;
//         outfb = mergePlanes(outfb);
//         if (fb != &img) delete fb;
//     }
// 
//     //
//     //  Convert everything to REC709
//     //
// 
//     if (hasChromaticies(outfb) || outfb->isYUV() || outfb->isYRYBY())
//     {
//         const FrameBuffer* fb = outfb;
//         outfb = convertToRGB709(outfb);
//         if (fb != &img) delete fb;
//     }
// 
//     //
//     //  Convert to 3 channel
//     //
// 
//     if (img.numChannels() != 3)
//     {
//         const FrameBuffer* fb = outfb;
//         vector<string> mapping;
//         mapping.push_back("R");
//         mapping.push_back("G");
//         mapping.push_back("B");
//         outfb = channelMap(const_cast<FrameBuffer*>(outfb), mapping);
//         if (fb != &img) delete fb;
//     }
// 
//     switch (img.dataType())
//     {
//       case FrameBuffer::HALF:
//       case FrameBuffer::FLOAT:
//       case FrameBuffer::USHORT:
//           {
//               const FrameBuffer* fb = outfb;
//               outfb = copyConvert(outfb, FrameBuffer::UCHAR);
//               if (fb != &img) delete fb;
//               break;
//           }
//       default:
//           break;
//     }
// 
//     //
//     //  Flip and Flop to get in the right orientation
//     //
// 
//     bool needflip = false;
//     bool needflop = false;
// 
//     switch (outfb->orientation())
//     {
//       case FrameBuffer::TOPLEFT:
//           needflip = true;
//           break;
//       case FrameBuffer::TOPRIGHT:
//       case FrameBuffer::BOTTOMRIGHT:
//           needflop = true;
//           break;
//     };
// 
//     if (needflop)
//     {
//         const FrameBuffer* fb = outfb;
//         if (outfb == &img) outfb = img.copy();
//         if (needflop) flop(const_cast<FrameBuffer*>(outfb));
//         if (fb != &img) delete fb;
//     }
// 
//     cinfo.err = jpeg_std_error(&jerr);
//     cinfo.err->error_exit = IOjp2_error_exit;
//     jpeg_create_compress(&cinfo);
//     cinfo.client_data = (void*)this;
// 
//     FILE* outfile;
// 
//     if ( !(outfile = TwkUtil::fopen(filename.c_str(), "wb")) )
//     {
//         TWK_THROW_STREAM(IOException, 
//                          "cannot open " << filename << " for writing");
//     }
// 
//     jpeg_stdio_dest(&cinfo, outfile);
// 
//     int h = outfb->height();
// 
//     cinfo.image_width      = outfb->width();
//     cinfo.image_height     = outfb->height();
//     cinfo.input_components = outfb->numChannels(); // really only 3 is ok
//     cinfo.in_color_space   = JCS_RGB; 
//     jpeg_set_defaults(&cinfo);
// 
//     jpeg_start_compress(&cinfo, TRUE);
//     throwError(filename);
// 
//     while (cinfo.next_scanline < h) 
//     {
//         int l = cinfo.next_scanline;
//         int lindex = needflip ? l : h - l - 1;
//         const unsigned char* scanline = outfb->scanline<unsigned char>(lindex);
// 	jpeg_write_scanlines(&cinfo, (JSAMPLE**)&scanline, 1);
//         throwError(filename);
//     }
// 
//     jpeg_finish_compress(&cinfo);
//     fclose(outfile);
//     jpeg_destroy_compress(&cinfo);
//     if (outfb != &img) delete outfb;
// }

}  //  End namespace TwkFB
