//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

// #ifdef _MSC_VER
// #include <cv.h>
// #else
// #include <opencv/cv.h>
// #endif

// #include <cv.h>

#include <TwkFBAux/FBAux.h>
#include <TwkFB/Operations.h>
#include <TwkExc/Exception.h>
#include <iostream>
#include <TwkUtil/Timer.h>
#include <TwkFB/FrameBuffer.h>

namespace TwkFBAux
{
    using namespace std;
#if 0
struct OpenCVErrorHandler
{
    static int error(int status,
                     const char* func_name,
                     const char* err_msg,
                     const char* file_name,
                     int line,
                     void* data)
    {
        cerr << "ERROR: " << err_msg
             << " (" << func_name << ")"
             << endl;

        TWK_THROW_EXC_STREAM(err_msg
                             << ", " << file_name
                             << ", " << func_name
                             << ", line " << line
                             );

        return 0;
    }

    OpenCVErrorHandler()
    {
        cvRedirectError(error, 0, 0);
    }
};

static OpenCVErrorHandler ehandler;

//
//  Note: The FB class only works with 1 plane.
//

class FB
{
public:
    FB(const FrameBuffer* fbin) : in(0), fb(0), output(false)
    {
        in = const_cast<FrameBuffer*>(fbin);
        
        switch (fbin->dataType())
        {
          case FrameBuffer::DOUBLE:
          case FrameBuffer::HALF:
              fb = copyConvertPlane(in, FrameBuffer::FLOAT);
              break;
          default:
              fb = const_cast<FrameBuffer*>(in);
              break;
        }
    }

    FB(const FrameBuffer* fbin, FrameBuffer::DataType newType) 
        : in(0), fb(0), output(false)
    {
        in = const_cast<FrameBuffer*>(fbin);
        
        if (in->dataType() != newType)
        {
            if (newType == FrameBuffer::HALF)
            {
                fb = copyConvertPlane(in, FrameBuffer::FLOAT);
            }
            else
            {
                fb = copyConvertPlane(in, newType);
            }
        }
        else
        {
            switch (fbin->dataType())
            {
              case FrameBuffer::DOUBLE:
              case FrameBuffer::HALF:
                  fb = copyConvertPlane(in, FrameBuffer::FLOAT);
                  break;
              default:
                  fb = const_cast<FrameBuffer*>(in);
                  break;
            }
        }
    }

    FB(FrameBuffer* fbin) : in(fbin), fb(0), output(true)
    {
        switch (fbin->dataType())
        {
          case FrameBuffer::DOUBLE:
          case FrameBuffer::HALF:
              fb = copyConvertPlane(in, FrameBuffer::FLOAT);
              break;
          default:
              fb = const_cast<FrameBuffer*>(in);
              break;
        }
    }

    FB(FrameBuffer* fbin, FrameBuffer::DataType newType) 
        : in(fbin), fb(0), output(true)
    {
        if (fbin->dataType() != newType)
        {
            if (newType == FrameBuffer::HALF)
            {
                fb = copyConvertPlane(fbin, FrameBuffer::FLOAT);
            }
            else
            {
                fb = copyConvertPlane(fbin, newType);
            }
        }
        else
        {
            switch (fbin->dataType())
            {
              case FrameBuffer::DOUBLE:
              case FrameBuffer::HALF:
                  fb = copyConvertPlane(in, FrameBuffer::FLOAT);
                  break;
              default:
                  fb = const_cast<FrameBuffer*>(in);
                  break;
            }
        }
    }

    ~FB() 
    { 
        if (fb != in) 
        {
            if (output) copyPlane(fb, in);
            delete fb; 
        }
    }

    int cvType()
    {
        int n = fb->numChannels();
        switch (fb->dataType())
        {
          case FrameBuffer::UCHAR:  return CV_8UC(n);
          case FrameBuffer::USHORT: return CV_16UC(n);
          case FrameBuffer::FLOAT:  return CV_32FC(n);
          default: break;
        }

        return -1;
    }

    CvMat operator() ()
    {
        CvMat m;
        cvInitMatHeader(&m,
                        fb->height(), 
                        fb->width(),
                        cvType(),
                        fb->pixels<unsigned char>(),
                        fb->scanlinePaddedSize());

        return m;
    }
    
