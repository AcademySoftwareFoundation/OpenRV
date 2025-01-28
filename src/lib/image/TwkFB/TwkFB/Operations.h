//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkFB__Operations__h__
#define __TwkFB__Operations__h__
#include <TwkFB/dll_defs.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkMath/Mat44.h>

namespace TwkFB
{

    //
    //  This is a a function that transforms an array of floats, possibly
    //  in place, and of 1, 2, 3, or 4 channels.
    //
    //  The first argument is the input array
    //  The second argument is the output array
    //  The third argument is the number of (interleaved) channels
    //  The forth argument is the number of pixels in the array
    //  The final argument is blind data passed to the function
    //

    typedef void (*ColorTransformFunc)(const float* input, float* output,
                                       int nchannels, int nelements,
                                       void* data);

    struct TWKFB_EXPORT LogCTransformParams
    {
        float LogCBlackSignal;    // ColorSpace::LogCBlackSignal()
        float LogCEncodingOffset; // ColorSpace::LogCEncodingOffset()
        float LogCEncodingGain;   // ColorSpace::LogCEncodingGain()
        float LogCGraySignal;     // ColorSpace::LogCGraySignal()
        float LogCBlackOffset;    // ColorSpace::LogCBlackOffset()
        float LogCLinearSlope;    // ColorSpace::LogCLinearSlope()
        float LogCLinearOffset;   // ColorSpace::LogCLinearOffset()
        float LogCLinearCutPoint; // ColorSpace::LogCLinearCutPoint();  Logc to
                                  // Linear cutoff
        float LogCCutPoint; // ColorSpace::LogCCutPoint(); Linear to LogC cutoff
        bool* chmap;
    };

    //
    //  Gets the LogC curve parameters from the framebuffer attr values.
    //  NB: The exposure index 'ei' can be optionally passed in to get
    //  logC curve values for a particular exposure index value instead
    //  of getting them from the framebuffer attrs.
    //  Also if ei=0.0 and there are no framebuffer LogC attrs, then
    //  the LogC curve parameter values for ei=800 are returned.
    //
    TWKFB_EXPORT void getLogCCurveParams(LogCTransformParams& params,
                                         const FrameBuffer* fb,
                                         float ei = 0.0f);

    //
    //  Some ColorTransformFuncs
    //

    TWKFB_EXPORT void logLinearTransform(const float*, float*, int, int,
                                         void*); // void* => bool[nchannels]
    TWKFB_EXPORT void linearLogTransform(const float*, float*, int, int,
                                         void*); // void* => bool[nchannels]
    TWKFB_EXPORT void
    logCLinearTransform(const float*, float*, int, int,
                        void*); // void* => LogCTransformParams
    TWKFB_EXPORT void
    linearLogCTransform(const float*, float*, int, int,
                        void*); // void* => LogCTransformParams
    TWKFB_EXPORT void redLogLinearTransform(const float*, float*, int, int,
                                            void*); // void* => bool[nchannels]
    TWKFB_EXPORT void linearRedLogTransform(const float*, float*, int, int,
                                            void*); // void* => bool[nchannels]
    TWKFB_EXPORT void linearColorTransform(const float*, float*, int, int,
                                           void*); // void* => Mat44f*
    TWKFB_EXPORT void yryby2rgbColorTransform(const float*, float*, int, int,
                                              void*); // void* => Vec3f* (Yw)
    TWKFB_EXPORT void rgb2yrybyColorTransform(const float*, float*, int, int,
                                              void*);
    TWKFB_EXPORT void premultTransform(const float*, float*, int, int, void*);
    TWKFB_EXPORT void unpremultTransform(const float*, float*, int, int, void*);
    TWKFB_EXPORT void floatChromaToIntegralTransform(const float*, float*, int,
                                                     int, void*);
    TWKFB_EXPORT void sRGBtoLinearTransform(const float*, float*, int, int,
                                            void*);
    TWKFB_EXPORT void linearToSRGBTransform(const float*, float*, int, int,
                                            void*);
    TWKFB_EXPORT void Rec709toLinearTransform(const float*, float*, int, int,
                                              void*);
    TWKFB_EXPORT void linearToRec709Transform(const float*, float*, int, int,
                                              void*);
    TWKFB_EXPORT void channelLUTTransform(const float*, float*, int, int,
                                          void*); // void* => LUT FB*
    TWKFB_EXPORT void pixel3DLUTTransform(const float*, float*, int, int,
                                          void*); // void* => LUT FB*
    TWKFB_EXPORT void luminanceLUTTransform(const float*, float*, int, int,
                                            void*); // void* => LUT FB*
    TWKFB_EXPORT void gammaTransform(const float*, float*, int, int,
                                     void*); // void* => float[3]
    TWKFB_EXPORT void powerTransform(const float*, float*, int, int,
                                     void*); // void* => float[3]

    //
    //  This function is called when resampling via the transfer function
    //  The input is a vec4 pixel, the output is a vec4 pixel. The
    //  TransferFunc will be called at least once for each input pixel in
    //  the input image and at least one for each output pixel in the
    //  output image.
    //

