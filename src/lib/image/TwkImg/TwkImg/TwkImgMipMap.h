//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkImgMipMap_h_
#define _TwkImgMipMap_h_

#include <TwkImg/TwkImgImage.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Color.h>
#include <assert.h>

namespace TwkImg
{

    //******************************************************************************
    // Determine the numbers of LODs
    // The maximum number of LODs is 16,
    // which corresponds to an lod0 image of size 65535 x 65535
    // Inconcievably huge, almost certainly larger
    // than we could ever plan on using.
    namespace
    {
        static const int MAX_NUM_LODS = 16;
    } // End unnamed namespace

    //******************************************************************************
    // Mip-maps are normally square, and restricted
    // to having power of two sizes. However, such
    // a restriction causes a problem. Most samples
    // into a mip-map will have a circular sample
    // footprint. If the original image was not
    // square, this circular sample will get scaled
    // into an ellipse, which means that for the most
    // common case we're causing multiple samples
    // to be taken for correct sampling, which is
    // foolish. This mip-map maintains the aspect
    // ratio of the original image used to create it,
    // and the lod0 image is a direct copy of the
    // original image.
    // There is a problem, however. When we halve
    // an image with odd dimensions, the new dimensions
    // are computed like this:
    // if ( isOdd( srcDim ) ) dstDim = ( srcDim + 1 ) / 2
    // else dstDim = srcDim / 2;
    // The problem is that the dstImg will be 0.5 units
    // too large. If you resample this image with
    // a resampling kernel that incorporates black into
    // the sample when the kernel leaves the boundary
    // of the image, then our resampled images will falsely
    // overcover the kernel. To correctly
    // sample this image, you should use a kernel that
    // only incorporates valid samples into its coverage sum
    // and then compute the coverage of the full kernel
    // in the lod0 image and darken your sample accordingly.
    template <class COLOR> class MipMap
    {
    public:
        typedef COLOR ColorType;

        MipMap(const TwkImg::Image<COLOR>& img, int maxLods = MAX_NUM_LODS,
               float workGamma = 1.0f);
        ~MipMap();

        // Basic data access.
        // The number of levels of detail
        int numLods() const;

        // Access to the level of detail images directly.
        const Image<COLOR>* lod(int d) const;
        Image<COLOR>* lod(int d);

        // Each of our lods has an origin of zero,
        // so the origin of the original image is stored
        // here.
        const TwkMath::Vec2i& origin() const;

        // Convenience functions for computing sample coverage
        // Uses bilinear, which is absolutely sufficient for
        // coverage computations in all cases, since we're
        // always computing coverage only at the boundary of
        // a rectangle.

    protected:
        // These special case handlers
        // are used by the constructor when
        // small sources are encountered.
        void specialCaseSquare(Image<COLOR>& srcImg, Image<COLOR>& dstImg);
        void specialCaseRectangle(Image<COLOR>& srcImg, Image<COLOR>& dstImg);

        // The data
        int m_numLods;
        Image<COLOR>* m_lods[MAX_NUM_LODS];
        TwkMath::Vec2i m_origin;
    };

    //******************************************************************************
    // Typedefs
    //******************************************************************************
    typedef MipMap<TwkMath::Col4f> Mip4f;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************

    // Use an unnamed namespace to restrict the scope
    // of these variables to this file.
    namespace
    {

        //******************************************************************************
        // Define filter values.
        // We use a 4x4 gaussian filter (exp(-1.125 (r*r)))
        // rather than the standard 2x2 box filter, because
        // sampling with a box filter introduces high-frequency
        // components into the resampled image that were not
        // present in the original image. Subsequently,
        // the filtered images when used could produce
        // moire patterns or scintillation.
        static const float center4x4 = 0.185622f;
        static const float edge4x4 = 0.029797f;
        static const float corner4x4 = 0.004783f;

        static const float center4x3 = 0.199414f;
        static const float edge4x3 = 0.032011f;
        static const float corner4x3 = 0.005139f;

        static const float center4x2 = 0.371244f;
        static const float edge4x2 = 0.059595f;
        static const float corner4x2 = 0.009567f;

