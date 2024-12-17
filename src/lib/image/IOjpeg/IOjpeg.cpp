//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <iostream>
#include <string>
#include <IOjpeg/IOjpeg.h>
#include <TwkFB/Operations.h>
#include <TwkFB/Exception.h>
#include <TwkUtil/File.h>
#include <TwkUtil/FileStream.h>
#include <stdlib.h>

extern "C"
{
#include <jpeglib.h>
#include <libexif/exif-entry.h>
}

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;

#define EXIF_MARKER (JPEG_APP0 + 1)
#define ICC_MARKER (JPEG_APP0 + 2)

    struct FileStreamSource : public jpeg_source_mgr
    {
        FileStream* stream;
    };

    IOjpeg::IOjpeg(StorageFormat format, IOType type, size_t chunkSize,
                   int maxAsync)
        : StreamingFrameBufferIO("IOjpeg", "m2", type, chunkSize, maxAsync)
        , m_error(false)
        , m_format(format)
    {
        //
        //  Indicate which extensions this plugin will handle The
        //  comparison against. The extensions are case-insensitive so
        //  there's no reason to provide upper case versions.
        //

        unsigned int cap = ImageRead | ImageWrite | Int8Capable | BruteForceIO;

        addType("jpeg", "JPEG Image", cap);
        addType("jpg", "JPEG Image", cap);

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
    }

    IOjpeg::~IOjpeg() {}

    string IOjpeg::about() const
    {
        char temp[80];
        sprintf(temp, "JPEG (IJG %d)", JPEG_LIB_VERSION);
        return temp;
    }

    void IOjpeg::throwError(const FileState& state) const
    {
        if (m_error)
        {
            m_error = false;
            TWK_THROW_STREAM(TwkFB::IOException,
                             "JPEG: failed to open jpeg file \""
                                 << state.filename << "\"");
        }
    }

    static const char* colorSpaceName(J_COLOR_SPACE space)
    {
        switch (space)
        {
        default:
        case JCS_UNKNOWN:
            return "UNKNOWN";
        case JCS_GRAYSCALE:
            return "GRAYSCALE";
        case JCS_RGB:
            return "RGB";
        case JCS_YCbCr:
            return "YCbCr";
        case JCS_CMYK:
            return "CMYK";
        case JCS_YCCK:
            return "YCbCrK";
        }
    }

    static void iojpeg_error_exit(j_common_ptr cinfo)
    {
        IOjpeg* o = (IOjpeg*)cinfo->client_data;
        o->m_error = true;
    }

    static void init_source(j_decompress_ptr cinfo)
    {
        FileStreamSource* source = (FileStreamSource*)cinfo->src;
        // cout << "stream size is " << source->stream->size() << endl;
        source->next_input_byte = (JOCTET*)source->stream->data();
        source->bytes_in_buffer = source->stream->size();
    }

    static boolean fill_input_buffer(j_decompress_ptr cinfo)
    {
        // cout << "in fill_input_buffer" << endl;
        return TRUE;
    }

    static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
    {
        // cout << "in skip_input_data num_bytes = " << num_bytes << endl;
        FileStreamSource* source = (FileStreamSource*)cinfo->src;
        source->next_input_byte += num_bytes;
        source->bytes_in_buffer -= num_bytes;
    }

    static void term_source(j_decompress_ptr cinfo)
    {
        // cout << "in term_source" << endl;
    }

    void IOjpeg::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        FileState state(filename);

        cinfo.err = jpeg_std_error(&jerr);
        cinfo.err->error_exit = iojpeg_error_exit;
        jpeg_create_decompress(&cinfo);
        cinfo.client_data = (void*)this;

        if (!(state.file = TwkUtil::fopen(state.filename.c_str(), "rb")))
        {
            TWK_THROW_STREAM(IOException, "JPEG: cannot open " << filename);
        }

        jpeg_save_markers(&cinfo, JPEG_COM,
                          0xFFFF); // comment marker (0xffff == all)
        jpeg_save_markers(&cinfo, EXIF_MARKER, 0xFFFF); // EXIF marker
        jpeg_save_markers(&cinfo, ICC_MARKER, 0xFFFF);  // ICC marker

        unsigned int magic = (unsigned int)getc(state.file) << 16
                             | (unsigned int)getc(state.file) << 8
                             | (unsigned int)getc(state.file);

        if (magic != 0xFFD8FF)
        {
            TWK_THROW_STREAM(IOException,
                             "JPEG: not a jpeg file: " << filename);
        }
        else
        {
            fseek(state.file, 0, SEEK_SET);
        }

        jpeg_stdio_src(&cinfo, state.file);

        if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK)
        {
            TWK_THROW_STREAM(TwkFB::IOException,
                             "JPEG: not a jpeg file \"" << filename);
        }

        throwError(state);
        jpeg_start_decompress(&cinfo);

        throwError(state);

        size_t pixels = cinfo.output_width * cinfo.output_height;
        size_t bytes = pixels * cinfo.output_components;
        size_t scanline_bytes = cinfo.output_components * cinfo.output_width;
        size_t w = cinfo.output_width;
        size_t h = cinfo.output_height;
        size_t components = cinfo.output_components;

        fbi.dataType = FrameBuffer::UCHAR;
        fbi.orientation = FrameBuffer::NATURAL;
        fbi.width = w;
        fbi.height = h;
        fbi.numChannels = components;

        if (cinfo.X_density != 0 && cinfo.Y_density != 0)
        {
            fbi.pixelAspect = double(cinfo.X_density) / double(cinfo.Y_density);
        }
        else
        {
            fbi.pixelAspect = 1.0;
        }

        readAttributes(fbi.proxy, cinfo);
    }

    //
    //  Main entry point for reading
    //

    void IOjpeg::readImage(FrameBuffer& fb, const std::string& filename,
                           const ReadRequest& request) const
    {
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr);
        cinfo.err->error_exit = iojpeg_error_exit;
        jpeg_create_decompress(&cinfo);
        cinfo.client_data = (void*)this;

        jpeg_save_markers(&cinfo, JPEG_COM,
                          0xFFFF); // comment marker (0xffff == all)
        jpeg_save_markers(&cinfo, EXIF_MARKER, 0xFFFF); // EXIF marker
        jpeg_save_markers(&cinfo, ICC_MARKER, 0xFFFF);  // ICC marker

        FileState state(filename);
        FileStreamSource incore_src;

        if (m_iotype == StandardIO)
        {
            if (!(state.file = TwkUtil::fopen(state.filename.c_str(), "rb")))
            {
                TWK_THROW_STREAM(IOException, "Cannot open " << state.filename
                                                             << " for reading");
            }

            jpeg_stdio_src(&cinfo, state.file);
        }
        else
        {
            state.stream = new FileStream(
                state.filename, (FileStream::Type)((unsigned int)m_iotype - 1),
                m_iosize, m_iomaxAsync);

            incore_src.stream = state.stream;
            incore_src.init_source = init_source;
            incore_src.fill_input_buffer = fill_input_buffer;
            incore_src.skip_input_data = skip_input_data;
            incore_src.resync_to_restart = jpeg_resync_to_restart;
            incore_src.term_source = term_source;
            incore_src.next_input_byte = 0;
            incore_src.bytes_in_buffer = 0;

            cinfo.src = &incore_src;
        }

        jpeg_read_header(&cinfo, TRUE);

        //
        //  Uncommenting this will allow direct YUV raw reading. Without
        //  it, the scanline size test fails because the computed output
        //  size is 0 x 0
        //
        // jpeg_calc_output_dimensions(&cinfo);

        throwError(state);

        if (cinfo.Y_density != cinfo.X_density)
        {
            fb.setPixelAspectRatio(
                float(double(cinfo.X_density) / double(cinfo.Y_density)));
        }

        StorageFormat outformat = RGB;
        StorageFormat informat = m_format;

        if (m_format == YUV)
        {
            if (canReadAsYUV(cinfo))
            {
            }
            else
            {
                informat = RGB;
            }
        }

        if (informat == YUV && cinfo.num_components == 3
            && cinfo.progressive_mode == FALSE
            && cinfo.jpeg_color_space == JCS_YCbCr)
        {
            //
            //  Use YUV reader if request and for three channel
            //  YCbCr colorspace images only
            //

            outformat = informat;
        }
        else if (informat == RGBA && cinfo.num_components == 3
                 && cinfo.progressive_mode == FALSE)
        {
            outformat = informat;
        }

        if (outformat == YUV)
            cinfo.raw_data_out = TRUE;

        jpeg_start_decompress(&cinfo);
        throwError(state);

        switch (outformat)
        {
        default:
        case RGB:
            readImageRGB(fb, state, cinfo);
            break;
        case RGBA:
            readImageRGBA(fb, state, cinfo);
            break;
        case YUV:
            readImageYUV(fb, state, cinfo);
            break;
        }

        readAttributes(fb, cinfo);
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
    }

    void IOjpeg::planarConfig(FrameBuffer& fb, Decompressor& cinfo) const
    {
        FrameBuffer* Y = &fb;
        FrameBuffer* U = 0;
        FrameBuffer* V = 0;

        int w = cinfo.output_width;
        int h = cinfo.output_height;

        FrameBuffer::Orientation orient = FrameBuffer::NATURAL;
        FrameBuffer::DataType type = FrameBuffer::UCHAR;

        int chw[3];
        int chh[3];
        int chx[3];

        for (int i = 0; i < cinfo.comps_in_scan; i++)
        {
            jpeg_component_info* c = cinfo.cur_comp_info[i];
            int rows = c->height_in_blocks * DCTSIZE;

            chw[i] = int(double(c->h_samp_factor)
                         / double(cinfo.max_h_samp_factor) * double(w));

            chh[i] = int(double(c->v_samp_factor)
                         / double(cinfo.max_v_samp_factor) * double(h));

            chx[i] = rows - chh[i];
        }

        Y->restructure(chw[0], chh[0], 0, 1, type, 0, 0, orient, true, chx[0]);

        if (fb.numPlanes() == 3)
        {
            U = fb.nextPlane();
            V = U->nextPlane();

            U->restructure(chw[1], chh[1], 0, 1, type, 0, 0, orient, true,
                           chx[1]);
            V->restructure(chw[2], chh[2], 0, 1, type, 0, 0, orient, true,
                           chx[2]);
        }
        else
        {
            if (fb.numPlanes() != 1)
            {
                FrameBuffer* old = fb.nextPlane();
                fb.removePlane(old);
                delete old;
            }

            U = new FrameBuffer(FrameBuffer::PixelCoordinates, chw[1], chh[1],
                                0, 1, type, 0, 0, orient, true, chx[1]);
            V = new FrameBuffer(FrameBuffer::PixelCoordinates, chw[2], chh[2],
                                0, 1, type, 0, 0, orient, true, chx[2]);

            fb.appendPlane(U);
            fb.appendPlane(V);
        }

        Y->setChannelName(0, "Y");
        U->setChannelName(0, "U");
        V->setChannelName(0, "V");
    }

