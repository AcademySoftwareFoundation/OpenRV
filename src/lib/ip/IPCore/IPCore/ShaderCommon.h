//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ShaderCommon__h__
#define __IPCore__ShaderCommon__h__
#include <iostream>
#include <IPCore/ShaderExpression.h>
#include <IPCore/IPImage.h>
#include <TwkMath/Mat44.h>

namespace IPCore
{
    namespace Shader
    {

        //
        //  Function Application
        //

        Expression* newSourcePlanarYAC2(const IPImage*, const TwkMath::Vec3f&);
        Expression* newSourcePlanarYAC2Uncrop(const IPImage*,
                                              const TwkMath::Vec3f&);
        Expression* newSourcePlanarYRYBYA(const IPImage*,
                                          const TwkMath::Vec3f&);
        Expression* newSourcePlanarYRYBYAUncrop(const IPImage*,
                                                const TwkMath::Vec3f&);
        Expression* newSourcePlanarYRYBY(const IPImage*, const TwkMath::Vec3f&);
        Expression* newSourcePlanarYRYBYUncrop(const IPImage*,
                                               const TwkMath::Vec3f&);
        Expression* newSourcePlanarYUVA(const IPImage*, const TwkMath::Mat44f&);
        Expression* newSourcePlanarYUVAUncrop(const IPImage*,
                                              const TwkMath::Mat44f&);
        Expression* newSourcePlanarYUV(const IPImage*, const TwkMath::Mat44f&);
        Expression* newSourcePlanar2YUV(const IPImage*, const TwkMath::Mat44f&);
        Expression* newSourcePlanarYUVUncrop(const IPImage*,
                                             const TwkMath::Mat44f&);
        Expression* newSourcePlanarRGB(const IPImage*);
        Expression* newSourcePlanarRGBUncrop(const IPImage*);
        Expression* newSourcePlanarRGBA(const IPImage*);
        Expression* newSourcePlanarRGBAUncrop(const IPImage*);
        Expression* newSourcePlanarYA(const IPImage*);
        Expression* newSourcePlanarYAUncrop(const IPImage*);
        Expression* newSourceRGBA(const IPImage*);
        Expression* newSourceRGBAUncrop(const IPImage*);
        Expression* newSourceBGRA(const IPImage*);
        Expression* newSourceBGRAUncrop(const IPImage*);
        Expression* newSourceARGB(const IPImage*);
        Expression* newSourceARGBUncrop(const IPImage*);
        Expression* newSourceARGBfromBGRA(const IPImage*);
        Expression* newSourceARGBfromBGRAUncrop(const IPImage*);
        Expression* newSourceABGR(const IPImage*);
        Expression* newSourceABGRUncrop(const IPImage*);
        Expression* newSourceABGRfromBGRA(const IPImage*);
        Expression* newSourceABGRfromBGRAUncrop(const IPImage*);
        Expression* newSourceY(const IPImage*);
        Expression* newSourceYA(const IPImage*);
        Expression* newSourceYVYU(const IPImage*, const TwkMath::Mat44f&);
        Expression* newSourceYUncrop(const IPImage*);
        Expression* newSourceYAUncrop(const IPImage*);
        Expression* newSourceYUVA(const IPImage*, const TwkMath::Mat44f&);
        Expression* newSourceAYUV(const IPImage*, const TwkMath::Mat44f&);

        Expression* newFilterGaussianVertical(Expression*,
                                              const float radius = 10,
                                              const float sigma = 267);
        Expression* newFilterGaussianHorizontal(Expression*,
                                                const float radius = 10,
                                                const float sigma = 267);
        Expression* newFilterGaussianVerticalFast(Expression*,
                                                  const TwkFB::FrameBuffer*,
                                                  const float radius = 10,
                                                  const float sigma = 22);

        Expression* newFilterGaussianHorizontalFast(Expression*,
                                                    const TwkFB::FrameBuffer*,
                                                    const float radius = 10,
                                                    const float sigma = 22);

        Expression* newFilterUnsharpMask(const IPImage*,
                                         const std::vector<Expression*>& FA1,
                                         const float amount = 2.0,
                                         const float threshold = 0.02);

        Expression* newFilterNoiseReduction(const IPImage*,
                                            const std::vector<Expression*>& FA1,
                                            const float amount = 2.0,
                                            const float threshold = 0.02);

        Expression* newFilterClarity(const IPImage*,
                                     const std::vector<Expression*>& FA1,
                                     const float amount = 0.2);

        Expression* newColorLineartoGray(Expression*);

        Expression* newColorSRGBYCbCr(Expression*);
        Expression* newColorYCbCrSRGB(Expression*);
        Expression* newColorCurveonY(Expression* FA, const TwkMath::Vec3f& p1,
                                     const TwkMath::Vec3f& p2,
                                     const TwkMath::Vec3f& p3,
                                     const TwkMath::Vec3f& p4);

