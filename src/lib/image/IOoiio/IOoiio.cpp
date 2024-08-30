//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#include <OpenImageIO/imageio.h>
#include <IOoiio/IOoiio.h>
#include <TwkFB/Exception.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkUtil/File.h>
#include <half.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

namespace TwkFB {
using namespace std;
using namespace TwkUtil;
using namespace OpenImageIO_v2_4;

IOoiio::IOoiio() : FrameBufferIO("IOoiio", "n") // OIIO after any defaults of ours
{
    StringPairVector codecs;
    unsigned int r = ImageRead;
    unsigned int w = ImageWrite;
    unsigned int rw = r | w;
    addType("psd", "Adobe Photoshop", r, codecs);
    addType("pic", "Softimage PIC", r, codecs);
    addType("tga", "TARGA", rw, codecs);
    addType("targa", "TARGA", rw, codecs);
    addType("sgi", "SGI image", rw, codecs);
    addType("jpg", "JPEG image", rw, codecs);
    addType("jpeg", "JPEG image", rw, codecs);
    addType("bw", "SGI image", rw, codecs);
    addType("rgb", "SGI image", rw, codecs);
    addType("rgba", "SGI image", rw, codecs);
    addType("inta", "SGI image", rw, codecs);
    addType("int", "SGI image", rw, codecs);
    addType("pnm", "PNM", rw, codecs);
    addType("fits", "FITS", rw, codecs);
    addType("dpx", "SMPTE DPX", rw, codecs);
    addType("cin", "Kodak Cineon", rw, codecs);
    addType("cineon", "Kodak Cineon", w, codecs);
    addType("webp", "Google WebP", rw, codecs);
    addType("ptex", "Disney PTex", r, codecs);
    addType("ptx", "Disney PTex", r, codecs);
    addType("rla", "Wavefront RLA", rw, codecs);
    addType("iff", "IFF", rw, codecs);
    addType("bmp", "Windows Bitmap", rw, codecs);
    addType("dds", "Direct Draw Surface", rw, codecs);
    addType("gif", "Graphics Interchange Format", r, codecs);
    addType("ico", "Palette", r, codecs);
    addType("pbm", "Portable Network Graphics", rw, codecs);
    addType("pgm", "Portable Network Graphics", rw, codecs);
    addType("ppm", "Portable Network Grapics", rw, codecs);

    // These are handled by their respective plugins
    // io_<something> in RV. This code is here to
    // test oiio handling of these image types.
    //  
    // addType("tif", "TIFF Image", rw, codecs);
    // addType("tiff", "TIFF Image", rw, codecs);
    addType("j2c", "JPEG-2000 Codestream", r, codecs);
    addType("j2k", "JPEG-2000 Codestream", r, codecs);
    addType("jpt", "JPT-stream (JPEG 2000, JPIP)", r, codecs);
    addType("jp2", "JPEG-2000 Image", r, codecs);
    // addType("png", "Portable Network Graphics Image", rw, codecs);
    // addType("z", "Pixar Z-Depth", r, codecs);
}

IOoiio::~IOoiio() {}

string
IOoiio::about() const
{ 
    ostringstream str;
    str << "OpenImageIO: "
        << OIIO_VERSION_MAJOR << "."
        << OIIO_VERSION_MINOR << "."
        << OIIO_VERSION_PATCH;

    return str.str();
}

static void 
readAttrs(TwkFB::FrameBuffer& fb, ImageSpec& spec)
{
    for (size_t i = 0, s = spec.extra_attribs.size(); i != s; i++)
    {
        const ParamValue& value = spec.extra_attribs[i];
        const TypeDesc type = value.type();
        const string name = value.name().string();
        const size_t n = value.nvalues();

        switch (type.basetype)
        {
          case TypeDesc::UNKNOWN:
          case TypeDesc::NONE:
          case TypeDesc::INT64:
          case TypeDesc::UINT64:
              fb.newAttribute<string>(name, "-Unknown-");
              break;
          case TypeDesc::UINT8:
          case TypeDesc::INT8:
              {
                  if (name == "ICCProfile")
                  {
                      fb.setICCprofile(value.data(), type.size());
                      fb.setPrimaryColorSpace(ColorSpace::ICCProfile());
                      fb.setTransferFunction(ColorSpace::ICCProfile());
                  }
                  break;
              }
          case TypeDesc::UINT16:
          case TypeDesc::INT16:
              {
                  if (n == 1)
                  {
                      fb.newAttribute<short>(name, *(short*)value.data());
                  }
                  break;
              }
          case TypeDesc::UINT32:
              break;
          case TypeDesc::INT32:
              {
                  if (n == 1)
                  {
                      fb.newAttribute<int>(name, *(int*)value.data());
                  }
                  break;
              }
          case TypeDesc::HALF:
              {
                  if (n == 1)
                  {
                      fb.newAttribute<half>(name, *(float*)value.data());
                  }
                  break;
              }
          case TypeDesc::FLOAT:
              {
                  if (n == 1)
                  {
                      fb.newAttribute<float>(name, *(float*)value.data());
                  }
                  break;
              }
          case TypeDesc::DOUBLE:
              {
                  if (n == 1)
                  {
                      fb.newAttribute<double>(name, *(double*)value.data());
                  }
                  break;
              }
              break;
          case TypeDesc::STRING:
              {
                  if (n == 1)
                  {
                      fb.newAttribute<string>(name, spec.get_string_attribute(name, "?"));
                  }
                  break;
              }
        }
    }

    fb.newAttribute<string>("Reader", "OpenImageIO");
}

static bool
subimagesAsLayers(const string& fname)
{
    return fname == "psd";
}

static bool
premultedAlpha(const string& fname)
{
    return fname != "psd";
}

void
IOoiio::getImageInfo(const std::string& filename, FBInfo& fbi) const
{
    string ext = extension(filename);

#if 0
    if (ext == "dpx" || ext == "cin" || ext == "cineon" || ext == "DPX" || ext == "CIN")
    {
        TWK_THROW_STREAM(IOException, "OIIO: Unable to open file \"" 
                         << filename << "\" for reading");
    }
#endif

    if (std::unique_ptr<ImageInput> in = ImageInput::create(filename))
    {
        ImageSpec spec;
        in->open(filename, spec);
        bool useLayers = subimagesAsLayers(in->format_name());

        fbi.width       = spec.width;
        fbi.height      = spec.height;
        fbi.numChannels = spec.nchannels;
        FrameBuffer::DataType dtype;

        switch (spec.format.basetype)
        {
          case TypeDesc::UNKNOWN:
          case TypeDesc::NONE:
          case TypeDesc::INT64:
          case TypeDesc::UINT64:
              break;
          case TypeDesc::UINT8:
          case TypeDesc::INT8:
              dtype = FrameBuffer::UCHAR;
              break;
          case TypeDesc::UINT16:
          case TypeDesc::INT16:
              dtype = FrameBuffer::USHORT;
              break;
          case TypeDesc::UINT32:
          case TypeDesc::INT32:
              dtype = FrameBuffer::UINT;
              break;
          case TypeDesc::HALF:
              dtype = FrameBuffer::HALF;
              break;
          case TypeDesc::FLOAT:
              dtype = FrameBuffer::FLOAT;
              break;
          case TypeDesc::DOUBLE:
              dtype = FrameBuffer::DOUBLE;
              break;
        }

        fbi.dataType = dtype;

        FrameBuffer::Orientation orientation;

        switch (spec.get_int_attribute("Orientation", 1))
        {
          case 5:
          case 1: orientation = FrameBuffer::TOPLEFT; break;
          case 6:
          case 2: orientation = FrameBuffer::TOPRIGHT; break;
          case 7:
          case 3: orientation = FrameBuffer::BOTTOMRIGHT; break;
          case 8:
          case 4: orientation = FrameBuffer::NATURAL; break;
        }

        for (int c = 0; c < spec.nchannels; c++)
        {
            FBInfo::ChannelInfo cinfo;
            cinfo.name = spec.channelnames[c];
            cinfo.type = fbi.dataType;
            fbi.channelInfos.push_back(cinfo);
        }

        fbi.orientation = orientation;

        fbi.proxy.attribute<string>("AlphaType") = 
            premultedAlpha(in->format_name()) ? "Premultiplied" : "Unpremultiplied";

        readAttrs(fbi.proxy, spec);

        size_t nsubimages = 1;
        ImageSpec subspec;
        while (in->seek_subimage(nsubimages, 0, subspec)) nsubimages++;

        if (nsubimages > 1)
        {
            for (size_t i = 0; i < nsubimages; i++)
            {
                FBInfo::ViewInfo vinfo;
                FBInfo::LayerInfo linfo;
                ostringstream str;

                if (useLayers)
                {
                    str << "Layer " << i;
                    fbi.layers.push_back(str.str());
                    fbi.viewInfos.resize(1);
                    linfo.name = str.str();
                    vinfo.name = "";
                }
                else
                {
                    str << "View " << i;
                    fbi.views.push_back(str.str());
                    vinfo.name = str.str();
                }

                for (size_t q = 0; q < subspec.channelnames.size(); q++)
                {
                    FBInfo::ChannelInfo cinfo;
                    cinfo.name = subspec.channelnames[q];
                    
                    if (subspec.channelformats.size() == subspec.channelnames.size())
                    {
                        switch (subspec.channelformats[q].basetype)
                        {
                          case TypeDesc::UNKNOWN:
                          case TypeDesc::NONE:
                          case TypeDesc::INT64:
                          case TypeDesc::UINT64:
                              break;
                          case TypeDesc::UINT8:
                          case TypeDesc::INT8:
                              cinfo.type = FrameBuffer::UCHAR;
                              break;
                          case TypeDesc::UINT16:
                          case TypeDesc::INT16:
                              cinfo.type = FrameBuffer::USHORT;
                              break;
                          case TypeDesc::UINT32:
                          case TypeDesc::INT32:
                              cinfo.type = FrameBuffer::UINT;
                              break;
                          case TypeDesc::HALF:
                              cinfo.type = FrameBuffer::HALF;
                              break;
                          case TypeDesc::FLOAT:
                              cinfo.type = FrameBuffer::FLOAT;
                              break;
                          case TypeDesc::DOUBLE:
                              cinfo.type = FrameBuffer::DOUBLE;
                              break;
                        }
                    }
                    else
                    {
                        cinfo.type = fbi.dataType;
                    }

                    if (useLayers) linfo.channels.push_back(cinfo);
                    else vinfo.otherChannels.push_back(cinfo);
                }

                if (useLayers) fbi.viewInfos.front().layers.push_back(linfo);
                else fbi.viewInfos.push_back(vinfo);
            }

        }
    }
    else
    {
        TWK_THROW_STREAM(IOException, "OIIO: Unable to open file \"" 
                         << filename << "\" for reading. " <<
                         OpenImageIO_v2_4::geterror());
    }
}

void
IOoiio::readImage(FrameBuffer& fb,
                  const std::string& filename,
                  const ReadRequest& request) const
{
    if (std::unique_ptr<ImageInput> in = ImageInput::create(filename))
    {
        bool useLayers = subimagesAsLayers(in->format_name());

        ImageSpec spec;
        int subimage = 0;
        in->open(filename, spec);

        if (useLayers)
        {
            if (request.layers.size())
            {
                sscanf(request.layers[0].c_str(), "Layer %d", &subimage);
                if (!in->seek_subimage(subimage, 0, spec))
                {
                    TWK_THROW_STREAM(IOException, "OIIO: failed to find subimage " << subimage);
                }
            }
        }
        else if (request.views.size())
        {
            sscanf(request.views[0].c_str(), "View %d", &subimage);
            if (!in->seek_subimage(subimage, 0, spec))
            {
                TWK_THROW_STREAM(IOException, "OIIO: failed to find subimage " << subimage);
            }
        }

        FrameBuffer::DataType dtype;
        TypeDesc::BASETYPE informat = (TypeDesc::BASETYPE)spec.format.basetype;

        switch (spec.format.basetype)
        {
          case TypeDesc::UNKNOWN:
          case TypeDesc::NONE:
              dtype = FrameBuffer::UCHAR;
              break;
          case TypeDesc::UINT8:
          case TypeDesc::INT8:
              dtype = FrameBuffer::UCHAR;
              break;
          case TypeDesc::UINT16:
          case TypeDesc::INT16:
              dtype = FrameBuffer::USHORT;
              break;
          case TypeDesc::UINT32:
          case TypeDesc::INT32:
          case TypeDesc::INT64:
          case TypeDesc::UINT64:
              dtype = FrameBuffer::UINT;
              informat = TypeDesc::UINT32;
              break;
          case TypeDesc::HALF:
              dtype = FrameBuffer::HALF;
              break;
          case TypeDesc::DOUBLE:
          case TypeDesc::FLOAT:
              dtype = FrameBuffer::FLOAT;
              break;
        }

        FrameBuffer::Orientation orientation;

        switch (spec.get_int_attribute("Orientation", 1))
        {
          case 5:
          case 1: orientation = FrameBuffer::TOPLEFT; break;
          case 6:
          case 2: orientation = FrameBuffer::TOPRIGHT; break;
          case 7:
          case 3: orientation = FrameBuffer::BOTTOMRIGHT; break;
          case 8:
          case 4: orientation = FrameBuffer::NATURAL; break;
        }

        fb.restructure(spec.width, spec.height, spec.depth, 
                       spec.nchannels, dtype, 
                       NULL, &spec.channelnames, orientation,
                       true);

        in->read_image(informat, fb.pixels<void>());

        readAttrs(fb, spec);

        fb.attribute<string>("AlphaType") = 
            premultedAlpha(in->format_name()) ? "Premultiplied" : "Unpremultiplied";

        if (useLayers)
        {
            if (!request.layers.empty())
            {
                fb.attribute<int>("OIIO/subimage") = subimage;
                fb.attribute<string>("Layer") = request.layers[0];
            }
        }
        else if (!request.views.empty())
        {
            fb.attribute<int>("OIIO/subimage") = subimage;
            fb.attribute<string>("View") = request.views[0];
        }

        in->close();
        in.reset();
    }
    else
    {
        TWK_THROW_STREAM(IOException, "OIIO: Unable to open file \"" 
                         << filename << "\" for reading. " <<
                         OpenImageIO_v2_4::geterror());
    }
}


void
IOoiio::writeImage(const FrameBuffer& img,
                        const std::string& filename,
                        const WriteRequest& request) const
{
    FrameBufferIO::writeImage(img, filename, request);
}

}  //  End namespace TwkFB