    typedef void (*TransferFunc)(const float* input, float* output, void* data);

    //
    //  Some transfer functions
    //

    TWKFB_EXPORT void scaledTransfer(const float*, float*,
                                     void*); // void* => float
    TWKFB_EXPORT void minTransfer(const float*, float*, void*); // void* => NULL
    TWKFB_EXPORT void maxTransfer(const float*, float*, void*); // void* => NULL

    //
    //
    //

    typedef std::vector<FrameBuffer*> FrameBufferVector;

    //
    //  Types may differ. Geometry must be identical.
    //  Copies from onto to with possible conversion to to's datatype
    //

    TWKFB_EXPORT void copy(const FrameBuffer* from, FrameBuffer* to);

    //
    //  Same as above, but only copies the first plane of a planar image
    //

    TWKFB_EXPORT void copyPlane(const FrameBuffer* from, FrameBuffer* to);

    //
    //  Copies image, converts it to the target data type.  The first
    //  version only does the passed in image plane. The second will copy
    //  convert all planes to the same data type. The color space should
    //  be REC_709 for data type conversion. However, if a
    //  PACKED_Cb8_Y8_Cr8_Y8 image is passed in, it will be converted to 3
    //  channel RGB REC_709 for output (this format is considered to
    //  actually be REC_709 RGB by Apple so we'll treat it similarily
    //  here). See their docs for a rationalization.
    //

    TWKFB_EXPORT FrameBuffer* copyConvertPlane(const FrameBuffer* from,
                                               FrameBuffer::DataType);

    TWKFB_EXPORT FrameBuffer* copyConvert(const FrameBuffer* from,
                                          FrameBuffer::DataType);

    //
    //  Copy convert Y-RY-BY (float) to Y-U-V (integral) The DataType
    //  argument should be one of the integral types.
    //

    TWKFB_EXPORT FrameBuffer* copyConvertYRYBYtoYUV(const FrameBuffer* from,
                                                    FrameBuffer::DataType);

    //
    //  Apply the function func to the pixels in from buffer and deposit
    //  in the to buffer. Pass in the same pointer for from and to for
    //  in-place operation.
    //

    TWKFB_EXPORT void applyTransform(const FrameBuffer* from, FrameBuffer* to,
                                     ColorTransformFunc func, void* data);

    //
    //  Color space adjustments
    //

    TWKFB_EXPORT void convertLogToLinear(const FrameBuffer* from,
                                         FrameBuffer* to);
    TWKFB_EXPORT void convertLinearToLog(const FrameBuffer* from,
                                         FrameBuffer* to);
    TWKFB_EXPORT void convertLogCToLinear(const FrameBuffer* from,
                                          FrameBuffer* to, float ei);
    TWKFB_EXPORT void convertLinearToLogC(const FrameBuffer* from,
                                          FrameBuffer* to, float ei);
    TWKFB_EXPORT void convertRedLogToLinear(const FrameBuffer* from,
                                            FrameBuffer* to);
    TWKFB_EXPORT void convertLinearToRedLog(const FrameBuffer* from,
                                            FrameBuffer* to);
    TWKFB_EXPORT void convertSRGBToLinear(const FrameBuffer* from,
                                          FrameBuffer* to);
    TWKFB_EXPORT void convertLinearToSRGB(const FrameBuffer* from,
                                          FrameBuffer* to);
    TWKFB_EXPORT void convertLinearToRec709(const FrameBuffer* from,
                                            FrameBuffer* to);
    TWKFB_EXPORT void convertRGBtoYRYBY(const FrameBuffer* from,
                                        FrameBuffer* to);
    TWKFB_EXPORT void premult(const FrameBuffer* from, FrameBuffer* to);
    TWKFB_EXPORT void unpremult(const FrameBuffer* from, FrameBuffer* to);

    TWKFB_EXPORT void linearizeFromGamma(const FrameBuffer* from,
                                         FrameBuffer* to, float fromGamma);
    TWKFB_EXPORT void applyGamma(const FrameBuffer* from, FrameBuffer* to,
                                 float gamma);
    TWKFB_EXPORT void applyCurve(const FrameBuffer* from, FrameBuffer* to,
                                 float r[256], float g[256], float b[256]);

    //
    //  There are better quality versions of the these functions in
    //  FBAux. These are here for code that cannot link against FBAux for
    //  some reason.
    //

    TWKFB_EXPORT void resample(const FrameBuffer* from, FrameBuffer* to);

    TWKFB_EXPORT void normalize(FrameBuffer*, bool discardmax, bool invert);

    TWKFB_EXPORT void nearestNeighborResize(const FrameBuffer* from,
                                            FrameBuffer* to);

    TWKFB_EXPORT FrameBuffer*
    channelMap(const FrameBuffer* from,
               const std::vector<std::string>& newMapping);

    TWKFB_EXPORT FrameBuffer*
    channelMapToPlanar(const FrameBuffer* from,
                       std::vector<std::string> newMapping);

    TWKFB_EXPORT void minMax(const FrameBuffer* fb,
                             std::vector<float>& minValues,
                             std::vector<float>& maxValues);

    //

