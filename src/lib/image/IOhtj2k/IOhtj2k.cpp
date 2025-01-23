//
//  Copyright (c) 2009 Tweak Software. 
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

namespace TwkFB {
using namespace std;
using namespace TwkUtil;
using namespace TwkMath;

IOhtj2k::IOhtj2k(IOType type,
                 size_t chunkSize,
                 int maxAsync) 
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

IOhtj2k::~IOhtj2k()
{
}

string
IOhtj2k::about() const
{ 
    return "HTJ2K (OpenJPH)";
}

void
IOhtj2k::getImageInfo(const std::string& filename, FBInfo& fbi) const
{
    ojph::j2c_infile j2c_file;
    j2c_file.open(filename.c_str());
    
    ojph::codestream codestream;
    codestream.read_headers(&j2c_file);

    //codestream.enable_resilience();
    ojph::param_siz siz = codestream.access_siz();

    fbi.numChannels = siz.get_num_components();
    fbi.width       = siz.get_recon_width(0);
    fbi.height      = siz.get_recon_height(0);
    fbi.pixelAspect = double(1.0);
    fbi.orientation = FrameBuffer::NATURAL;

    switch(siz.get_bit_depth(0))
    {
        case 8:
            fbi.dataType    = FrameBuffer::UCHAR;
            break;
        case 10:
        case 12:
        case 16:
            fbi.dataType    = FrameBuffer::USHORT;
            break;
    }

}

ojph::si16 convert_si32_to_si16(const ojph::si32 si32_value, bool convert_special_numbers_to_finite_numbers = false)
  {
    if (si32_value > INT16_MAX)
      return INT16_MAX;
    else if (si32_value < INT16_MIN)
      return INT16_MIN;
    else if (true == convert_special_numbers_to_finite_numbers)
    {
      const ojph::si16 si16_value = (ojph::si16)si32_value;
      half half_value;
      half_value.setBits(si16_value);
      if (half_value.isFinite())
        return si16_value;

      // handle non-real number to real-number mapping
      if (half_value.isNan())
        half_value = 0.0f;
      else if (half_value.isInfinity() && !half_value.isNegative())
        half_value = HALF_MAX;
      else if (half_value.isInfinity() && half_value.isNegative())
        half_value = -1.0f * HALF_MAX;
      
      return half_value.bits();
    }  
    else
      return (ojph::si16)si32_value;
  }

void
IOhtj2k::readImage(FrameBuffer& fb,
                   const std::string& filename,
                   const ReadRequest& request) const
{
    ojph::ui32 skipped_res_for_read = 0;
    ojph::ui32 skipped_res_for_recon = 0;
    ojph::j2c_infile j2c_file;
    j2c_file.open(filename.c_str());
    
    ojph::codestream codestream;
    codestream.read_headers(&j2c_file);

    //codestream.enable_resilience();
    ojph::param_siz siz = codestream.access_siz();
    ojph::param_nlt nlt = codestream.access_nlt();
    fb.setOrientation(FrameBuffer::TOPLEFT);

    const int ch = siz.get_num_components();
    const int w = siz.get_recon_width(0);
    const int h = siz.get_recon_height(0);
    FrameBuffer::DataType dtype;

    bool nlt_is_signed;
    ojph::ui8 nlt_bit_depth;
    bool has_nlt = nlt.get_type3_transformation(0, nlt_bit_depth, nlt_is_signed);
    bool is_signed = siz.is_signed(0);
    /* For now we are setting the data type based on the first channel */
    switch(siz.get_bit_depth(0))
    {
        case 8:
            dtype    = FrameBuffer::UCHAR;
            break;
        case 10:
        case 12:
        case 16:
            if (has_nlt && is_signed)
                dtype = FrameBuffer::HALF;
            else
                dtype    = FrameBuffer::USHORT;
            break;
        case 32:
            if (has_nlt && is_signed)
                dtype = FrameBuffer::DOUBLE;
            else
                dtype    = FrameBuffer::UINT;
            break;
    }

    fb.restructure(w, h, 0, ch, dtype);
    codestream.create();

    if (codestream.is_planar()){
        for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
            
            for (ojph::ui32 i = 0; i < h; ++i)
            {
                ojph::ui32 comp_num;
                ojph::line_buf *line = codestream.pull(comp_num);
                const ojph::si32* sp = line->i32;
                assert(comp_num == c);
                if (dtype == FrameBuffer::UCHAR){
                    unsigned char* dout = fb.scanline<unsigned char>(h - i - 1);
                    dout += c;
                    for(ojph::ui32 j=w; j > 0; j--, dout += ch){
                        *dout = *sp++;
                    }
                }
                if (dtype == FrameBuffer::USHORT){
                    unsigned short* dout = fb.scanline<unsigned short>(h - i - 1);
                    dout += c;
                    for(ojph::ui32 j=w; j > 0; j--, dout += ch){
                        *dout = *sp++;
                    }    
                }
                if (dtype == FrameBuffer::HALF){
                    ojph::si16* dout = (ojph::si16 *)fb.scanline<unsigned short>(h - i - 1);
                    dout += c;
                    for(ojph::ui32 j=w; j > 0; j--, dout += ch){
                        *dout = convert_si32_to_si16(*sp++);
                    }
                }
            }
        
    } else {

        for (ojph::ui32 i = 0; i < h; ++i)
        {
            for (ojph::ui32 c = 0; c < siz.get_num_components(); ++c)
            {
                ojph::ui32 comp_num;
                ojph::line_buf *line = codestream.pull(comp_num);
                const ojph::si32* sp = line->i32;
                assert(comp_num == c);
                if (dtype == FrameBuffer::UCHAR){
                    unsigned char* dout = fb.scanline<unsigned char>(h - i - 1);
                    dout += c;
                    for(ojph::ui32 j=w; j > 0; j--, dout += ch){
                        *dout = *sp++;
                    }
                }
                if (dtype == FrameBuffer::USHORT){
                    unsigned short* dout = fb.scanline<unsigned short>(h - i - 1);
                    dout += c;
                    for(ojph::ui32 j=w; j > 0; j--, dout += ch){
                        *dout = *sp++;
                    }
                }
                if (dtype == FrameBuffer::HALF){
                    ojph::si16* dout = (ojph::si16 *)fb.scanline<unsigned short>(h - i - 1);
                    dout += c;
                    for(ojph::ui32 j=w; j > 0; j--, dout += ch){
                        *dout = convert_si32_to_si16(*sp++);
                    }
                }
            }
        }
    }

}

} // TwkFB
