//******************************************************************************
// Copyright (c) 2008 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#include <IODevIL/IODevIL.h>
#include <IL/il.h>
#include <TwkExc/Exception.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <TwkMath/Mat44.h>
#include <TwkUtil/File.h>
#include <fstream>

namespace TwkFB {
using namespace std;
using namespace TwkUtil;
using namespace TwkMath;
using namespace TwkFB;

IODevIL::Mutex IODevIL::m_devILmutex;

IODevIL::IODevIL() : FrameBufferIO("IODevIL", "y") // DevIL is least preferred
{
    ScopedLock lock(m_devILmutex);
    
    //
    //  Indicate which extensions this plugin will handle The
    //  comparison against. The extensions are case-insensitive so
    //  there's no reason to provide upper case versions.
    //

    ilInit();

    //
    //  Haven't yet actually implemented the 'write' capabilities advertised here,
    //  so turn them off for now so that we don't advertise the capability falsely.
    //
    //  unsigned int cap8rw  = ImageRead | ImageWrite | Int8Capable;
    //  unsigned int cap16rw = ImageRead | ImageWrite | Int16Capable;
    unsigned int cap8rw  = ImageRead | Int8Capable;
    unsigned int cap16rw = ImageRead | Int16Capable;
    unsigned int cap8r   = ImageRead | Int8Capable;
    unsigned int cap16r  = ImageRead | Int16Capable;

    addType("bmp", "Windows Bitmap", cap8rw);
    addType("cut", "Dr. Halo Cut File", cap8r);
    addType("dds", "DirectDraw Surface", cap8rw);
    addType("gif", "Graphics Interchange Format", cap8r);
    addType("ico", "Palette", cap8r);
    addType("cur", "Palette", cap8r);
    addType("lbm", "Interlaced Bitmap", cap8r);
    addType("lif", "Homeworld File", cap8r);
    addType("lmp", "Doom Walls/Flats", cap8r);
    addType("mdl", "Half-Life Model", cap8r);
    addType("pcd", "PhotoCD", cap8r);
    addType("pcx", "ZSoft PCX", cap8rw);
    addType("pic", "PIC", cap8r);
    addType("pbm", "Portable Network Graphics", cap8rw);
    addType("pgm", "Portable Network Graphics", cap8rw);
    addType("ppm", "Portable Network Graphics", cap8rw);
    //addType("pnm", "Portable Network Graphics", cap8r);
    addType("psd", "PhotoShop", cap8r);
    addType("sgi", "Silicon Graphics", cap8rw);
    addType("rgb", "Silicon Graphics", cap8rw);
    addType("bw", "Silicon Graphics", cap8rw);
    addType("rgba", "Silicon Graphics", cap8rw);
    addType("tga", "Targa", cap8rw);
    addType("wal", "Quake2 Texture", cap8r);

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

    m_errorMap[IL_INVALID_ENUM]         = "Invalid enum";
    m_errorMap[IL_OUT_OF_MEMORY]        = "Out of memory";
    m_errorMap[IL_FORMAT_NOT_SUPPORTED] = "Format not supported";
    m_errorMap[IL_INTERNAL_ERROR]       = "IL internal error";
    m_errorMap[IL_ILLEGAL_OPERATION]    = "Illegal operation";
    m_errorMap[IL_ILLEGAL_FILE_VALUE]   = "Illegal file value";
    m_errorMap[IL_INVALID_FILE_HEADER]  = "Invalid file header";
    m_errorMap[IL_INVALID_PARAM]        = "Invalid parameter";
    m_errorMap[IL_COULD_NOT_OPEN_FILE]  = "Could not open file";
    m_errorMap[IL_INVALID_EXTENSION]    = "Invalid extension";
    m_errorMap[IL_FILE_ALREADY_EXISTS]  = "File already exists";
    m_errorMap[IL_OUT_FORMAT_SAME]      = "Out format same";
    m_errorMap[IL_STACK_OVERFLOW]       = "Stack overflow";
    m_errorMap[IL_STACK_UNDERFLOW]      = "Stack underflow";
    m_errorMap[IL_INVALID_CONVERSION]   = "Invalid conversion";
    m_errorMap[IL_BAD_DIMENSIONS]       = "Bad dimensions";
    m_errorMap[IL_FILE_READ_ERROR]      = "File error";
}

IODevIL::~IODevIL()
{
}

string
IODevIL::about() const
{ 
    char temp[80];
    sprintf(temp, "DevIL %d", IL_VERSION);
    return temp;
}

const char*
IODevIL::errorString(unsigned int e) const
{
    ErrorMap::const_iterator i = m_errorMap.find(e);

    if (i != m_errorMap.end())
    {
        return i->second;
    }
    else
    {
        return "unknown error";
    }
}

static FrameBuffer::DataType
fbtype(ILenum e)
{
    switch (e)
    {
      case IL_BYTE:
      case IL_UNSIGNED_BYTE:
          return FrameBuffer::UCHAR;
          break;
              case IL_SHORT:
      case IL_UNSIGNED_SHORT:
          return FrameBuffer::USHORT;
          break;
      case IL_INT:
      case IL_UNSIGNED_INT:
          break;
    }

    TWK_THROW_EXC_STREAM("No DataType for " << (int)e);
}

static bool
channelsForFormat(ILenum e, FrameBuffer::StringVector& channels)
{
    const char* ch = 0;
    bool reverse = false;

    switch (e)
    {
      case IL_COLOUR_INDEX:
      case IL_BGR:              reverse = true;
      case IL_RGB:              ch = "RGB"; break;
      case IL_BGRA:             reverse = true;
      case IL_RGBA:             ch = "RGBA"; break;
      case IL_LUMINANCE:        ch = "Y"; break;
      case IL_LUMINANCE_ALPHA:  ch = "YA"; break;
    }

    for (const char* p = ch; p && *p != '\0'; p++)
    {
        char temp[2];
        temp[1] = '\0';
        temp[0] = *p;
        channels.push_back(temp);
    }

    return reverse;
}

void
IODevIL::getImageInfo(const std::string& filename, FBInfo& fbi) const
{
    ScopedLock lock(m_devILmutex);

    //
    //  This library can't deal with getImageInfo() without reading
    //  the entire image. So in the case of 
    //

    string ext = extension(filename);

#if 0
    if (ext == "tga" || ext == "TGA" || ext == "targa" || ext == "TARGA")
    {
        //
        //  Read the TARGA header instead of the whole image. NOTE: if
        //  this is being used on a big-endian machine, the header
        //  will need to be swapped.
        //

        ifstream infile(UNICODE_C_STR(filename.c_str()));

        if (!infile)
        {
            TWK_THROW_STREAM(IOException, "Cannnot read " << filename);
        }

        TargaHeader header;
        infile.read((char*)&header, sizeof(TargaHeader));

        fbi.width       = header.width;
        fbi.height      = header.height;
        fbi.numChannels = header.bpp / 8;
        fbi.dataType    = FrameBuffer::UCHAR;

        cout << "w, h, nc = " << fbi.width 
             << ", " << fbi.height 
             << ", " << fbi.numChannels << endl;
    }
    else
#endif
    {
        ILuint image = ilGenImage();
        ilBindImage(image);

        if (ilLoadImage(filename.c_str()))
        {
            fbi.orientation = ilGetInteger(IL_IMAGE_ORIGIN) == IL_ORIGIN_LOWER_LEFT 
                ? FrameBuffer::NATURAL 
                : FrameBuffer::TOPLEFT;
            fbi.width       = ilGetInteger(IL_IMAGE_WIDTH);
            fbi.height      = ilGetInteger(IL_IMAGE_HEIGHT);
            fbi.numChannels = ilGetInteger(IL_IMAGE_CHANNELS);
            fbi.dataType    = fbtype(ilGetInteger(IL_IMAGE_TYPE));
        }
        else
        {
            TWK_THROW_STREAM(IOException, "DevIL ERROR: "
                             << errorString(ilGetError())
                             << ", while reading "
                             << filename);
        }

        ilDeleteImage(image);
    }
}

void
IODevIL::readImage(FrameBuffer& fb,
                   const std::string& filename,
                   const ReadRequest& request) const
{
    ScopedLock lock(m_devILmutex);

    ILuint image = ilGenImage();
    ilBindImage(image);

    if (ilLoadImage(filename.c_str()))
    {
        int    w      = ilGetInteger(IL_IMAGE_WIDTH);
        int    h      = ilGetInteger(IL_IMAGE_HEIGHT);
        ILenum type   = ilGetInteger(IL_IMAGE_TYPE);
        ILenum format = ilGetInteger(IL_IMAGE_FORMAT);
        int    nc     = ilGetInteger(IL_IMAGE_CHANNELS);

        if (format == IL_COLOUR_INDEX) 
        {
            if (ilConvertImage(IL_RGB, type) == IL_FALSE) 
            {
                TWK_THROW_STREAM(IOException, "DevIL ERROR: "
                                 << errorString(ilGetError())
                                 << ", while reading "
                                 << filename);
            }
            format = IL_RGB;
            nc = 3;
        }

        FrameBuffer::Orientation orient = 
            ilGetInteger(IL_IMAGE_ORIGIN) == IL_ORIGIN_LOWER_LEFT 
            ? FrameBuffer::NATURAL 
            : FrameBuffer::TOPLEFT;

        FrameBuffer::StringVector channels;
        bool reverse = channelsForFormat(format, channels);

        fb.restructure(w, h, 0, nc, fbtype(type), 0, &channels, orient, true);

        ilCopyPixels(0, 0, 0, w, h, 1, format, 
                     fb.dataType() == FrameBuffer::UCHAR ? IL_UNSIGNED_BYTE : IL_UNSIGNED_SHORT,
                     fb.scanline<void>(0));

        if (reverse)
        {
            Mat44f M( 0, 0, 1, 0,
                      0, 1, 0, 0,
                      1, 0, 0, 0,
                      0, 0, 0, 1 );
            
            applyTransform(&fb, &fb, linearColorTransform, &M);
        }
    }

    ilDeleteImage(image);
}

void
IODevIL::writeImage(const FrameBuffer& img,
                    const std::string& filename,
                    const WriteRequest& request) const
{
}

} // TwkFB
