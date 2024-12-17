//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOrgbe__IOrgbe__h__
#define __IOrgbe__IOrgbe__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>

namespace TwkFB
{

    class IOrgbe : public FrameBufferIO
    {
    public:
        //
        //  Types
        //

        struct rgbe_header_info
        {
            int valid;            /* indicate which fields are valid */
            char programtype[16]; /* listed at beginning of file to identify it
                                   * after "#?".  defaults to "RGBE" */
            float gamma;          /* image has already been gamma corrected with
                                   * given gamma.  defaults to 1.0 (no correction) */
            float exposure;       /* a value of 1.0 in an image corresponds to
                                   * <exposure> watts/steradian/m^2.
                                   * defaults to 1.0 */
        };

        /* flags indicating which fields in an rgbe_header_info are valid */
        static const int RGBE_VALID_PROGRAMTYPE = 0x01;
        static const int RGBE_VALID_GAMMA = 0x02;
        static const int RGBE_VALID_EXPOSURE = 0x04;

        /* return codes for rgbe routines */
        static const int RGBE_RETURN_SUCCESS = 0;
        static const int RGBE_RETURN_FAILURE = -1;

        IOrgbe();
        virtual ~IOrgbe();

        virtual void readImage(FrameBuffer&, const std::string& filename,
                               const ReadRequest& request) const;
        virtual void writeImage(const FrameBuffer&, const std::string& filename,
                                const WriteRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

        /* read or write headers */
        /* you may set rgbe_header_info to null if you want to */
        int RGBE_WriteHeader(FILE* fp, int width, int height,
                             rgbe_header_info* info) const;
        int RGBE_ReadHeader(FILE* fp, int* width, int* height,
                            rgbe_header_info* info, bool silent = false) const;
        int RGBE_ReadHeader_OLD(FILE* fp, int* width, int* height,
                                rgbe_header_info* info,
                                bool silent = false) const;

        /* read or write pixels */
        /* can read or write pixels in chunks of any size including single
         * pixels*/
        int RGBE_WritePixels(FILE* fp, float* data, int numpixels) const;
        int RGBE_ReadPixels(FILE* fp, float* data, int numpixels) const;

        /* read or write run length encoded files */
        /* must be called to read or write whole scanlines */
        int RGBE_WritePixels_RLE(FILE* fp, float* data, int scanline_width,
                                 int num_scanlines) const;
        int RGBE_ReadPixels_RLE(FILE* fp, float* data, int scanline_width,
                                int num_scanlines) const;

        int rgbe_error(int rgbe_error_code, char* msg,
                       bool silent = false) const;
        void float2rgbe(unsigned char rgbe[4], float red, float green,
                        float blue) const;
        void rgbe2float(float* red, float* green, float* blue,
                        unsigned char rgbe[4]) const;
        int RGBE_WriteBytes_RLE(FILE* fp, unsigned char* data,
                                int numbytes) const;
    };

} // namespace TwkFB

#endif // __IOrgbe__IOrgbe__h__