        Expression* newColorShadowonY(Expression*, const TwkMath::Vec4f&,
                                      const float);
        Expression* newColorHighlightonY(Expression*, const TwkMath::Vec4f&,
                                         const float);
        Expression* newColorVibrance(Expression*, const TwkMath::Vec3f&,
                                     const float);

        Expression* newColorOutOfRange(Expression*);
        Expression* newColorSRGBToLinear(Expression*);
        Expression* newColorRec709ToLinear(Expression*);
        Expression* newColorSMPTE240MToLinear(Expression*);
        Expression* newColorACESLogToLinear(Expression*);
        Expression* newColorGamma(Expression*, const TwkMath::Vec3f&);

        Expression* newColorYCbCrSRGB(Expression*);
        Expression* newColorSRGBYCbCr(Expression*);
        Expression* newColorTemperatureOffset(Expression*,
                                              const TwkMath::Vec4f&);
        Expression* newColorBlendWithConstant(Expression*,
                                              const TwkMath::Vec4f&);

        //
        //  newColorMatrix: is homogeneous alpha preserving
        //  newColorMatrix4D: is really 4D not homogenous coordinate so alpha
        //                    is used as the 4th component.
        //
        Expression* newColorMatrix(Expression*, const TwkMath::Mat44f&);
        Expression* newColorMatrix4D(Expression*, const TwkMath::Mat44f&);
        // if 0 >= whiteCode <= 1 then assumed [0,1] else assumed [0,1024]
        Expression* newColorCineonLogToLinear(Expression*, double refBlack,
                                              double refWhite, double softClip);
        Expression* newColorLinearToCineonLog(Expression*, double refBlack,
                                              double refWhite);
        Expression* newColorLogCLinear(Expression*, float blackSignal,
                                       float encodingOffset, float encodingGain,
                                       float graySignal, float blackOffset,
                                       float linearSlope, float linearOffset,
                                       float cutoff);
        Expression* newColorLinearLogC(Expression*, float blackSignal,
                                       float encodingOffset, float encodingGain,
                                       float graySignal, float blackOffset,
                                       float linearSlope, float linearOffset,
                                       float cutoff);
        Expression* newColorViperLogToLinear(Expression*);
        Expression* newColorRedLogToLinear(Expression*);
        Expression* newColorLinearToRedLog(Expression*);
        Expression* newColorPremult(Expression*);
        Expression* newColorPremultLight(Expression*);
        Expression* newColorUnpremult(Expression*);
        Expression* newColorLinearToSRGB(Expression*);
        Expression* newColorLinearToRec709(Expression*);
        Expression* newColorLinearToSMPTE240M(Expression*);
        Expression* newColorLinearToACESLog(Expression*);
        Expression* newColor3DLUTGLSampling(Expression*,
                                            const TwkFB::FrameBuffer*,
                                            const TwkMath::Vec3f& inScale,
                                            const TwkMath::Vec3f& inOffset,
                                            const TwkMath::Vec3f& outScale,
                                            const TwkMath::Vec3f& outOffset);
        Expression* newColor3DLUT(Expression*, const TwkFB::FrameBuffer*,
                                  const TwkMath::Vec3f& outScale,
                                  const TwkMath::Vec3f& outOffset);

        Expression* newColorQuantize(Expression*, int partitions);

        Expression* newColorChannelLUT(Expression*, const TwkFB::FrameBuffer*,
                                       const TwkMath::Vec3f& outScale,
                                       const TwkMath::Vec3f& outOffset);
        Expression* newColorLuminanceLUT(Expression*,
                                         const TwkFB::FrameBuffer*);
        Expression* newColorCDL(Expression*, const TwkMath::Vec3f& scale,
                                const TwkMath::Vec3f& offset,
                                const TwkMath::Vec3f& power, float saturation,
                                bool noClamp);

        Expression* newColorCDLForACES(Expression*, const TwkMath::Vec3f& scale,
                                       const TwkMath::Vec3f& offset,
                                       const TwkMath::Vec3f& power,
                                       float saturation, bool noClamp,
                                       bool isACESLog);

        Expression* newColorClamp(Expression*, float minValue = 0.0f,
                                  float maxValue = 1.0f);

        Expression* newConstantBG(const IPImage*, Expression*,
                                  const TwkMath::Vec3f&);

        Expression* newCheckerboardBG(const IPImage*, Expression*);
        Expression* newCrosshatchBG(const IPImage*, Expression*);

        Expression* newDither(const IPImage*, Expression*, size_t displayBits,
                              size_t seed);

        Expression* newStencilBox(const IPImage*, Expression*);

        Expression* newStencilBoxNegAlpha(const IPImage*, Expression*);

        Expression* newStereoChecker(const IPImage*, const float parityXOffset,
                                     const float parityYOffset,
                                     const std::vector<Expression*>&);

        Expression* newStereoScanline(const IPImage*, const float parityYOffset,
                                      const std::vector<Expression*>&);

        Expression* newStereoAnaglyph(const IPImage*,
                                      const std::vector<Expression*>&);

