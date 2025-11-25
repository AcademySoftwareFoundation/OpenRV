//
//  Copyright (c) 2025 Sam Richards
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IOhtj2k/IOhtj2k.h>
#include <TwkUtil/FileStream.h>
#include <TwkUtil/FileMMap.h>
#include <TwkUtil/File.h>
#include <TwkFB/Exception.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <TwkFB/Operations.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/Interrupt.h>
#include <TwkUtil/StdioBuf.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkMath/Color.h>
#include <vector>
#include <string>
#include <openjph/ojph_arg.h>
#include <openjph/ojph_mem.h>
#include <openjph/ojph_file.h>
#include <openjph/ojph_codestream.h>
#include <openjph/ojph_params.h>
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkMath;

    IOhtj2k::IOhtj2k(IOType type, size_t chunkSize, int maxAsync)
        : StreamingFrameBufferIO("IOhtj2k", "m7", type, chunkSize, maxAsync)
    // Note for the sortKey we need to be sooner than OpenImageIO since that will be the fallback.
    {
        //
        //  Indicate which extensions this plugin will handle The
        //  comparison against. The extensions are case-insensitive so
        //  there's no reason to provide upper case versions.
        //

        StringPairVector codecs;

        unsigned int cap = ImageRead;

        addType("j2c", "J2C Image", cap, codecs);
    }

    IOhtj2k::~IOhtj2k() {}

    string IOhtj2k::about() const { return "HTJ2K (OpenJPH)"; }

    void IOhtj2k::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        ojph::j2c_infile j2c_file;
        j2c_file.open(filename.c_str());

        ojph::codestream codestream;
        codestream.read_headers(&j2c_file);

        // codestream.enable_resilience();
        ojph::param_siz siz = codestream.access_siz();
        ojph::param_nlt nlt = codestream.access_nlt();
        bool nlt_is_signed;
        ojph::ui8 nlt_bit_depth;
        ojph::ui8 nl_type;
        bool has_nlt = nlt.get_nonlinear_transform(0, nlt_bit_depth, nlt_is_signed, nl_type);
        bool is_signed = siz.is_signed(0);

        // Signed and notlinear transforms are not supported, but it could be added in the future.
        if (is_signed)
            TWK_THROW_STREAM(UnsupportedException, "HTJ2K: unsupported signed jpeg2000 file " << filename);

        if (has_nlt)
            TWK_THROW_STREAM(UnsupportedException, "HTJ2K: unsupported notlinear transform " << filename);

        fbi.numChannels = siz.get_num_components();
        fbi.width = siz.get_recon_width(0);
        fbi.height = siz.get_recon_height(0);
        fbi.pixelAspect = double(1.0);
        fbi.orientation = FrameBuffer::NATURAL;

        switch (siz.get_bit_depth(0))
        {
        case 8:
            fbi.dataType = FrameBuffer::UCHAR;
            break;
        case 10:
        case 12:
            fbi.dataType = FrameBuffer::USHORT;
            break;
        case 16:
            fbi.dataType = FrameBuffer::USHORT;
            break;
        default:
            TWK_THROW_STREAM(UnsupportedException, "HTJ2K: unsupported bitdepth " << filename);
        }
    }

    void copyScanLine(ojph::codestream* codestream, ojph::ui32 width, ojph::ui32 channels, ojph::ui32 row, ojph::ui32 component,
                      FrameBuffer::DataType dtype, int bit_offset, FrameBuffer* fb)
    {

        ojph::ui32 comp_num;
        ojph::line_buf* line = codestream->pull(comp_num);
        const ojph::si32* sp = line->i32;
        assert(comp_num == component);
        if (dtype == FrameBuffer::UCHAR)
        {
            unsigned char* dout = fb->scanline<unsigned char>(row);
            dout += component;
            for (ojph::ui32 j = width; j > 0; j--, dout += channels)
            {
                *dout = *sp++;
            }
        }
        if (dtype == FrameBuffer::USHORT)
        {
            unsigned short* dout = fb->scanline<unsigned short>(row);
            dout += component;
            for (ojph::ui32 j = width; j > 0; j--, dout += channels)
            {
                *dout = *sp << bit_offset;
                sp++;
            }
        }
    }

    FrameBuffer* decodeHTJ2K(ojph::infile_base* infile, FrameBuffer* fb)
    {
        ojph::codestream codestream;
        codestream.read_headers(infile);

        // codestream.enable_resilience();
        ojph::param_siz siz = codestream.access_siz();
        ojph::param_nlt nlt = codestream.access_nlt();
        codestream.create();

        int bit_offset = 0;
        const int ch = siz.get_num_components();
        const int w = siz.get_recon_width(0);
        const int h = siz.get_recon_height(0);
        bool nlt_is_signed;
        ojph::ui8 nlt_bit_depth;
        ojph::ui8 nl_type;
        bool has_nlt = nlt.get_nonlinear_transform(0, nlt_bit_depth, nlt_is_signed, nl_type);
        bool is_signed = siz.is_signed(0);

        // Signed and notlinear transforms are not supported, but it could be added in the future.
        if (is_signed)
            TWK_THROW_STREAM(UnsupportedException, "HTJ2K: unsupported signed jpeg2000 image");

        if (has_nlt)
            TWK_THROW_STREAM(UnsupportedException, "HTJ2K: unsupported notlinear transform in jpeg2000 image");

        // 4. Wrap the decoded image in a FrameBuffer
        FrameBuffer::DataType dtype = FrameBuffer::USHORT;

        switch (siz.get_bit_depth(0))
        {
        case 8:
            dtype = FrameBuffer::UCHAR;
            break;
        case 10:
            bit_offset = 6;
            dtype = FrameBuffer::USHORT;
            break;
        case 12:
            bit_offset = 4;
        case 16:
            dtype = FrameBuffer::USHORT;
            break;
        }

        if (fb == NULL)
        {
            // If no FrameBuffer is provided, create a new one
            // Typically used when called from MovieFFMpeg
            fb = new FrameBuffer(w, h, ch, dtype);
            fb->setOrientation(FrameBuffer::BOTTOMLEFT);
        }
        else
        {
            // If a FrameBuffer is provided, restructure it
            fb->restructure(w, h, 0, ch, dtype);
            // I dont really understand why its TOPLEFT here, but BOTTOMLEFT for MovieFFMPEG
            // when the underlying data is the same.
            // I suspect it has to do with the way the FrameBuffer is used in MovieFFMPEG
            // and how it expects the orientation to be set.
            fb->setOrientation(FrameBuffer::TOPLEFT);
        }
        fb->newAttribute("fileBitDepth", siz.get_bit_depth(0));
        if (codestream.is_planar())
        {
            // Its pretty rare for RGB to be planar, possibily the most common case is a single channel image
            for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
                for (ojph::ui32 i = 0; i < h; ++i)
                    copyScanLine(&codestream, w, ch, i, c, dtype, bit_offset, fb);
        }
        else
        {
            for (ojph::ui32 i = 0; i < h; ++i)
                for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
                    copyScanLine(&codestream, w, ch, i, c, dtype, bit_offset, fb);
        }

        return fb;
    }

    void IOhtj2k::readImage(FrameBuffer& fb, const std::string& filename, const ReadRequest& request) const
    {
        // The other case where we are decoding is in IOffmpeg where we are decoding from memory, so are using a different reader
        // but both modules then call decodeHTJ2K

        ojph::j2c_infile j2c_file;
        j2c_file.open(filename.c_str());

        decodeHTJ2K(&j2c_file, &fb);
    }

} // namespace TwkFB