    FrameBuffer* in;
    FrameBuffer* fb;
    bool output;
};

void
gaussBlur(const FrameBuffer* src, FrameBuffer* dst, int w, int h)
{
    if (src->nextPlane() && dst->nextPlane())
    {
        gaussBlur(src->nextPlane(), dst->nextPlane(), w, h);
    }
    
    //
    //  Other blur types could be used here
    //
    
    FB in(src, dst->dataType());
    FB out(dst);
    
    CvMat a = in();
    CvMat b = out();
    cvSmooth((const CvArr*)&a, (CvArr*)&b, CV_GAUSSIAN, w, h, 0);
}

void
add(const FrameBuffer* fb0, const FrameBuffer* fb1, FrameBuffer* fbout)
{
    if (fb0->nextPlane() && fb0->nextPlane() && fbout->nextPlane())
    {
        add(fb0->nextPlane(), fb1->nextPlane(), fbout->nextPlane());
    }
    
    FB in0(fb0, fbout->dataType());
    FB in1(fb1, fbout->dataType());
    FB out(fbout);
    
    CvMat a = in0();
    CvMat b = in1();
    CvMat c = out();
    
    cvAdd((const CvArr*)&a, (const CvArr*)&b, &c);
}

void
subtract(const FrameBuffer* fb0, const FrameBuffer* fb1, FrameBuffer* fbout)
{
    if (fb0->nextPlane() && fb0->nextPlane() && fbout->nextPlane())
    {
        subtract(fb0->nextPlane(), fb1->nextPlane(), fbout->nextPlane());
    }
    
    FB in0(fb0, fbout->dataType());
    FB in1(fb1, fbout->dataType());
    FB out(fbout);
    
    CvMat a = in0();
    CvMat b = in1();
    CvMat c = out();
    
    cvAdd((const CvArr*)&a, (const CvArr*)&b, &c);
}

void
multiply(const FrameBuffer* fb0,
         const FrameBuffer* fb1,
         FrameBuffer* fbout,
         float scale)
{
    if (fb0->nextPlane() && fb0->nextPlane() && fbout->nextPlane())
    {
        multiply(fb0->nextPlane(), fb1->nextPlane(), fbout->nextPlane(), scale);
    }
    
    FB in0(fb0, fbout->dataType());
    FB in1(fb1, fbout->dataType());
    FB out(fbout);
    
    CvMat a = in0();
    CvMat b = in1();
    CvMat c = out();
    
    cvMul((const CvArr*)&a, (const CvArr*)&b, &c, scale);
}

void
convertScaleShift(const FrameBuffer* fb0,
                  FrameBuffer* fbout,
                  float scale,
                  float shift)
{
    if (fb0->nextPlane() && fbout->nextPlane())
    {
        convertScaleShift(fb0->nextPlane(),
                          fbout->nextPlane(),
                          scale,
                          shift);
    }
    
    FB in0(fb0, fbout->dataType());
    FB out(fbout);
    
    CvMat a = in0();
    CvMat c = out();
    
    cvConvertScale((const CvArr*)&a, &c, scale, shift);
}

void
divide(const FrameBuffer* fb0,
       const FrameBuffer* fb1,
       FrameBuffer* fbout,
       float scale)
{
    if (fb0->nextPlane() && fb1->nextPlane() && fbout->nextPlane())
    {
        divide(fb0->nextPlane(),
               fb1->nextPlane(),
               fbout->nextPlane(),
               scale);
    }
    
    FB in0(fb0,fbout->dataType());
    FB in1(fb1,fbout->dataType());
    FB out(fbout);
    
    CvMat a = in0();
    CvMat b = in1();
    CvMat c = out();
    cvDiv((const CvArr*)&a, (const CvArr*)&b, &c, scale);
}

void
pow(const FrameBuffer* fb0,
    FrameBuffer* fbout,
    double power)
{
    if (fb0->nextPlane() && fbout->nextPlane())
    {
        pow(fb0->nextPlane(), fbout->nextPlane(), power);
    }
    
    FB in(fb0, fbout->dataType());
    FB out(fbout);
    
    CvMat a = in();
    CvMat c = out();
    cvPow((const CvArr*)&a, (CvArr*)&c, power);
}

void
threshold(const FrameBuffer* in,
          FrameBuffer* out,
          double value,
          double maxValue,
          ThresholdType ttype)
{
    int type = 0;
    
    if (in->nextPlane() && out->nextPlane())
    {
        threshold(in->nextPlane(),
                  out->nextPlane(),
                  value, maxValue, ttype);
    }
    
    switch (ttype)
    {
        case ThresholdBinary:         type = CV_THRESH_BINARY; break;
        case ThresholdBinaryInverse:  type = CV_THRESH_BINARY_INV; break;
        case ThresholdTruncate:       type = CV_THRESH_TRUNC; break;
        case ThresholdToZero:         type = CV_THRESH_TOZERO; break;
        case ThresholdToZeroInverse:  type = CV_THRESH_TOZERO_INV; break;
    }
    
    FB fba(in, out->dataType());
    FB fbb(out);
    
    CvMat a = fba();
    CvMat b = fbb();
    cvThreshold((const CvArr*)&a, &b, value, maxValue, type);
}

MinMaxLocation 
minMax(const FrameBuffer* in)
{
    FB fb(in);
    
    CvMat a = fb();
    MinMaxLocation value;
    
    cvMinMaxLoc((const CvArr*)&a, &value.minValue, &value.maxValue,
                (CvPoint*)&value.minPixel, (CvPoint*)&value.maxPixel,
                0);
    
    return value;
}
    
void
resize(const FrameBuffer* src, FrameBuffer* dst)
{
    TwkUtil::Timer time1;
    
    time1.start();
    //
    //  This should do both a resize and a conversion to be compatible
    //  with the FB resize function.
    //
    
    if (src->nextPlane() && dst->nextPlane())
    {
        resize(src->nextPlane(),
               dst->nextPlane());
    }
    
    if (dst->width() == 0 || dst->height() == 0 ||
        src->width() == 0 || src->height() == 0)
    {
        return;
    }
    
    if (src->width() == dst->width() && src->height() == dst->height())
    {
        copyPlane(src, dst);
        return;
    }
    
    int i;
    Interpolation interp = AreaInterpolation;
    switch (interp)
    {
            //case NearestNeighborInterpolation:    i = CV_INTER_NN; break;
        case LinearInterpolation:             i = CV_INTER_LINEAR; break;
        case AreaInterpolation:
        {
            if (dst->width() > src->width() || dst->height() > src->height())
            {
                i = CV_INTER_CUBIC;
            }
            else
            {
                // only for minification
                i = CV_INTER_AREA;
            }
            break;
        }
    }
    
    FrameBuffer *origDst = 0;
    if ((i == CV_INTER_AREA && dst->dataType() == TwkFB::FrameBuffer::USHORT) ||
        dst->dataType() == TwkFB::FrameBuffer::PACKED_R10_G10_B10_X2 ||
        dst->dataType() == TwkFB::FrameBuffer::PACKED_X2_B10_G10_R10 ||
        dst->dataType() == TwkFB::FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8 ||
        dst->dataType() == TwkFB::FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8 )
        //
        //  Convert fbs to FLOAT because opencv has bug when resizing shorts
        //  Also opencv cannot operate on packed data.
        //
    {
        origDst = dst;
        dst = copyConvertPlane (dst, TwkFB::FrameBuffer::FLOAT);
    }
    
    FB in(src, dst->dataType());
    FB out(dst);
    
    CvMat a = in();
    CvMat b = out();
    
    cvResize((const CvArr*)&a, (CvArr*)&b, i);
    
    if (origDst)
    {
        copyPlane (dst, origDst);
        delete dst;
        dst = origDst;
    }
    
    if (src->uncrop())
    {
        const double xfactor = double(dst->width()) / double(src->width());
        const double yfactor = double(dst->height()) / double(src->height());
        
        dst->setUncrop(int(double(src->uncropWidth()) * xfactor),
                       int(double(src->uncropHeight()) * yfactor),
                       int(double(src->uncropX()) * xfactor),
                       int(double(src->uncropY()) * yfactor));
    }
    
    dst->setPixelAspectRatio(src->pixelAspectRatio());
    time1.stop();
    fprintf(stderr, "time %f \n", time1.elapsed());
}

#endif