        static const float center4x1 = 0.430838f;
        static const float edge4x1 = 0.069161f;

        static const float center3x3 = 0.214230f;
        static const float edge3x3 = 0.034390f;
        static const float corner3x3 = 0.005520f;

        static const float center3x2 = 0.398828f;
        static const float edge3x2 = 0.064023f;
        static const float corner3x2 = 0.010277f;

        static const float center3x1 = 0.462850f;
        static const float edge3x1 = 0.074299f;

        static const float center2x2 = 0.742488f;
        static const float edge2x2 = 0.119189f;
        static const float corner2x2 = 0.019133f;

        static const float center2x1 = 0.861677f;
        static const float edge2x1 = 0.138322f;

        //******************************************************************************
        static inline bool isOdd(int num) { return (bool)(num & 1); }

        //******************************************************************************
        static inline bool isEven(int num) { return !isOdd(num); }

        //*****************************************************************************
        template <class COLOR> static void gammaImg(Image<COLOR>& img, float g)
        {
            // General case, we can't do anything
            return;
        }

        // Specializations
        //*****************************************************************************
        static void gammaImg(Image<float>& img, float g)
        {
            if (g == 1.0f || g == 0.0f)
            {
                return;
            }

            float expo = 1.0f / g;

            float* pixel = img.pixels();
            float* const endPixel = pixel + img.numPixels();

            for (; pixel != endPixel; ++pixel)
            {
                *pixel = powf(*pixel, expo);
            }
        }

        //*****************************************************************************
        static void gammaImg(Image<double>& img, float g)
        {
            if (g == 1.0f || g == 0.0f)
            {
                return;
            }

            float expo = 1.0f / g;

            double* pixel = img.pixels();
            double* const endPixel = pixel + img.numPixels();

            for (; pixel != endPixel; ++pixel)
            {
                *pixel = pow(*pixel, expo);
            }
        }

        //*****************************************************************************
        static void gammaImg(Image<TwkMath::Vec2f>& img, float g)
        {
            if (g == 1.0f || g == 0.0f)
            {
                return;
            }

            float expo = 1.0f / g;

            TwkMath::Vec2f* pixel = img.pixels();
            TwkMath::Vec2f* const endPixel = pixel + img.numPixels();

            for (; pixel != endPixel; ++pixel)
            {
                for (int i = 0; i < 2; ++i)
                {
                    (*pixel)[i] = powf((*pixel)[i], expo);
                }
            }
        }

        //*****************************************************************************
        static void gammaImg(Image<TwkMath::Vec3f>& img, float g)
        {
            if (g == 1.0f || g == 0.0f)
            {
                return;
            }

            float expo = 1.0f / g;

            TwkMath::Vec3f* pixel = img.pixels();
            TwkMath::Vec3f* const endPixel = pixel + img.numPixels();

            for (; pixel != endPixel; ++pixel)
            {
                for (int i = 0; i < 3; ++i)
                {
                    (*pixel)[i] = powf((*pixel)[i], expo);
                }
            }
        }

        //*****************************************************************************
        static void gammaImg(Image<TwkMath::Vec4f>& img, float g)
        {
            if (g == 1.0f || g == 0.0f)
            {
                return;
            }

            float expo = 1.0f / g;

            TwkMath::Vec4f* pixel = img.pixels();
            TwkMath::Vec4f* const endPixel = pixel + img.numPixels();

            for (; pixel != endPixel; ++pixel)
            {
                for (int i = 0; i < 4; ++i)
                {
                    (*pixel)[i] = powf((*pixel)[i], expo);
                }
            }
        }

        //*****************************************************************************
        static void gammaImg(Image<TwkMath::Vec2d>& img, float g)
        {
            if (g == 1.0f || g == 0.0f)
            {
                return;
            }

            float expo = 1.0f / g;

            TwkMath::Vec2d* pixel = img.pixels();
            TwkMath::Vec2d* const endPixel = pixel + img.numPixels();

            for (; pixel != endPixel; ++pixel)
            {
                for (int i = 0; i < 2; ++i)
                {
                    (*pixel)[i] = pow((*pixel)[i], expo);
                }
            }
        }