        Expression* newStereoLumAnaglyph(const IPImage*,
                                         const std::vector<Expression*>&);

        Expression* newInlineBoxResize(bool adaptive, const IPImage*,
                                       Expression*,
                                       const TwkMath::Vec2f& scale);

        Expression* newDownSample(const IPImage* image, Expression*,
                                  const int scale);

        Expression* newDerivativeDownSample(Expression* expr);

        Expression* newUpSampleHorizontal(const IPImage* image, Expression*,
                                          bool);

        Expression* newUpSampleVertical(const IPImage* image, Expression*,
                                        bool);

        Expression* newSimpleBoxFilter(const IPImage*, Expression*,
                                       const float size);

        Expression* newBlend(const IPImage*, const std::vector<Expression*>&,
                             const IPImage::BlendMode);

        Expression* newHistogram(const IPImage*,
                                 const std::vector<Expression*>&);

        Expression* newLensWarp(const IPImage*, Expression*, float k1, float k2,
                                float k3, float d, float p1, float p2,
                                const TwkMath::Vec2f& center,
                                const TwkMath::Vec2f& f,
                                const TwkMath::Vec2f& cropRatio);

        Expression* newLensWarp3DE4AnamorphicDegree6(
            const IPImage* image, Expression* FA, const TwkMath::Vec2f& c02,
            const TwkMath::Vec2f& c22, const TwkMath::Vec2f& c04,
            const TwkMath::Vec2f& c24, const TwkMath::Vec2f& c44,
            const TwkMath::Vec2f& c06, const TwkMath::Vec2f& c26,
            const TwkMath::Vec2f& c46, const TwkMath::Vec2f& c66,
            const TwkMath::Vec2f& center, const TwkMath::Vec2f& f,
            const TwkMath::Vec2f& cropRatio);

        //
        //  ICC profile shaders
        //

        Expression* newICCLinearToSRGB(Expression*, const TwkMath::Vec3f& gamma,
                                       const TwkMath::Vec3f& a,
                                       const TwkMath::Vec3f& b,
                                       const TwkMath::Vec3f& c,
                                       const TwkMath::Vec3f& d);

        //
        //  Common Symbols
        //

        const Symbol* P();            // ParameterInOut
        const Symbol* P1();           // ParameterInOut
        const Symbol* P2();           // ParameterInOut
        const Symbol* _offset();      // ParameterInOut
        const Symbol* frame();        // ParameterConstIn
        const Symbol* time();         //        .
        const Symbol* fps();          //        .
        const Symbol* RGBA_sampler(); //        .
        const Symbol* Y_sampler();    //        .
        const Symbol* U_sampler();    //        .
        const Symbol* V_sampler();    //        .
        const Symbol* R_sampler();    //        .
        const Symbol* G_sampler();    //        .
        const Symbol* B_sampler();    //        .
        const Symbol* A_sampler();    //        .
        const Symbol* RY_sampler();   //        .
        const Symbol* BY_sampler();   //        .
        const Symbol* YA_sampler();   //        .
        const Symbol* C2_sampler();   //        .
        const Symbol* ST_coord();     //        .
        const Symbol* ratio0();       //        .
        const Symbol* ratio1();       //        .

        //
        //  Substitute an expression where a Source function exists. The
        //  return expression is the new expression and is either the root or
        //  replacement input. (The replacement will be returned if root is a
        //  Source function). Either way the unused sub-expression is deleted
        //  in the process.
        //
        //  It is assumed that there is only one Source function in root. If
        //  more exist only the first will be replaced. replacement may itself
        //  have Source functions so calling this repeatedly is probably a bad
        //  idea.
        //

        Expression* replaceSourceWithExpression(Expression* root,
                                                Expression* replacement);

        //
        //  Pick a source assembly shader based on the IPImage's
        //  FrameBuffer. Only the renderer should call this with the last two
        //  arguments -- in that case the value of bgra is known (whether or
        //  not we're using a bgra fast path) and in that case it may swap in
        //  another Source function.
        //

        Expression* sourceAssemblyShader(IPImage*, bool bgra = true,
                                         bool force = true);

        // plain gaussian
        IPImage* applyGaussianFilter(const IPNode* node, IPImage* image,
                                     size_t radius, float sigma);
        IPImage* applyGaussianFilter(const IPNode* node, IPImage* image,
                                     size_t radius); // derive default sigma

        // fast gaussian
        TwkFB::FrameBuffer* generateFastGaussianWeightsFB(size_t radius,
                                                          float sigma);
        TwkFB::FrameBuffer* generateFastGaussianWeightsFB(size_t radius);
        IPImage* applyFastGaussianFilter(const IPNode* node, IPImage* image,
                                         size_t radius);
        IPImage* applyFastGaussianFilter(const IPNode* node, IPImage* image,
                                         size_t radius, float sigma);

    } // namespace Shader
} // namespace IPCore

#endif // __IPCore__ShaderCommon__h__