    //
    // used both for up and down scaling
    // assume scale is same vertical and horizontal
    //
    // evn if the src has scanline padding, the dst will NOT have padding.
    //
    template <typename T>
    void resizeLinear(const FrameBuffer* src, FrameBuffer* dst)
    {
        float scalew = (float)src->width() / dst->width();   // < 1
        float scaleh = (float)src->height() / dst->height(); // < 1

        size_t lineWidth = src->scanlinePaddedSize() / sizeof(T);
        size_t dstLineWidth = dst->scanlinePaddedSize() / sizeof(T);
        size_t dstSize = dstLineWidth * dst->height();
        size_t numChannels = dst->numChannels();
        size_t srcWidth = src->width();
        size_t srcHeight = src->height();
        size_t dstWidth = dst->width();
        size_t dstHeight = dst->height();

        T* dstData = dst->pixels<T>();
        const T* srcData = src->pixels<T>();

        for (size_t i = 0; i < dstHeight; ++i)
        {
            for (size_t j = 0; j < dstWidth; ++j)
            {
                float yCenter = i * scaleh;
                float xCenter = j * scalew;
                size_t xStartInt = floor(xCenter);
                size_t xEndInt = min(srcWidth - 1, xStartInt + 1);
                size_t yStartInt = floor(yCenter);
                size_t yEndInt = min(srcHeight - 1, yStartInt + 1);

                float xW = xCenter - xStartInt;
                float yW = yCenter - yStartInt;

                const T* startDownLeft =
                    srcData + yStartInt * lineWidth + xStartInt * numChannels;
                const T* startUpLeft =
                    srcData + yEndInt * lineWidth + xStartInt * numChannels;
                const T* startDownRight =
                    srcData + yStartInt * lineWidth + xEndInt * numChannels;
                const T* startUpRight =
                    srcData + yEndInt * lineWidth + xEndInt * numChannels;
                T* dstLoc = dstData + i * dstLineWidth + j * numChannels;

                for (size_t n = 0; n < numChannels; ++n)
                {
                    float down =
                        startDownLeft[n] * (1 - xW) + startDownRight[n] * xW;
                    float up = startUpLeft[n] * (1 - xW) + startUpRight[n] * xW;
                    dstLoc[n] = down * (1 - yW) + up * yW;
                }
            }
        }
        return;
    }