        //*****************************************************************************
        static void gammaImg(Image<TwkMath::Vec3d>& img, float g)
        {
            if (g == 1.0f || g == 0.0f)
            {
                return;
            }

            float expo = 1.0f / g;

            TwkMath::Vec3d* pixel = img.pixels();
            TwkMath::Vec3d* const endPixel = pixel + img.numPixels();

            for (; pixel != endPixel; ++pixel)
            {
                for (int i = 0; i < 3; ++i)
                {
                    (*pixel)[i] = pow((*pixel)[i], expo);
                }
            }
        }

        //*****************************************************************************
        static void gammaImg(Image<TwkMath::Vec4d>& img, float g)
        {
            if (g == 1.0f || g == 0.0f)
            {
                return;
            }

            float expo = 1.0f / g;

            TwkMath::Vec4d* pixel = img.pixels();
            TwkMath::Vec4d* const endPixel = pixel + img.numPixels();

            for (; pixel != endPixel; ++pixel)
            {
                for (int i = 0; i < 4; ++i)
                {
                    (*pixel)[i] = pow((*pixel)[i], expo);
                }
            }
        }

    } // End unnamed namespace

    //******************************************************************************
    template <class COLOR>
    MipMap<COLOR>::MipMap(const Image<COLOR>& img, int maxLods, float workGamma)
    {
        if (maxLods > MAX_NUM_LODS)
        {
            maxLods = MAX_NUM_LODS;
        }

        // Build the levels Lod0 is equal to the image given.
        // Except that we need to make the offsets equal to zero.
        m_origin = img.origin();
        m_lods[0] = new Image<COLOR>(img);
        m_lods[0]->setOrigin(TwkMath::Vec2i(0, 0));

        // Transform work image into working gamma space.
        gammaImg(*(m_lods[0]), workGamma);

        int dstWidth = m_lods[0]->width();
        int dstHeight = m_lods[0]->height();
        for (m_numLods = 1;
             (dstWidth > 1 || dstHeight > 1) && m_numLods <= maxLods;
             m_numLods++)
        {
            const int srcWidth = dstWidth;
            const int srcHeight = dstHeight;
            bool wOdd;
            if ((wOdd = isOdd(srcWidth)))
            {
                dstWidth = (srcWidth + 1) / 2;
            }
            else
            {
                dstWidth = srcWidth / 2;
            }

            bool hOdd;
            if ((hOdd = isOdd(srcHeight)))
            {
                dstHeight = (srcHeight + 1) / 2;
            }
            else
            {
                dstHeight = srcHeight / 2;
            }

            const int lod = m_numLods;
            const int lastLod = lod - 1;
            Image<COLOR>* srcImg = m_lods[lastLod];
            Image<COLOR>* dstImg = new Image<COLOR>(dstWidth, dstHeight);
            m_lods[lod] = dstImg;

            // If the src image is very small, some of our iterations
            // below would access out-of-bound pixels. However, we don't
            // want to irreparably slow down the mip-map creation process
            // by checking at every single sample. Instead, we have
            // special handlers for the small src situation.
            if (srcWidth < 4)
            {
                if (srcHeight < 4)
                {
                    specialCaseSquare(*srcImg, *dstImg);
                }
                else
                {
                    specialCaseRectangle(*srcImg, *dstImg);
                }
                continue;
            }
            else if (srcHeight < 4)
            {
                specialCaseRectangle(*srcImg, *dstImg);
                continue;
            }

            // Compute strides to ease stepping through the pixels
            const int srcStride = srcImg->stride();
            const int srcStride2 = srcStride * 2;
            const int dstStride = dstImg->stride();

            // We subtract one to have these rows point
            // to one pixel before the start of the scanline.
            // The filters below respect this arrangement.
            COLOR* srcRow1 = srcImg->pixels() - 1;
            COLOR* srcRow0 = srcRow1 - srcStride;
            COLOR* srcRow2 = srcRow1 + srcStride;
            COLOR* srcRow3 = srcRow2 + srcStride;

            // We compute dstLast to point to the first pixel
            // in the last valid row, rather than dstEnd which
            // would point to the first pixel in the first invalid row.
            COLOR* dstRow = dstImg->pixels();
            COLOR* dstRowLast = dstRow + (dstStride * (dstImg->height() - 1));

            //********************************************************************
            // Do the first row, in which srcRow0 is invalid.

            // Do the start corner of the first row.
            // src0 is invalid
            // index 0 is also invalid.
            COLOR* src0 = srcRow0;
            COLOR* src1 = srcRow1;
            COLOR* src2 = srcRow2;
            COLOR* src3 = srcRow3;
            COLOR* dst = dstRow;
            COLOR* dstLast = dst + (dstStride - 1);
            (*dst) = (src3[3]) * corner3x3
                     + (src1[3] + src2[3] + src3[1] + src3[2]) * edge3x3
                     + (src1[1] + src1[2] + src2[1] + src2[2]) * center3x3;

            // Do the normal pixels of the first row.
            // Increment the pointer in the initializer.
            // We move the src pointers by two because that image
            // is twice as large as we are.
            for (src0 += 2, src1 += 2, src2 += 2, src3 += 2, dst++;
                 dst < dstLast;
                 src0 += 2, src1 += 2, src2 += 2, src3 += 2, dst++)
            {
                // src0 is invalid.
                (*dst) = (src3[0] + src3[3]) * corner4x3
                         + (src1[0] + src1[3] + src2[0] + src2[3] + src3[1]
                            + src3[2])
                               * edge4x3
                         + (src1[1] + src1[2] + src2[1] + src2[2]) * center4x3;
            }

            // Do the end corner of the first row.
            if (wOdd)
            {
                // src0 is invalid
                // indices 2 & 3 are also invalid.
                (*dst) = (src3[0]) * corner3x2
                         + (src1[0] + src2[0] + src3[1]) * edge3x2
                         + (src1[1] + src2[1]) * center3x2;
            }
            else
            {
                // src0 is invalid.
                // index 3 is also invalid
                (*dst) = (src3[0]) * corner3x3
                         + (src1[0] + src2[0] + src3[1] + src3[2]) * edge3x3
                         + (src1[1] + src1[2] + src2[1] + src2[2]) * center3x3;
            }

            //********************************************************************
            // Loop over all the rows except the first,
            // which we already did, and the last, which we'll
            // do later.
            // We increment the rows and in both the initializer and the
            // increment, since we already did one row.
            for (srcRow0 += srcStride2, srcRow1 += srcStride2,
                 srcRow2 += srcStride2, srcRow3 += srcStride2,
                 dstRow += dstStride; // End initializer
                 dstRow < dstRowLast; // Test
                 srcRow0 += srcStride2, srcRow1 += srcStride2,
                 srcRow2 += srcStride2, srcRow3 += srcStride2,
                 dstRow += dstStride) // Increment
            {
                src0 = srcRow0;
                src1 = srcRow1;
                src2 = srcRow2;
                src3 = srcRow3;
                dst = dstRow;
                dstLast = dst + (dstStride - 1);

                // Do the first pixel of the row.
                // Index 0 is invalid.
                (*dst) = (src0[3] + src3[3]) * corner4x3
                         + (src0[1] + src0[2] + src1[3] + src2[3] + src3[1]
                            + src3[2])
                               * edge4x3
                         + (src1[1] + src1[2] + src2[1] + src2[2]) * center4x3;

                // Loop over the pixels in the middle of the row
                for (src0 += 2, src1 += 2, src2 += 2, src3 += 2, dst++;
                     dst < dstLast;
                     src0 += 2, src1 += 2, src2 += 2, src3 += 2, dst++)
                {
                    // In this inner loop, we're always using
                    // the normal kernel
                    // Nothing is invalid!!!
                    (*dst) =
                        (src0[0] + src0[3] + src3[0] + src3[3]) * corner4x4
                        + (src0[1] + src0[2] + src1[0] + src1[3] + src2[0]
                           + src2[3] + src3[1] + src3[2])
                              * edge4x4
                        + (src1[1] + src1[2] + src2[1] + src2[2]) * center4x4;
                }

                if (wOdd)
                {
                    // Do the last pixel of the row.
                    // Indices 2 and 3 are invalid.
                    (*dst) = (src0[0] + src3[0]) * corner4x2
                             + (src0[1] + src1[0] + src2[0] + src3[1]) * edge4x2
                             + (src1[1] + src2[1]) * center4x2;
                }
                else
                {
                    // Do the last pixel of the row.
                    // Index 3 is invalid.
                    (*dst) =
                        (src0[0] + src3[0]) * corner4x3
                        + (src0[1] + src0[2] + src1[0] + src2[0] + src3[1]
                           + src3[2])
                              * edge4x3
                        + (src1[1] + src1[2] + src2[1] + src2[2]) * center4x3;
                }
            }

            //********************************************************************
            // Do the last row.
            src0 = srcRow0;
            src1 = srcRow1;
            src2 = srcRow2;
            src3 = srcRow3;
            dst = dstRow;
            dstLast = dst + (dstStride - 1);
            if (hOdd)
            {
                // Do the last row.
                // src2 and src3 are invalid.

                // Do the start corner of the first row.
                // index 0 is also invalid.
                (*dst) = (src0[3]) * corner3x2
                         + (src0[1] + src0[2] + src1[3]) * edge3x2
                         + (src1[1] + src1[2]) * center3x2;

                // Do the normal pixels of the last row.
                for (src0 += 2, src1 += 2, src2 += 2, src3 += 2, dst++;
                     dst < dstLast;
                     src0 += 2, src1 += 2, src2 += 2, src3 += 2, dst++)
                {
                    // src2 and src3 are invalid.
                    (*dst) = (src0[0] + src0[3]) * corner4x2
                             + (src0[1] + src0[2] + src1[0] + src1[3]) * edge4x2
                             + (src1[1] + src1[2]) * center4x2;
                }

                // Do the end corner of the last row.
                if (wOdd)
                {
                    // src2 and src3 are invalid
                    // indices 2 & 3 are also invalid.
                    (*dst) = (src0[0]) * corner2x2
                             + (src0[1] + src1[0]) * edge2x2
                             + (src1[1]) * center2x2;
                }
                else
                {
                    // src2 and src3 are invalid.
                    // index 3 is also invalid
                    (*dst) = (src0[0]) * corner3x2
                             + (src0[1] + src0[2] + src1[0]) * edge3x2
                             + (src1[1] + src1[2]) * center3x2;
                }
            }
            else
            {
                // Do the last row.
                // src3 is invalid.

                // Do the start corner of the first row.
                // index 0 is also invalid.
                (*dst) = (src0[3]) * corner4x3
                         + (src0[1] + src0[2] + src1[3] + src2[3]) * edge3x3
                         + (src1[1] + src1[2] + src2[1] + src2[2]) * center3x3;

                // Do the normal pixels of the last row.
                for (src0 += 2, src1 += 2, src2 += 2, src3 += 2, dst++;
                     dst < dstLast;
                     src0 += 2, src1 += 2, src2 += 2, src3 += 2, dst++)
                {
                    // src3 is invalid.
                    (*dst) =
                        (src0[0] + src0[3]) * corner4x3
                        + (src0[1] + src0[2] + src1[0] + src1[3] + src2[0]
                           + src2[3])
                              * edge4x3
                        + (src1[1] + src1[2] + src2[1] + src2[2]) * center4x3;
                }

                // Do the end corner of the last row.
                if (wOdd)
                {
                    // src3 is invalid
                    // indices 2 & 3 are also invalid.
                    (*dst) = (src0[0]) * corner3x2
                             + (src0[1] + src1[0] + src2[0]) * edge3x2
                             + (src1[1] + src2[1]) * center3x2;
                }
                else
                {
                    // src3 is invalid.
                    // index 3 is also invalid
                    (*dst) =
                        (src0[0]) * corner3x3
                        + (src0[1] + src0[2] + src1[0] + src2[0]) * edge3x3
                        + (src1[1] + src1[2] + src2[1] + src2[2]) * center3x3;
                }
            }
        } // End of loop over lods.

        // Loop over LODS again, this time un-gammaing the images
        for (int i = 0; i < m_numLods; ++i)
        {
            gammaImg(*(m_lods[i]), 1.0f / workGamma);
        }
    }

