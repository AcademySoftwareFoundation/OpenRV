//******************************************************************************
// Copyright (c) 2001-2014 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#include <IOraw/IOraw.h>
#include <TwkMath/Chromaticities.h>
#include <TwkUtil/File.h>
#include <libraw/libraw.h>
#include <iostream>
#include <thread>
#include <boost/date_time/local_time/local_time.hpp>

using namespace std;
using namespace TwkMath;
namespace {

float
truncateFloat(float f)
{
    return floor(f * 100) / 100;
}

void
setFBAttributes(TwkFB::FrameBuffer* fb, LibRaw* ip, string primaries)
{
    // Handle color space settings

    if (primaries == "AdobeRGB")
    {
        fb->setPrimaryColorSpace(TwkFB::ColorSpace::AdobeRGB());

        Vec2f red   = TwkMath::Chromaticities<float>::AdobeRGB().red;
        Vec2f green = TwkMath::Chromaticities<float>::AdobeRGB().green;
        Vec2f blue  = TwkMath::Chromaticities<float>::AdobeRGB().blue;
        Vec2f white = TwkMath::Chromaticities<float>::AdobeRGB().white;

        fb->setPrimaries(white[0], white[1], red[0], red[1],
                         green[0], green[1], blue[0], blue[1]);
    }
    else
    {
        fb->setPrimaryColorSpace(TwkFB::ColorSpace::sRGB());

        Vec2f red   = TwkMath::Chromaticities<float>::sRGB().red;
        Vec2f green = TwkMath::Chromaticities<float>::sRGB().green;
        Vec2f blue  = TwkMath::Chromaticities<float>::sRGB().blue;
        Vec2f white = TwkMath::Chromaticities<float>::sRGB().white;

        fb->setPrimaries(white[0], white[1], red[0], red[1],
                         green[0], green[1], blue[0], blue[1]);
    }

    fb->setTransferFunction(TwkFB::ColorSpace::Linear());
    fb->setOrientation(TwkFB::FrameBuffer::TOPLEFT);

    // Pull metadata from the file
    ostringstream value;

    value << ip->imgdata.color.pre_mul[0] << ", " <<
             ip->imgdata.color.pre_mul[1] << ", " <<
             ip->imgdata.color.pre_mul[2] << ", " <<
             ip->imgdata.color.pre_mul[3];
    fb->newAttribute("WhiteBalance", value.str());
    value.clear(); value.str("");

    value << ip->imgdata.other.focal_len << "mm";
    fb->newAttribute("FocalLength", value.str());
    value.clear(); value.str("");

    value << "f/" << truncateFloat(ip->imgdata.other.aperture);
    fb->newAttribute("Aperture", value.str());
    value.clear(); value.str("");

    value << "1/" << truncateFloat(1.0f / ip->imgdata.other.shutter) << " secs";
    fb->newAttribute("Shutter", value.str());
    value.clear(); value.str("");

    value << ip->imgdata.other.iso_speed;
    fb->newAttribute("ISO", value.str());
    value.clear(); value.str("");

    value << ip->imgdata.idata.model;
    fb->newAttribute("Model", value.str());
    value.clear(); value.str("");

    value << ip->imgdata.idata.make;
    fb->newAttribute("Make", value.str());
    value.clear(); value.str("");

    time_t tt = static_cast<time_t>(ip->imgdata.other.timestamp);
    boost::posix_time::ptime pt = boost::posix_time::from_time_t(tt);
    boost::local_time::time_zone_ptr
        zone(new boost::local_time::posix_time_zone("UTC"));
    boost::local_time::local_date_time ldt_with_zone(pt, zone);
    value << ldt_with_zone;
    fb->newAttribute("Created", value.str());
}

}