    template <typename T>
    void resizeArea(const FrameBuffer* src, FrameBuffer* dst)
    {

        float scalew = (float)src->width() / dst->width();   // > 1
        float scaleh = (float)src->height() / dst->height(); // > 1

        size_t lineWidth = src->scanlinePaddedSize() / sizeof(T);
        size_t dstLineWidth = dst->scanlinePaddedSize() / sizeof(T);
        size_t dstSize = dstLineWidth * dst->height();
        size_t numChannels = dst->numChannels();
        size_t srcWidth = src->width();
        size_t srcHeight = src->height();
        size_t dstWidth = dst->width();
        size_t dstHeight = dst->height();

        T* dstData = (T*)dst->pixels<T>();
        const T* srcData = (T*)src->pixels<T>();
        memset(dstData, 0, dstHeight * dst->scanlinePaddedSize());
        vector<float> t(numChannels);

        for (size_t i = 0; i < dstHeight; ++i)
        {
            for (size_t j = 0; j < dstWidth; ++j)
            {
                float yStart = i * scaleh;
                float yEnd = yStart + scaleh;
                float xStart = j * scalew;
                float xEnd = xStart + scalew;
                size_t xStartInt = floor(xStart);
                size_t xEndInt = ceil(xEnd - 1.0);
                size_t yStartInt = floor(yStart);
                size_t yEndInt = ceil(yEnd - 1.0);

                const T* start =
                    srcData + yStartInt * lineWidth + xStartInt * numChannels;
                float w = 0;
                T* dstLoc = dstData + i * dstLineWidth + j * numChannels;
                for (size_t n = 0; n < numChannels; ++n)
                    t[n] = 0.0;
                for (size_t k = yStartInt; k <= yEndInt; ++k)
                {
                    for (size_t m = xStartInt; m <= xEndInt; ++m)
                    {
                        if (k > srcHeight - 1 || m > srcWidth - 1)
                            continue;
                        float weight = 1.0;
                        if (m == xStartInt)
                            weight *= m + 1.0 - xStart;
                        else if (m == xEndInt)
                            weight *= xEnd - m;
                        if (k == yStartInt)
                            weight *= k + 1.0 - yStart;
                        else if (k == yEndInt)
                            weight *= yEnd - k;
                        w += weight;
                        const T* d = start + (k - yStartInt) * lineWidth
                                     + (m - xStartInt) * numChannels;
                        for (size_t n = 0; n < numChannels; ++n)
                        {
                            t[n] += weight * d[n];
                        }
                    }
                }
                for (size_t n = 0; n < numChannels; ++n)
                {
                    dstLoc[n] = t[n] / w;
                }
            }
        }
        return;
    }