    //******************************************************************************
    template <class COLOR> inline MipMap<COLOR>::~MipMap()
    {
        for (int i = 0; i < m_numLods; i++)
        {
            delete m_lods[i];
        }
    }

    //******************************************************************************
    template <class COLOR> inline int MipMap<COLOR>::numLods() const
    {
        return m_numLods;
    }

    //******************************************************************************
    template <class COLOR>
    inline const Image<COLOR>* MipMap<COLOR>::lod(int d) const
    {

        assert(d >= 0 && d < m_numLods);

        return m_lods[d];
    }

    //******************************************************************************
    template <class COLOR> inline Image<COLOR>* MipMap<COLOR>::lod(int d)
    {

        assert(d >= 0 && d < m_numLods);

        return m_lods[d];
    }

    //******************************************************************************
    template <class COLOR>
    inline const TwkMath::Vec2i& MipMap<COLOR>::origin() const
    {
        return m_origin;
    }

    //******************************************************************************
    template <class COLOR>
    void MipMap<COLOR>::specialCaseSquare(Image<COLOR>& src, Image<COLOR>& dst)
    {
        // So we only have to handle half as many cases, we
        // transpose the images if it's not oriented how
        // we expect it to be.
        bool transposeWhenDone = false;
        if (src.width() < src.height())
        {
            src.transpose();
            dst.transpose();
            transposeWhenDone = true;
        }

        // Temporary variables.
        const int srcWidth = src.width();
        const int srcHeight = src.height();
        COLOR tmp[3];

        if (srcWidth == 3)
        {
            if (srcHeight == 3)
            {
                dst.pixel(0, 0) = (src.pixel(0, 0) + src.pixel(1, 0)
                                   + src.pixel(0, 1) + src.pixel(1, 1))
                                      * center3x3
                                  +

                                  (src.pixel(0, 2) + src.pixel(1, 2)
                                   + src.pixel(2, 0) + src.pixel(2, 1))
                                      * edge3x3
                                  +

                                  (src.pixel(2, 2)) * corner3x3;

                // All of the other cases involve very partial coverage
                // by the masks, 2x2.
                dst.pixel(1, 0) = (src.pixel(2, 0)) * center2x2 +

                                  (src.pixel(1, 0) + src.pixel(2, 1)) * edge2x2
                                  +

                                  (src.pixel(1, 1)) * corner2x2;

                dst.pixel(0, 1) = (src.pixel(0, 2)) * center2x2 +

                                  (src.pixel(0, 1) + src.pixel(1, 2)) * edge2x2
                                  +

                                  (src.pixel(1, 1)) * corner2x2;

                dst.pixel(1, 1) = (src.pixel(2, 2)) * center2x2 +

                                  (src.pixel(1, 2) + src.pixel(2, 1)) * edge2x2
                                  +

                                  (src.pixel(1, 1)) * corner2x2;
            }
            else if (srcHeight == 2)
            {
                // Well, it's 3x2, so first average the pixels vertically.
                tmp[0] = 0.5f * (src.pixel(0, 0) + src.pixel(0, 1));
                tmp[1] = 0.5f * (src.pixel(1, 0) + src.pixel(1, 1));
                tmp[2] = 0.5f * (src.pixel(2, 0) + src.pixel(2, 1));

                // Now it's 3x1, so average the two with the 3x1 filter
                dst.pixel(0, 0) =
                    (tmp[0] + tmp[1]) * center3x1 + (tmp[2]) * edge3x1;
                dst.pixel(1, 0) = (tmp[2]) * center2x1 + (tmp[1]) * edge2x1;
            }
            else
            {
                // It's 3x1, so average it with the 3x1 filter

                assert(srcHeight == 1);

                dst.pixel(0, 0) =
                    (src.pixel(0, 0) + src.pixel(1, 0)) * center3x1
                    + (src.pixel(2, 0)) * edge3x1;
                dst.pixel(1, 0) =
                    (src.pixel(2, 0)) * center2x1 + (src.pixel(1, 0)) * edge2x1;
            }
        }
        else
        {

            assert(srcWidth == 2);

            if (srcHeight == 2)
            {
                dst.pixel(0, 0) = 0.25f
                                  * (src.pixel(0, 0) + src.pixel(1, 0)
                                     + src.pixel(0, 1) + src.pixel(1, 1));
            }
            else
            {

                assert(srcHeight == 1);

                dst.pixel(0, 0) = 0.5f * (src.pixel(0, 0) + src.pixel(1, 0));
            }
        }

        // If we had transposed the images when we began,
        // fix that.
        if (transposeWhenDone)
        {
            src.transpose();
            dst.transpose();
        }
    }