//
// Taken from
// http://docs.scribus.net/devel/scimgdataloader__jpeg_8cpp-source.html
//

// #define EXIF_MARKER  (JPEG_APP0 + 1)
// #define ICC_MARKER  (JPEG_APP0 + 2)
#define ICC_OVERHEAD_LEN 14
#define EXIF_OVERHEAD_LEN 6
#define MAX_BYTES_IN_MARKER 65533
#define MAX_DATA_BYTES_IN_MARKER (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)
#define MAX_SEQ_NO 255
#define PHOTOSHOP_MARKER (JPEG_APP0 + 13)

    static bool marker_is_icc(jpeg_saved_marker_ptr marker)
    {
        return marker->marker == ICC_MARKER
               && marker->data_length >= ICC_OVERHEAD_LEN &&
               /* verify the identifying string */
               GETJOCTET(marker->data[0]) == 0x49
               && GETJOCTET(marker->data[1]) == 0x43
               && GETJOCTET(marker->data[2]) == 0x43
               && GETJOCTET(marker->data[3]) == 0x5F
               && GETJOCTET(marker->data[4]) == 0x50
               && GETJOCTET(marker->data[5]) == 0x52
               && GETJOCTET(marker->data[6]) == 0x4F
               && GETJOCTET(marker->data[7]) == 0x46
               && GETJOCTET(marker->data[8]) == 0x49
               && GETJOCTET(marker->data[9]) == 0x4C
               && GETJOCTET(marker->data[10]) == 0x45
               && GETJOCTET(marker->data[11]) == 0x0;
    }

    static bool marker_is_exif(jpeg_saved_marker_ptr marker)
    {
        return marker->marker == EXIF_MARKER
               && marker->data_length >= EXIF_OVERHEAD_LEN &&
               /* verify the identifying string */
               GETJOCTET(marker->data[0]) == 'E'
               && GETJOCTET(marker->data[1]) == 'x'
               && GETJOCTET(marker->data[2]) == 'i'
               && GETJOCTET(marker->data[3]) == 'f'
               && GETJOCTET(marker->data[4]) == 0x0
               && GETJOCTET(marker->data[5]) == 0x0;
    }

    static bool marker_is_photoshop(jpeg_saved_marker_ptr marker)
    {
        return marker->marker == PHOTOSHOP_MARKER
               && marker->data_length >= ICC_OVERHEAD_LEN &&
               /* verify the identifying string */
               GETJOCTET(marker->data[0]) == 0x50
               && GETJOCTET(marker->data[1]) == 0x68
               && GETJOCTET(marker->data[2]) == 0x6F
               && GETJOCTET(marker->data[3]) == 0x74
               && GETJOCTET(marker->data[4]) == 0x6F
               && GETJOCTET(marker->data[5]) == 0x73
               && GETJOCTET(marker->data[6]) == 0x68
               && GETJOCTET(marker->data[7]) == 0x6F
               && GETJOCTET(marker->data[8]) == 0x70
               && GETJOCTET(marker->data[9]) == 0x20
               && GETJOCTET(marker->data[10]) == 0x33
               && GETJOCTET(marker->data[11]) == 0x2E
               && GETJOCTET(marker->data[12]) == 0x30
               && GETJOCTET(marker->data[13]) == 0x0;
    }

    static bool read_jpeg_marker(UINT8 requestmarker, j_decompress_ptr cinfo,
                                 JOCTET** icc_data_ptr,
                                 unsigned int* icc_data_len)
    {
        jpeg_saved_marker_ptr marker;
        int num_markers = 0;
        int seq_no;
        JOCTET* icc_data;
        unsigned int total_length;
#define MAX_SEQ_NO 255 /* sufficient since marker numbers are bytes */
        char marker_present[MAX_SEQ_NO + 1]; /* 1 if marker found */
        unsigned int
            data_length[MAX_SEQ_NO + 1]; /* size of profile data in marker */
        unsigned int
            data_offset[MAX_SEQ_NO + 1]; /* offset for data in marker */

        *icc_data_ptr = NULL; /* avoid confusion if false return */
        *icc_data_len = 0;

        /* This first pass over the saved markers discovers whether there are
         * any ICC markers and verifies the consistency of the marker numbering.
         */

        for (seq_no = 1; seq_no <= MAX_SEQ_NO; seq_no++)
            marker_present[seq_no] = 0;
        seq_no = 0;
        for (marker = cinfo->marker_list; marker != NULL; marker = marker->next)
        {
            if (requestmarker == ICC_MARKER && marker_is_icc(marker))
            {
                if (num_markers == 0)
                    num_markers = GETJOCTET(marker->data[13]);
                else if (num_markers != GETJOCTET(marker->data[13]))
                    return false; /* inconsistent num_markers fields */
                seq_no = GETJOCTET(marker->data[12]);
                if (seq_no <= 0 || seq_no > num_markers)
                    return false; /* bogus sequence number */
                if (marker_present[seq_no])
                    return false; /* duplicate sequence numbers */
                marker_present[seq_no] = 1;
                data_length[seq_no] = marker->data_length - ICC_OVERHEAD_LEN;
            }
            else if (requestmarker == PHOTOSHOP_MARKER
                     && marker_is_photoshop(marker))
            {
                num_markers = ++seq_no;
                marker_present[seq_no] = 1;
                data_length[seq_no] = marker->data_length - ICC_OVERHEAD_LEN;
            }
            else if (requestmarker == EXIF_MARKER && marker_is_exif(marker))
            {
                num_markers = ++seq_no;
                marker_present[seq_no] = 1;
                data_length[seq_no] = marker->data_length - EXIF_OVERHEAD_LEN;
            }
        }

        if (num_markers == 0)
            return false;

        /* Check for missing markers, count total space needed,
         * compute offset of each marker's part of the data.
         */

        total_length = 0;
        for (seq_no = 1; seq_no <= num_markers; seq_no++)
        {
            if (marker_present[seq_no] == 0)
                return false; /* missing sequence number */
            data_offset[seq_no] = total_length;
            total_length += data_length[seq_no];
        }

        if (total_length <= 0)
            return false; /* found only empty markers? */

        /* Allocate space for assembled data */
        icc_data = (JOCTET*)malloc(total_length * sizeof(JOCTET));
        if (icc_data == NULL)
            return false; /* oops, out of memory */
        seq_no = 0;
        /* and fill it in */
        for (marker = cinfo->marker_list; marker != NULL; marker = marker->next)
        {
            if ((requestmarker == ICC_MARKER && marker_is_icc(marker))
                || (requestmarker == PHOTOSHOP_MARKER
                    && marker_is_photoshop(marker))
                || (requestmarker == EXIF_MARKER && marker_is_exif(marker))
                || (requestmarker == 0xE1))
            {
                JOCTET FAR* src_ptr;
                JOCTET* dst_ptr;
                unsigned int length;
                if (requestmarker == ICC_MARKER)
                {
                    seq_no = GETJOCTET(marker->data[12]);
                    src_ptr = marker->data + ICC_OVERHEAD_LEN;
                }
                else if (requestmarker == PHOTOSHOP_MARKER)
                {
                    seq_no++;
                    src_ptr = marker->data + ICC_OVERHEAD_LEN;
                }
                else if (requestmarker == EXIF_MARKER)
                {
                    seq_no++;
                    src_ptr = marker->data + EXIF_OVERHEAD_LEN;
                }

                dst_ptr = icc_data + data_offset[seq_no];
                length = data_length[seq_no];

                while (length--)
                {
                    *dst_ptr++ = *src_ptr++;
                }
            }
        }

        *icc_data_ptr = icc_data;
        *icc_data_len = total_length;

        return true;
    }

    void IOjpeg::readAttributes(FrameBuffer& fb,
                                struct jpeg_decompress_struct& cinfo) const
    {
        //
        //  COM/EXIF/ICC data
        //
        //  We check for an ICC profile as well color space tags in EXIF
        //

        bool flagged_sRGB = false;
        bool flagged_AdobeRGB = false;
        bool flagged_ICC = false;

        for (jpeg_marker_struct* marker = cinfo.marker_list; marker;
             marker = marker->next)
        {
            if (marker->marker == JPEG_COM)
            {
                string s((char*)(marker->data), marker->data_length);
                fb.newAttribute("Comment", s);
            }
            else if (marker->marker == EXIF_MARKER)
            {
                JOCTET* exif_buf = NULL;
                unsigned int exif_len = 0;

                ExifData* exif =
                    exif_data_new_from_data(marker->data, marker->data_length);

                if (exif)
                {
                    ExifByteOrder o = exif_data_get_byte_order(exif);

                    for (int i = 0; i < EXIF_IFD_COUNT; i++)
                    {
                        ExifContent* con = exif->ifd[i];

                        for (int j = 0; j < con->count; j++)
                        {
                            ExifEntry* e = con->entries[j];

                            if (e->tag
                                == EXIF_TAG_COLOR_SPACE) // Color Space is a
                                                         // special case
                            {
                                ExifShort v_short = exif_get_short(e->data, o);

                                if (v_short == 1)
                                {
                                    flagged_sRGB = true;
                                    fb.newAttribute("EXIF/ColorSpace",
                                                    string("1 (sRGB)"));
                                }
                                else if (v_short == 2)
                                {
                                    flagged_AdobeRGB = true;
                                    fb.newAttribute("EXIF/ColorSpace",
                                                    string("2 (Adobe RGB)"));
                                }
                                else if (v_short == 0xffff)
                                {
                                    fb.newAttribute(
                                        "EXIF/ColorSpace",
                                        string("0xffff (Uncalibrated)"));
                                }
                            }
                            else
                            {
                                //
                                //  We'll just use libexif to make all
                                //  properties into strings. If you want a
                                //  non-string, you'll have to make a special
                                //  case.
                                //

                                const char* name = exif_tag_get_name(e->tag);

                                char value[1024];
                                exif_entry_get_value(e, value, sizeof(value));

                                fb.newAttribute(string("EXIF/") + name,
                                                string(value));
                            }
                        }
                    }
                }
            }
            else if (marker->marker == ICC_MARKER)
            {
                JOCTET* icc_buf = NULL;
                unsigned int icc_len = 0;

                if (read_jpeg_marker(ICC_MARKER, &cinfo, &icc_buf, &icc_len))
                {
                    flagged_ICC = true;

                    fb.setICCprofile(icc_buf, icc_len);
                }

                if (icc_buf)
                {
                    free(icc_buf);
                }
            }
        }

        if (flagged_ICC)
        {
            fb.setPrimaryColorSpace(
                ColorSpace::ICCProfile()); // profile name already set
            fb.setTransferFunction(
                ColorSpace::ICCProfile()); // profile name already set
        }
        else if (flagged_sRGB)
        {
            fb.setPrimaryColorSpace(ColorSpace::Rec709());
            fb.setTransferFunction(ColorSpace::sRGB());
        }
        else if (flagged_AdobeRGB)
        {
            fb.setPrimaryColorSpace(ColorSpace::ICCProfile());
            fb.attribute<string>(ColorSpace::ICCProfileDescription()) =
                string("Adobe RGB");
        }

        //
        //  Get the sampling information
        //

        int maxsample = 1;
        vector<int> xsamps(cinfo.comps_in_scan);
        vector<int> ysamps(cinfo.comps_in_scan);
        bool allsame = true;

        for (int i = 0; i < cinfo.comps_in_scan; i++)
        {
            jpeg_component_info* c = cinfo.cur_comp_info[i];
            int xs = c->h_samp_factor;
            int ys = c->v_samp_factor;
            xsamps[i] = xs;
            ysamps[i] = ys;
            if (xs != ys)
                allsame = false;
            maxsample = std::max(maxsample, xs);
            maxsample = std::max(maxsample, ys);
        }

        ostringstream str;

        for (int i = 0; i < xsamps.size(); i++)
        {
            if (i)
                str << ":";
            str << xsamps[i];
        }

        if (!allsame)
        {
            str << " h, ";

            for (int i = 0; i < ysamps.size(); i++)
            {
                if (i)
                    str << ":";
                str << ysamps[i];
            }

            str << " v";
        }

        fb.newAttribute("JPEG/Sampling", str.str());

        fb.newAttribute("JPEG/ColorSpace",
                        string(colorSpaceName(cinfo.jpeg_color_space)));

        if (cinfo.saw_Adobe_marker)
        {
            fb.newAttribute("JPEG/AdobeTransformCode",
                            int(cinfo.Adobe_transform));
        }

        fb.newAttribute(
            "JPEG/Encoding",
            string(cinfo.arith_code == TRUE ? "Arithmetic" : "Huffman"));

        ostringstream den;
        if (cinfo.X_density != cinfo.Y_density)
        {
            den << int(cinfo.X_density) << "x" << int(cinfo.Y_density);
        }
        else
        {
            den << int(cinfo.X_density);
        }

        switch (cinfo.density_unit)
        {
        case 1:
            den << " dpi";
            break;
        case 2:
            den << " dots/cm";
            break;
        }

        fb.newAttribute("JPEG/Density", den.str());

        if (cinfo.X_density != cinfo.Y_density)
        {
            fb.newAttribute("JPEG/PixelAspect",
                            float(cinfo.X_density) / float(cinfo.Y_density));
        }

        if (cinfo.progressive_mode)
        {
            fb.newAttribute("JPEG/Mode", string("Progressive"));
        }

        if (cinfo.output_gamma != 1.0)
        {
            fb.newAttribute("JPEG/Gamma", float(cinfo.output_gamma));
        }

        ostringstream ver;
        ver << int(cinfo.JFIF_major_version) << "."
            << int(cinfo.JFIF_minor_version);
        fb.newAttribute("JPEG/Version", ver.str());
    }

    void IOjpeg::readImageRGB(FrameBuffer& fb, const FileState& state,
                              Decompressor& cinfo) const
    {
        //
        //  This is the basic JPEG -> RGB interleaved reader.
        //  It allows libjpeg to do the color conversion.
        //

        size_t pixels = cinfo.output_width * cinfo.output_height;
        size_t bytes = pixels * cinfo.output_components;
        size_t scanline_bytes = cinfo.output_components * cinfo.output_width;
        size_t w = cinfo.output_width;
        size_t h = cinfo.output_height;
        size_t components = cinfo.output_components;

        fb.restructure(w, h, 0, components, FrameBuffer::UCHAR);

        while (cinfo.output_scanline < h)
        {
            int l = cinfo.output_scanline;
            unsigned char* scanline = fb.scanline<unsigned char>(h - l - 1);
            jpeg_read_scanlines(&cinfo, &scanline, 1);
            throwError(state);
        }
    }

    void IOjpeg::readImageRGBA(FrameBuffer& fb, const FileState& state,
                               Decompressor& cinfo) const
    {
        //
        //  This is the basic JPEG -> RGB interleaved reader.
        //  It allows libjpeg to do the color conversion.
        //

        size_t pixels = cinfo.output_width * cinfo.output_height;
        size_t bytes = pixels * cinfo.output_components;
        size_t scanline_bytes = cinfo.output_components * cinfo.output_width;
        size_t w = cinfo.output_width;
        size_t h = cinfo.output_height;
        size_t components = cinfo.output_components;

        fb.restructure(w, h, 0, 4, FrameBuffer::UCHAR);
        vector<unsigned char> buffer(w * 3);
        unsigned char* inscanline = &buffer.front();
        const unsigned char* endp = &buffer.back();

        while (cinfo.output_scanline < h)
        {
            int l = cinfo.output_scanline;
            unsigned char* scanline = fb.scanline<unsigned char>(h - l - 1);
            jpeg_read_scanlines(&cinfo, &inscanline, 1);

            for (unsigned char *inp = inscanline, *outp = scanline;
                 inp <= endp;)
            {
                *outp = *inp;
                outp++;
                inp++;
                *outp = *inp;
                outp++;
                inp++;
                *outp = *inp;
                outp++;
                inp++;
                *outp = 255;
                outp++;
            }

            throwError(state);
        }
    }

    bool IOjpeg::canReadAsYUV(const Decompressor& cinfo) const
    {
        int w = cinfo.output_width;

        for (int i = 0; i < 3; i++)
        {
            jpeg_component_info* info = cinfo.cur_comp_info[i];
            int rowsize = info->width_in_blocks * DCTSIZE;
            int cw = int(double(info->h_samp_factor)
                         / double(cinfo.max_h_samp_factor) * double(w));

            if (cw != rowsize)
                return false;
        }

        return true;
    }

    void IOjpeg::readImageYUV(FrameBuffer& fb, const FileState& state,
                              Decompressor& cinfo) const
    {
        //
        //  Raw mode, read the Y Cb Cr planes directly
        //

        size_t pixels = cinfo.output_width * cinfo.output_height;
        size_t bytes = pixels * cinfo.output_components;
        size_t scanline_bytes = cinfo.output_components * cinfo.output_width;
        size_t w = cinfo.output_width;
        size_t h = cinfo.output_height;
        size_t components = cinfo.output_components;

        jpeg_component_info* yinfo = cinfo.cur_comp_info[0];
        jpeg_component_info* uinfo = cinfo.cur_comp_info[1];
        jpeg_component_info* vinfo = cinfo.cur_comp_info[2];

        int yrowsize = yinfo->width_in_blocks * DCTSIZE;
        int urowsize = uinfo->width_in_blocks * DCTSIZE;
        int vrowsize = vinfo->width_in_blocks * DCTSIZE;
        int yrows = yinfo->height_in_blocks * DCTSIZE;
        int urows = uinfo->height_in_blocks * DCTSIZE;
        int vrows = vinfo->height_in_blocks * DCTSIZE;

        JSAMPROW* image[3];
        vector<JSAMPROW> yscanlines(yrows);
        vector<JSAMPROW> uscanlines(urows);
        vector<JSAMPROW> vscanlines(vrows);

        image[0] = &yscanlines.front();
        image[1] = &uscanlines.front();
        image[2] = &vscanlines.front();

        int yinc = DCTSIZE * yinfo->v_samp_factor;
        int uinc = DCTSIZE * uinfo->v_samp_factor;
        int vinc = DCTSIZE * vinfo->v_samp_factor;

        planarConfig(fb, cinfo);
        FrameBuffer* Y = &fb;
        FrameBuffer* U = Y->nextPlane();
        FrameBuffer* V = U->nextPlane();

        int yh = Y->height();
        int uh = U->height();
        int vh = V->height();

        // if (yrowsize != Y->width() || urowsize != U->width() || vrowsize !=
        // V->width() || yrows != Y->height() || urows != U->height() || vrows
        // != V->height())
        if (yrowsize != Y->width() || urowsize != U->width()
            || vrowsize != V->width())
        {
            //
            //  We can't do a direct read. Instead, we need to read into
            //  temp buffers and copy the scanlines to the final images
            //

#if 1
            cout << "INFO: IOjpeg: non-direct read"
                 << ", yrowsize = " << yrowsize << ", urowsize = " << urowsize
                 << ", vrowsize = " << vrowsize << ", yrows = " << yrows
                 << ", urows = " << urows << ", vrows = " << vrows << endl;
#endif

            vector<JSAMPLE> ybuffer(yrowsize * yrows);
            vector<JSAMPLE> ubuffer(urowsize * urows);
            vector<JSAMPLE> vbuffer(vrowsize * vrows);

            for (int i = 0; i < yrows; i++)
                yscanlines[i] = &ybuffer[i * yrowsize];
            for (int i = 0; i < urows; i++)
                uscanlines[i] = &ubuffer[i * urowsize];
            for (int i = 0; i < vrows; i++)
                vscanlines[i] = &vbuffer[i * vrowsize];

            for (int yr = 0, ur = 0, vr = 0; yr < Y->height();
                 yr += yinc, ur += uinc, vr += vinc)
            {
                jpeg_read_raw_data(&cinfo, image, yinc);
                throwError(state);

                for (int i = 0; i < yrows && (i + yr) < yh; i++)
                {
                    memcpy(Y->scanline<JSAMPLE>(yh - (i + yr) - 1),
                           &ybuffer[i * yrowsize],
                           Y->width() * sizeof(JSAMPLE));
                }

                for (int i = 0; i < urows && (i + ur) < uh; i++)
                {
                    memcpy(U->scanline<JSAMPLE>(uh - (i + ur) - 1),
                           &ubuffer[i * urowsize],
                           U->width() * sizeof(JSAMPLE));
                }

                for (int i = 0; i < vrows && (i + vr) < vh; i++)
                {
                    memcpy(V->scanline<JSAMPLE>(vh - (i + vr) - 1),
                           &vbuffer[i * vrowsize],
                           V->width() * sizeof(JSAMPLE));
                }
            }
        }
        else
        {
            //
            //  Direct read into memory
            //

#if 0
        cout << "INFO: IOjpeg: direct read" << endl;
#endif

            for (int i = 0; i < yrows; i++)
                yscanlines[i] = Y->scanline<JSAMPLE>(i);
            for (int i = 0; i < urows; i++)
                uscanlines[i] = U->scanline<JSAMPLE>(i);
            for (int i = 0; i < vrows; i++)
                vscanlines[i] = V->scanline<JSAMPLE>(i);

            for (int row = 0; row < yrows; row += yinc)
            {
                jpeg_read_raw_data(&cinfo, image, yinc);
                image[0] += yinc;
                image[1] += uinc;
                image[2] += vinc;

                throwError(state);
            }

            Y->setOrientation(FrameBuffer::TOPLEFT);
            U->setOrientation(FrameBuffer::TOPLEFT);
            V->setOrientation(FrameBuffer::TOPLEFT);
        }
    }

    void IOjpeg::writeImage(const FrameBuffer& img, const std::string& filename,
                            const WriteRequest& request) const
    {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        const FrameBuffer* outfb = &img;

        int X_density = 1;
        int Y_density = 1;
        float gamma = 1.0;

        //
        //  Check on output params
        //

        for (size_t i = 0; i < request.parameters.size(); i++)
        {
            const StringPair pair = request.parameters[i];

            string name = pair.first;
            string value = pair.second;

            if (name == "output/pa")
            {
                double pa = atof(value.c_str());
                X_density = int(1000000 * pa);
                Y_density = int(1000000);
            }
            else if (name == "output/pa/numerator")
                X_density = atoi(value.c_str());
            else if (name == "output/pa/denominator")
                Y_density = atoi(value.c_str());
            else if (name == "output/gamma")
                gamma = atof(value.c_str());
        }

        //
        //  Convert to UCHAR packed if not already.
        //

        if (img.isPlanar())
        {
            const FrameBuffer* fb = outfb;
            outfb = mergePlanes(outfb);
            if (fb != &img)
                delete fb;
        }

        //
        //  Convert everything to REC709
        //

        if (outfb->hasPrimaries() || outfb->isYUV() || outfb->isYRYBY()
            || outfb->dataType() >= FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            const FrameBuffer* fb = outfb;
            outfb = convertToLinearRGB709(outfb);
            if (fb != &img)
                delete fb;
        }

        //
        //  Convert to 3 channel, RGB
        //

        if (img.numChannels() != 3 || img.channelName(0) != "R"
            || img.channelName(1) != "G" || img.channelName(2) != "B")
        {
            const FrameBuffer* fb = outfb;
            vector<string> mapping;
            mapping.push_back("R");
            mapping.push_back("G");
            mapping.push_back("B");
            outfb = channelMap(const_cast<FrameBuffer*>(outfb), mapping);
            if (fb != &img)
                delete fb;
        }

        switch (img.dataType())
        {
        case FrameBuffer::HALF:
        case FrameBuffer::FLOAT:
        case FrameBuffer::USHORT:
        {
            const FrameBuffer* fb = outfb;
            outfb = copyConvert(outfb, FrameBuffer::UCHAR);
            if (fb != &img)
                delete fb;
            break;
        }
        default:
            break;
        }

        //
        //  Flip and Flop to get in the right orientation
        //

        bool needflip = false;
        bool needflop = false;

        switch (outfb->orientation())
        {
        case FrameBuffer::TOPLEFT:
            needflip = true;
            break;
        case FrameBuffer::TOPRIGHT:
        case FrameBuffer::BOTTOMRIGHT:
            needflop = true;
            break;
        default:
            break;
        };

        if (needflop)
        {
            if (outfb == &img)
                outfb = img.copy();
            flop(const_cast<FrameBuffer*>(outfb));
        }

        cinfo.err = jpeg_std_error(&jerr);
        cinfo.err->error_exit = iojpeg_error_exit;
        jpeg_create_compress(&cinfo);
        cinfo.client_data = (void*)this;

        FileState state(filename);

        if (!(state.file = TwkUtil::fopen(state.filename.c_str(), "wb")))
        {
            TWK_THROW_STREAM(IOException,
                             "cannot open \"" << filename << "\" for writing");
        }

        jpeg_stdio_dest(&cinfo, state.file);

        int h = outfb->height();

        cinfo.write_JFIF_header = TRUE;
        cinfo.input_gamma = gamma;
        cinfo.X_density = X_density;
        cinfo.Y_density = Y_density;
        cinfo.image_width = outfb->width();
        cinfo.image_height = outfb->height();
        cinfo.input_components = outfb->numChannels(); // really only 3 is ok
        cinfo.in_color_space = JCS_RGB;

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, int(request.quality * 100.0), FALSE);

        jpeg_start_compress(&cinfo, TRUE);
        throwError(state);

        while (cinfo.next_scanline < h)
        {
            int l = cinfo.next_scanline;
            int lindex = needflip ? l : h - l - 1;
            const unsigned char* scanline =
                outfb->scanline<unsigned char>(lindex);
            jpeg_write_scanlines(&cinfo, (JSAMPLE**)&scanline, 1);
            throwError(state);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);

        if (outfb != &img)
            delete outfb;
    }

} //  End namespace TwkFB