    TWKFB_EXPORT void transfer(const FrameBuffer* from, FrameBuffer* to,
                               TransferFunc, void*);

    //
    // These are in place.
    //

    TWKFB_EXPORT void flip(FrameBuffer* flipMe);
    TWKFB_EXPORT void flop(FrameBuffer* flopMe);

    //
    //  Crop does not set the uncrop region. It also doesn't work with
    //  images that have depth > 1.
    //

    TWKFB_EXPORT FrameBuffer* cropPlane(const FrameBuffer* fb, int x0, int y0,
                                        int x1, int y1);
    TWKFB_EXPORT FrameBuffer* crop(const FrameBuffer* fb, int x0, int y0,
                                   int x1, int y1);

    //
    //  Crops in infb using a rectangle starting at x0, y0, with the width
    //  of each plane in outfb. The two fbs must have the same types for
    //  each plane currently.
    //

    TWKFB_EXPORT void cropInto(const FrameBuffer* infb, FrameBuffer* outfb,
                               int x0, int y0, int x1, int y1);
    //
    //  like crop, but also sets uncrop region
    //

    TWKFB_EXPORT FrameBuffer* cropWithUncrop(const FrameBuffer* fb, int x0,
                                             int y0, int x1, int y1);

    //
    //  Rip an fb into individual image planes (each a single
    //  channel fb)
    //

    TWKFB_EXPORT FrameBufferVector split(const FrameBuffer* fb);

    //
    //  Merge planes into a single image. Each image should be single channel
    //  and all should have the same DataType
    //

    TWKFB_EXPORT FrameBuffer* merge(const FrameBufferVector& fbs);

    //
    //  Convert planer to packed fb. The output resolution is the same as
    //  the first plane's resolution. The other planes are resampled to
    //  convert up (or down).
    //
    //  Currently nearest neighbor filtering
    //

    TWKFB_EXPORT FrameBuffer* mergePlanes(const FrameBuffer* fb);

    //
    //  Retrieve chromaticities and compute Yw vector
    //  or return 709 version thereof
    //

    TWKFB_EXPORT void yrybyYweights(const FrameBuffer* fb, float& rw, float& gw,
                                    float& bw);

    TWKFB_EXPORT void rec709Matrix(const FrameBuffer* fb, float M[16],
                                   bool adapt = false);
    TWKFB_EXPORT void acesMatrix(const FrameBuffer* fb, float M[16],
                                 bool adapt = false);

    //
    //  General conversion function. This takes an array of floats (the
    //  chromaticies) as white, red, green, blue (8 floats) for the first
    //  two args, the two neutral values (usually the same as whites) and
    //  the last argument indicates whether chromatic adaptation transform
    //  should be applied.
    //

    TWKFB_EXPORT void
    colorSpaceConversionMatrix(const float*, // in chromaticies (8 floats)
                               const float*, // out chromaticies
                               const float*, // in neutal (2 floats)
                               const float*, // out neutal
                               bool,         // adapt
                               float*);      // return 4x4 matrix

    //
    //  Get the RGB<->YUV matrix for various conversion types.
    //

    TWKFB_EXPORT TwkMath::Mat44f RGBtoYUVMatrix(const FrameBuffer* fb);
    TWKFB_EXPORT TwkMath::Mat44f YUVtoRGBMatrix(const FrameBuffer* fb);

    TWKFB_EXPORT void getRGBtoYUVMatrix(TwkMath::Mat44f& m,
                                        const std::string& fb_conversion,
                                        const std::string& fb_range,
                                        unsigned int bits = 8);

    TWKFB_EXPORT void getYUVtoRGBMatrix(TwkMath::Mat44f& m,
                                        const std::string& rb_conversion,
                                        const std::string& fb_range,
                                        unsigned int bits = 8);

    //
    //  From and to can be the same pointer for inplace conversion
    //
    TWKFB_EXPORT void convertRGBtoYUV(const FrameBuffer* from, FrameBuffer* to);
    TWKFB_EXPORT void convertYUVtoRGB(const FrameBuffer* from, FrameBuffer* to);

    //
    //  This function only accepts packed pixel formats (not planar).
    //
    //  Convert YUV         -> RGB
    //  Convert YRYBY       -> RGB
    //  Convert non-709 RGB -> RGB
    //

    TWKFB_EXPORT FrameBuffer* convertToLinearRGB709(const FrameBuffer* fb);

    //
    //  Return a matrix that transforms a pixel coordinate using the image
    //  orientation.
    //

    TWKFB_EXPORT void orientationMatrix(const FrameBuffer* fb, bool normalized,
                                        float* m);

    //
    //  Get a single linear (REC709 primaries) RGBA value from the frame
    //  buffer regardless of framebuffer configuration (planar or not) or
    //  color space.  This function is NOT fast.
    //
    //  Takes into account the orientation of the image data
    //  automatically. In other words, (0,0) is always the lower left
    //  corner of the image (when being viewed).
    //

    TWKFB_EXPORT void linearRGBA709pixelValue(const FrameBuffer* fb, int x,
                                              int y, float* result);

} // namespace TwkFB

#endif // __TwkFB__Operations__h__