    //******************************************************************************
    template <class COLOR>
    void MipMap<COLOR>::specialCaseRectangle(Image<COLOR>& srcImg,
                                             Image<COLOR>& dstImg)
    {
        // Just like the special case square, we want an
        // image that's aligned conveniently for us. We want
        // a situation in which the image is a vertical sliver,
        // meaning the width is less than 4 and the height is
        // greater than or equal to 4, so height > width.
        // If we don't have that, transpose.
        bool transposeWhenDone = false;
        if (srcImg.height() < srcImg.width())
        {
            srcImg.transpose();
            dstImg.transpose();
            transposeWhenDone = true;
        }

        // Temporary variables
        const int srcWidth = srcImg.width();
        const int srcHeight = srcImg.height();
        const int dstHeight = dstImg.height();

        assert(srcWidth < 4);
        assert(srcHeight >= 4);

        COLOR* src;
        int srcStride;
        int srcStride2;
        COLOR* srcLast;
        COLOR* tmp;
        int tmpStride;
        int tmpStride2;
        COLOR* dst;
        int dstStride;
        COLOR* dstLast;

        // This means that srcWidth is less than 4 while height
        // is greater than or equal to 4.
        if (srcWidth == 3)
        {
            // Build a srcHeight x 2 temporary array.
            Image<COLOR>* tmpImg = new Image<COLOR>(2, srcHeight);

            // Loop over src pixels to create averaged
            // pixels
            src = srcImg.pixels();
            srcStride = 3;
            srcStride2 = 6;
            srcLast = src + ((srcHeight - 1) * srcStride);
            tmp = tmpImg->pixels();
            tmpStride = 2;
            tmpStride2 = 4;
            for (; src <= srcLast; src += srcStride, tmp += tmpStride)
            {
                // Here we're averaging pixels horizontally
                tmp[0] = (src[0] + src[1]) * center3x1 + (src[2]) * edge3x1;
                tmp[1] = (src[2]) * center2x1 + (src[1]) * edge2x1;
            }

            // Create dst pixels.
            dst = dstImg.pixels();
            dstStride = 2;
            dstLast = dst + ((dstHeight - 1) * dstStride);
            tmp = tmpImg->pixels();

            // Do the first pixels.
            // Indices 0 & 1 are invalid.
            dst[0] = (tmp[2] + tmp[4]) * center3x1 + (tmp[6]) * edge3x1;
            dst[1] = (tmp[3] + tmp[5]) * center3x1 + (tmp[7]) * edge3x1;

            // Loop over all the pixels except the last.
            for (dst += dstStride, tmp += tmpStride2; dst < dstLast;
                 dst += dstStride, tmp += tmpStride2)
            {
                // First dst row is made up of
                // tmp[0] tmp[2] tmp[4] tmp[6]
                // tmp[1] tmp[3] tmp[5] tmp[7]
                dst[0] =
                    (tmp[2] + tmp[4]) * center4x1 + (tmp[0] + tmp[6]) * edge4x1;
                dst[1] =
                    (tmp[3] + tmp[5]) * center4x1 + (tmp[1] + tmp[7]) * edge4x1;
            }

            // Do the last pixels
            if (isOdd(srcHeight))
            {
                // Odd.
                // Indices 4-7 are invalid.
                dst[0] = (tmp[2]) * center2x1 + (tmp[0]) * edge2x1;
                dst[1] = (tmp[3]) * center2x1 + (tmp[1]) * edge2x1;
            }
            else
            {
                // Even.
                // Indices 6-7 are invalid.
                dst[0] = (tmp[2] + tmp[4]) * center3x1 + (tmp[0]) * edge3x1;
                dst[1] = (tmp[3] + tmp[5]) * center3x1 + (tmp[1]) * edge3x1;
            }

            delete tmpImg;
        }
        else if (srcWidth == 2)
        {
            // Build a srcHeight x 1 temporary array.
            Image<COLOR>* tmpImg = new Image<COLOR>(1, srcHeight);

            // Loop over src pixels to create averaged
            // pixels
            src = srcImg.pixels();
            srcStride = 2;
            srcLast = src + ((srcHeight - 1) * srcStride);
            tmp = tmpImg->pixels();
            tmpStride = 1;
            tmpStride2 = 2;
            for (; src <= srcLast; src += srcStride, tmp += tmpStride)
            {
                // Here we're averaging pixels horizontally
                (*tmp) = (src[0] + src[1]) * 0.5f;
            }

            // Create dst pixels.
            dst = dstImg.pixels();
            dstStride = 1;
            dstLast = dst + ((dstHeight - 1) * dstStride);
            tmp = tmpImg->pixels();

            // Do the first pixels.
            // Indices 0 & 1 are invalid.
            (*dst) = (tmp[1] + tmp[2]) * center3x1 + (tmp[3]) * edge3x1;

            // Loop over all the pixels except the last.
            for (dst += dstStride, tmp += tmpStride2; dst < dstLast;
                 dst += dstStride, tmp += tmpStride2)
            {
                // First dst row is made up of
                // tmp[0] tmp[1] tmp[2] tmp[3]
                (*dst) =
                    (tmp[1] + tmp[2]) * center4x1 + (tmp[0] + tmp[3]) * edge4x1;
            }

            // Do the last pixels
            if (isOdd(srcHeight))
            {
                // Odd.
                // Indices 2-3 are invalid.
                (*dst) = (tmp[1]) * center2x1 + (tmp[0]) * edge2x1;
            }
            else
            {
                // Even.
                // Index 3 is invalid.
                (*dst) = (tmp[1] + tmp[2]) * center3x1 + (tmp[0]) * edge3x1;
            }

            delete tmpImg;
        }
        else
        {
            assert(srcWidth == 1);

            // We don't need to make a temporary image - pixels
            // are already in a row.
            dst = dstImg.pixels();
            dstStride = 1;
            dstLast = dst + ((dstHeight - 1) * dstStride);
            src = srcImg.pixels();
            srcStride = 1;
            srcStride2 = 2;

            // Do the first pixels.
            // Index 0 is invalid.
            (*dst) = (src[1] + src[2]) * center3x1 + (src[3]) * edge3x1;

            // Loop over all the pixels except the last.
            for (dst += dstStride, src += srcStride2; dst < dstLast;
                 dst += dstStride, src += srcStride2)
            {
                // First dst row is made up of
                // tmp[0] tmp[1] tmp[2] tmp[3]
                (*dst) =
                    (src[1] + src[2]) * center4x1 + (src[0] + src[3]) * edge4x1;
            }

            // Do the last pixels
            if (isOdd(srcHeight))
            {
                // Odd.
                // Indices 2-3 are invalid.
                (*dst) = (src[1]) * center2x1 + (src[0]) * edge2x1;
            }
            else
            {
                // Even.
                // Index 3 is invalid.
                (*dst) = (src[1] + src[2]) * center3x1 + (src[0]) * edge3x1;
            }
        }

        // If we had transposed in the beginning, fix that.
        if (transposeWhenDone)
        {
            srcImg.transpose();
            dstImg.transpose();
        }
    }

} // End namespace TwkImg

#endif