    template <typename T>
    void resizeFrameBuffer(const FrameBuffer* src, FrameBuffer* dst)
    {
        if (src->width() <= dst->width() || src->height() <= dst->height())
            resizeLinear<T>(src, dst);
        else
            resizeArea<T>(src, dst);
    }

    void resize(const FrameBuffer* src, FrameBuffer* dst)
    {
        //    TwkUtil::Timer time1;
        //    time1.start();
        // assert(src->dataType() == dst->dataType());
        FrameBuffer* tempFB = 0;

        if (src->dataType() != dst->dataType())
        {
            tempFB = copyConvert(src, dst->dataType());
            src = tempFB;
        }

        if (src->nextPlane() && dst->nextPlane())
        {
            resize(src->nextPlane(), dst->nextPlane());
        }

        if (dst->width() == 0 || dst->height() == 0 || src->width() == 0
            || src->height() == 0)
        {
            return;
        }

        if (src->width() == dst->width() && src->height() == dst->height())
        {
            copyPlane(src, dst);
            return;
        }

        bool needsConvert = false;
        //
        //  Convert fbs to FLOAT for now because this resize function cannot
        //  operate on packed data.
        //
        if (src->dataType() == TwkFB::FrameBuffer::PACKED_R10_G10_B10_X2
            || src->dataType() == TwkFB::FrameBuffer::PACKED_X2_B10_G10_R10
            || src->dataType() == TwkFB::FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8
            || src->dataType() == TwkFB::FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8
            || src->dataType() == TwkFB::FrameBuffer::HALF
            || src->dataType() == TwkFB::FrameBuffer::DOUBLE)
        {
            needsConvert = true;
        }
        const FrameBuffer* convertedSrc =
            needsConvert ? copyConvertPlane(src, TwkFB::FrameBuffer::FLOAT)
                         : src;
        FrameBuffer* convertedDst =
            needsConvert ? copyConvertPlane(dst, TwkFB::FrameBuffer::FLOAT)
                         : dst;

        switch (convertedSrc->dataType())
        {
        default:
        case FrameBuffer::UCHAR:
            resizeFrameBuffer<unsigned char>(convertedSrc, convertedDst);
            break;
        case FrameBuffer::USHORT:
            resizeFrameBuffer<unsigned short>(convertedSrc, convertedDst);
            break;
        case FrameBuffer::UINT:
            resizeFrameBuffer<unsigned int>(convertedSrc, convertedDst);
            break;
        case FrameBuffer::FLOAT:
            resizeFrameBuffer<float>(convertedSrc, convertedDst);
            break;
        }

        if (needsConvert)
        {
            copyPlane(convertedDst, dst);
            delete convertedSrc;
            delete convertedDst;
        }

        if (src->uncrop())
        {
            const double xfactor = double(dst->width()) / double(src->width());
            const double yfactor =
                double(dst->height()) / double(src->height());

            dst->setUncrop(int(double(src->uncropWidth()) * xfactor),
                           int(double(src->uncropHeight()) * yfactor),
                           int(double(src->uncropX()) * xfactor),
                           int(double(src->uncropY()) * yfactor));
        }

        dst->setPixelAspectRatio(src->pixelAspectRatio());
        //    time1.stop();
        //    fprintf(stderr, "time %f \n", time1.elapsed());

        if (tempFB)
            delete tempFB;
    }

} // namespace TwkFBAux