namespace TwkFB {

IOraw::IOraw(int threads, double scale, string primaries, bool bruteForce) :
    FrameBufferIO("IOraw", "n")
{
    //
    // Assign the number of threads. Use -1 for auto threads.
    //

    m_threadCount = (threads == -1) ?
        (int) floor(0.75 * std::thread::hardware_concurrency() + 0.5) :
        threads;

    //
    // Look for a user specified scaling factor and color space primaries.
    //

    m_scaleFactor = scale;
    m_primaries   = primaries;

    //
    // Add all the camera types we know about
    //

    unsigned int capabilities = ImageRead;
    if (bruteForce) capabilities |= BruteForceIO;

    StringPairVector codecs;
    addType("arw", "Sony/Minolta RAW", capabilities, codecs);        // tested
    addType("bay", "Casio Bayer RAW", capabilities, codecs);
    addType("cr2", "Canon RAW 2", capabilities, codecs);             // tested
    addType("crw", "Canon RAW", capabilities, codecs);               // tested
    addType("dc2", "Kodak RAW 3", capabilities, codecs);
    addType("dcr", "Kodak RAW", capabilities, codecs);
    addType("dng", "Digital NeGative", capabilities, codecs);        // tested
    addType("erf", "Epson RAW", capabilities, codecs);
    addType("fff", "Imacon/Hasselblad RAW", capabilities, codecs);
    addType("k25", "Kodak RAW 4", capabilities, codecs);
    addType("kdc", "Kodak RAW 2", capabilities, codecs);
    addType("mdc", "Sony/Minolta RAW 4", capabilities, codecs);
    addType("mos", "CREO Photo RAW", capabilities, codecs);
    addType("mrw", "Sony/Minolta RAW 3", capabilities, codecs);
    addType("nef", "Nikon Electronic Format", capabilities, codecs); // tested
    addType("orf", "Olympus RAW", capabilities, codecs);
    addType("pef", "Pentax RAW", capabilities, codecs);
    addType("ptx", "Pentax RAW 2", capabilities, codecs);
    addType("pxn", "Fotoman RAW", capabilities, codecs);
    addType("raf", "Fuji RAW", capabilities, codecs);                // tested
    addType("raw", "Panasonic/Casio/Leica RAW", capabilities, codecs);
    addType("rdc", "Ricoh RAW", capabilities, codecs);
    addType("rwl", "Leica RAW", capabilities, codecs);
    addType("srf", "Sony/Minolta RAW 2", capabilities, codecs);
//    addType("x3f", "Sigma RAW", capabilities, codecs);               // forbiden
    addType("rmf", "Canon Raw Media Format", capabilities, codecs);
}

IOraw::~IOraw() {}

string
IOraw::about() const
{
    ostringstream str;
    str << "RAW (LibRaw: " << LibRaw::version() << ")";
    return str.str();
}

void
IOraw::readImage(FrameBuffer& fb, const std::string& filename,
    const ReadRequest& request) const
{
    //
    // Initialize the libraw image processor with the desired number of threads
    //

    LibRaw iProcessor;

    //
    // Default options to use with dcraw
    //

    iProcessor.imgdata.params.no_auto_bright = 1;  // '-W'
    iProcessor.imgdata.params.use_camera_wb  = 1;  // '-w'
    iProcessor.imgdata.params.output_bps     = 16; // '-8' need '-9'
    iProcessor.imgdata.params.output_color   =
        (m_primaries == "AdobeRGB") ? 2 : 1;       // '-o 1'
    iProcessor.imgdata.params.adjust_maximum_thr = 0; // This is LibRAW-ism;
                                                      // the prevent maximum adjust on a preframe basis.

    //
    // Set for linear data output
    //

    iProcessor.imgdata.params.gamm[0]        = 1.0;
    iProcessor.imgdata.params.gamm[1]        = 1.0;

    //
    // Open the file and read the metadata
    //

    iProcessor.open_file(UNICODE_C_STR(filename.c_str()));
    iProcessor.unpack();
    iProcessor.unpack_thumb();
    iProcessor.dcraw_process();

    int width, height, numChannels, bps;
    iProcessor.get_mem_image_format(&width, &height, &numChannels, &bps);
    int stride = width * (bps/8) * numChannels;

    //
    // Find the greated white balance value to use for scaling if none was
    // provided by the user or in auto-mode (i.e. -1)
    //

    double scale_factor = m_scaleFactor;

    if (scale_factor == -1.0)
    {
        double max01 = max(iProcessor.imgdata.color.pre_mul[0],
                          iProcessor.imgdata.color.pre_mul[1]);
        double max23 = max(iProcessor.imgdata.color.pre_mul[2],
                          iProcessor.imgdata.color.pre_mul[3]);
        scale_factor = max(max01, max23);

        //
        // Lastly make sure the derived scale factor is reasonable. "Should be
        // between 1.0 and 2.8. If its outside this range then the number
        // does not seem appropriate for 0.18 grey pt correction so we
        // dont use it."
        //
        if (scale_factor < 1.0f || scale_factor > 2.8f)
        {
            scale_factor = 1.0f;
        }
    }

    //
    //  XXX in one test of 4k image we save about 0.003 sec (per frame) by
    //  using non-planar FB, avoiding a copy.  But we know that some cards are
    //  slow displaying these textures, so stick to planar FBs in all cases for
    //  now.
    //

    bool planar = true;

/*
    cerr << "make: " << iProcessor.imgdata.idata.make <<
            " model: " << iProcessor.imgdata.idata.model <<
            " colors: " << iProcessor.imgdata.idata.colors <<
            " cdesc: " << iProcessor.imgdata.idata.cdesc <<
            " pixel_aspect: " << iProcessor.imgdata.sizes.pixel_aspect <<
            " flip: " << iProcessor.imgdata.sizes.flip <<
            " flash_used: " << iProcessor.imgdata.color.flash_used <<
            " model2: " << iProcessor.imgdata.color.model2 <<
            " profile: " << iProcessor.imgdata.color.profile <<
            " iso_speed: " << iProcessor.imgdata.other.iso_speed <<
            " shutter: " << iProcessor.imgdata.other.shutter <<
            " aperture: " << iProcessor.imgdata.other.aperture <<
            " focal_len: " << iProcessor.imgdata.other.focal_len <<
            " timestamp: " << iProcessor.imgdata.other.timestamp <<
            " shot_order: " << iProcessor.imgdata.other.shot_order <<
            " gpsdata: " << iProcessor.imgdata.other.gpsdata <<
            " desc: " << iProcessor.imgdata.other.desc <<
            " artist: " << iProcessor.imgdata.other.artist << endl;
*/
    if (!planar)
    {
        fb.restructure(width, height, 0, 3, FrameBuffer::USHORT);
        ushort* rgba = fb.pixels<unsigned short>();
        iProcessor.copy_mem_image(rgba, stride, 0);
    }
    else
    {
        //
        // Store each channel in its own Framebuffer plane.
        //

        FrameBuffer::Samplings full(3, 1);
        FrameBuffer::StringVector planeNames;
        planeNames.push_back("R");
        planeNames.push_back("G");
        planeNames.push_back("B");

        fb.restructurePlanar(width, height,
                            full, full,
                         planeNames,
                         FrameBuffer::USHORT,
                         FrameBuffer::BOTTOMLEFT,
                         1);  //  1 channel per plane.

        unsigned short* u16Buffer =
            new unsigned short [width * height * 3];

        iProcessor.copy_mem_image(u16Buffer, stride, 0);

        ushort* rp = fb.pixels<unsigned short>();
        ushort* gp = fb.nextPlane()->pixels<unsigned short>();
        ushort* bp = fb.nextPlane()->nextPlane()->pixels<unsigned short>();

        unsigned short* bufp = u16Buffer;
        for (int i=0; i < width * height; ++i)
        {
            *rp++ = *bufp++;
            *gp++ = *bufp++;
            *bp++ = *bufp++;
        }

        delete [] u16Buffer;
    }

    fb.attribute<float>(TwkFB::ColorSpace::LinearScale()) = scale_factor;

    setFBAttributes(&fb, &iProcessor, m_primaries); // Snag metadata

    iProcessor.recycle(); // Clean up
}

void
IOraw::getImageInfo(const std::string& filename, FBInfo& info) const
{
    //
    // Open the file with desired number of threads
    //

    LibRaw iProcessor;
    iProcessor.open_file(UNICODE_C_STR(filename.c_str()));

    //
    // Snag the useful metadata
    //

    setFBAttributes(&info.proxy, &iProcessor, m_primaries);

    int bps;
    iProcessor.get_mem_image_format(
        &info.width, &info.height, &info.numChannels, &bps);
    info.dataType    = FrameBuffer::USHORT;
    info.orientation = FrameBuffer::TOPLEFT;

    iProcessor.recycle(); // Clean up
}

}  //  End namespace TwkFB
