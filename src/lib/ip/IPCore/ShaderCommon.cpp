//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/ShaderCommon.h>
#include <IPCore/ShaderState.h>
#include <TwkGLF/GL.h>
#include <TwkFB/Operations.h>
#include <TwkMath/MatrixColor.h>
#include <boost/thread/mutex.hpp>
#include <boost/functional/hash.hpp>

#include <limits>

#define DECLARE_SYMBOL(NAME, QUALIFIER, TYPE)                            \
    const Symbol* NAME()                                                 \
    {                                                                    \
        return new Symbol((Symbol::Qualifier)(Symbol::QUALIFIER), #NAME, \
                          Symbol::TYPE);                                 \
    }

//
//  External source code blobs. NOTE: has to be in the global
//  namespace
//

extern const char* SourceY_glsl;
extern const char* SourceYUVA_glsl;
extern const char* SourceAYUV_glsl;
extern const char* SourceYA_glsl;
extern const char* SourceYVYU_glsl;
extern const char* SourceYUncrop_glsl;
extern const char* SourceYAUncrop_glsl;
extern const char* SourcePlanarYA_glsl;
extern const char* SourcePlanarYAUncrop_glsl;
extern const char* SourceRGBA_glsl;
extern const char* SourceRGBAUncrop_glsl;
extern const char* SourceBGRA_glsl;
extern const char* SourceBGRAUncrop_glsl;
extern const char* SourceARGB_glsl;
extern const char* SourceARGBUncrop_glsl;
extern const char* SourceARGBfromBGRA_glsl;
extern const char* SourceARGBfromBGRAUncrop_glsl;
extern const char* SourceABGR_glsl;
extern const char* SourceABGRUncrop_glsl;
extern const char* SourceABGRfromBGRA_glsl;
extern const char* SourceABGRfromBGRAUncrop_glsl;
extern const char* SourcePlanarRGBA_glsl;
extern const char* SourcePlanarRGBAUncrop_glsl;
extern const char* SourcePlanarRGB_glsl;
extern const char* SourcePlanarRGBUncrop_glsl;
extern const char* SourcePlanarYUV_glsl;
extern const char* SourcePlanar2YUV_glsl;
extern const char* SourcePlanarYUVUncrop_glsl;
extern const char* SourcePlanarYUVA_glsl;
extern const char* SourcePlanarYUVAUncrop_glsl;
extern const char* SourcePlanarYRYBY_glsl;
extern const char* SourcePlanarYRYBYUncrop_glsl;
extern const char* SourcePlanarYRYBYA_glsl;
extern const char* SourcePlanarYRYBYAUncrop_glsl;
extern const char* SourcePlanarYAC2_glsl;
extern const char* SourcePlanarYAC2Uncrop_glsl;
extern const char* ColorCurve_glsl;
extern const char* ColorShadow_glsl;
extern const char* ColorHighlight_glsl;
extern const char* ColorVibrance_glsl;
extern const char* ColorLinearGray_glsl;
extern const char* ColorTemperatureOffset_glsl;
extern const char* ColorBlendWithConstant_glsl;
extern const char* ColorSRGBLinear_glsl;
extern const char* ColorRec709Linear_glsl;
extern const char* ColorSMPTE240MLinear_glsl;
extern const char* ColorACESLogLinear_glsl;
extern const char* ColorClamp_glsl;
extern const char* ColorGamma_glsl;
extern const char* ColorMatrix_glsl;
extern const char* ColorMatrix4D_glsl;
extern const char* ColorCineonLogLinear_glsl;
extern const char* ColorLinearCineonLog_glsl;
extern const char* ColorCineonSoftClipLogLinear_glsl;
extern const char* ColorLogCLinear_glsl;
extern const char* ColorLinearLogC_glsl;
extern const char* ColorLinearSMPTE240M_glsl;
extern const char* ColorViperLogLinear_glsl;
extern const char* ColorRedLogLinear_glsl;
extern const char* ColorLinearRedLog_glsl;
extern const char* ColorPremult_glsl;
extern const char* ColorPremultLight_glsl;
extern const char* ColorUnpremult_glsl;
extern const char* ColorLinearSRGB_glsl;
extern const char* ColorLinearRec709_glsl;
extern const char* ColorLinearACESLog_glsl;
extern const char* Color3DLUT_glsl;
extern const char* Color3DLUTGLSampling_glsl;
extern const char* ColorChannelLUT_glsl;
extern const char* ColorCDL_glsl;
extern const char* ColorCDL_SAT_glsl;
extern const char* ColorCDL_SAT_noClamp_glsl;
extern const char* ColorCDLForACESLinear_glsl;
extern const char* ColorCDLForACESLog_glsl;
extern const char* ColorLuminanceLUT_glsl;
extern const char* ColorOutOfRange_glsl;
extern const char* ColorSRGBYCbCr_glsl;
extern const char* ColorYCbCrSRGB_glsl;
extern const char* ColorQuantize_glsl;
extern const char* InlineBoxResize_glsl;
extern const char* InlineAdaptiveBoxResize_glsl;
extern const char* Dither_glsl;
extern const char* StencilBox_glsl;
extern const char* StencilBoxNegAlpha_glsl;
extern const char* StereoScanline_glsl;
extern const char* StereoChecker_glsl;
extern const char* StereoAnaglyph_glsl;
extern const char* StereoLumAnaglyph_glsl;
extern const char* Over2_glsl;
extern const char* Over3_glsl;
extern const char* Over4_glsl;
extern const char* Replace2_glsl;
extern const char* Replace3_glsl;
extern const char* Replace4_glsl;
extern const char* Add2_glsl;
extern const char* Add3_glsl;
extern const char* Add4_glsl;
extern const char* Difference2_glsl;
extern const char* Difference3_glsl;
extern const char* Difference4_glsl;
extern const char* ReverseDifference2_glsl;
extern const char* ReverseDifference3_glsl;
extern const char* ReverseDifference4_glsl;
extern const char* InlineDissolve2_glsl;
extern const char* BoxFilter_glsl;
extern const char* ConstantBG_glsl;
extern const char* CheckerboardBG_glsl;
extern const char* CrosshatchBG_glsl;
extern const char* LensWarpRadial_glsl;
extern const char* LensWarpTangential_glsl;
extern const char* LensWarpRadialAndTangential_glsl;
extern const char* LensWarp3DE4AnamorphicDegree6_glsl;
extern const char* ResizeDownSample_glsl;
extern const char* ResizeDownSampleFast_glsl;
extern const char* ResizeDownSampleDerivative_glsl;
extern const char* ResizeUpSampleVertical_glsl;
extern const char* ResizeUpSampleHorizontal_glsl;
extern const char* ResizeUpSampleVerticalMitchell_glsl;
extern const char* ResizeUpSampleHorizontalMitchell_glsl;
extern const char* FilterGaussianHorizontal_glsl;
extern const char* FilterGaussianVertical_glsl;
extern const char* FilterGaussianHorizontalFast_glsl;
extern const char* FilterGaussianVerticalFast_glsl;
extern const char* FilterUnsharpMask_glsl;
extern const char* FilterNoiseReduction_glsl;
extern const char* FilterClarity_glsl;
extern const char* Histogram_glsl;
extern const char* ICCLinearSRGB_glsl;

namespace IPCore
{
    namespace Shader
    {
        using namespace std;
        using namespace TwkFB;
        using namespace TwkMath;

        //
        // Common symbols. Initialize these on the fly so windows DLLs work
        // *sigh*. Also: every time one of these is added to a Function
        // parameter or global list, these need to return a *new* version of
        // the symbol. Functions should not share symbol objects even if they
        // have the same arguments -- each one needs to have a unique
        // pointer.
        //

        DECLARE_SYMBOL(P, ParameterConstIn, Vec4fType)
        DECLARE_SYMBOL(P1, ParameterConstIn, Vec4fType)
        DECLARE_SYMBOL(P2, ParameterConstIn, Vec4fType)
        DECLARE_SYMBOL(_offset, SpecialParameterConstIn, Vec2fType)
        DECLARE_SYMBOL(offset, SpecialParameterConstIn, Vec2fType)
        DECLARE_SYMBOL(globalFrame, ParameterConstIn, FloatType)
        DECLARE_SYMBOL(localFrame, ParameterConstIn, FloatType)
        DECLARE_SYMBOL(time, SpecialParameterConstIn, FloatType)
        DECLARE_SYMBOL(fps, ParameterConstIn, FloatType)
        DECLARE_SYMBOL(RGBA_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(samplerSize, ParameterConstIn, Vec2fType)
        DECLARE_SYMBOL(uncropOrigin, ParameterConstIn, Vec2fType)
        DECLARE_SYMBOL(uncropDimension, ParameterConstIn, Vec2fType)
        DECLARE_SYMBOL(Y_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(YUVA_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(YVYU_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(U_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(V_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(R_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(G_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(B_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(A_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(RY_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(BY_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(YA_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(RYBY_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(UV_sampler, ParameterConstIn, Sampler2DRectType)
        DECLARE_SYMBOL(ST_coord, ParameterConstIn, Coord2DRectType)
        DECLARE_SYMBOL(ratio0, ParameterConstIn, Vec2fType);
        DECLARE_SYMBOL(ratio1, ParameterConstIn, Vec2fType);
        DECLARE_SYMBOL(in0, ParameterConstIn, InputImageType);
        DECLARE_SYMBOL(in1, ParameterConstIn, InputImageType);
        DECLARE_SYMBOL(in2, ParameterConstIn, InputImageType);
        DECLARE_SYMBOL(in3, ParameterConstIn, InputImageType);
        DECLARE_SYMBOL(in4, ParameterConstIn, InputImageType);

        //
        //  Cached global pointers to Function objects including the
        //  parameters. Right now you have add the parameters by hand since
        //  there's no easy way to parse the shader. Its also unclear how they
        //  are going to be ultimately used.
        //
        //  MULTITHREADING NOTE: these suckers are probably getting redefined
        //  multiple times by various threads. Fortunately, it doesn't really
        //  matter although there may be some extra ShaderProgram copies.
        //  Ideally, the lazy building would all get done by the main thread at
        //  some point *before* any caching threads get started.
        //

        static Function* Shader_SourceY = 0;
        static Function* Shader_SourceYUVA = 0;
        static Function* Shader_SourceAYUV = 0;
        static Function* Shader_SourceYA = 0;
        static Function* Shader_SourceYVYU = 0;
        static Function* Shader_SourceYUncrop = 0;
        static Function* Shader_SourceYAUncrop = 0;
        static Function* Shader_SourcePlanarYA = 0;
        static Function* Shader_SourcePlanarYAUncrop = 0;
        static Function* Shader_SourceRGBA = 0;
        static Function* Shader_SourceRGBAUncrop = 0;
        static Function* Shader_SourceBGRA = 0;
        static Function* Shader_SourceBGRAUncrop = 0;
        static Function* Shader_SourceARGB = 0;
        static Function* Shader_SourceARGBUncrop = 0;
        static Function* Shader_SourceARGBfromBGRA = 0;
        static Function* Shader_SourceARGBfromBGRAUncrop = 0;
        static Function* Shader_SourceABGR = 0;
        static Function* Shader_SourceABGRUncrop = 0;
        static Function* Shader_SourceABGRfromBGRA = 0;
        static Function* Shader_SourceABGRfromBGRAUncrop = 0;
        static Function* Shader_SourcePlanarRGBA = 0;
        static Function* Shader_SourcePlanarRGBAUncrop = 0;
        static Function* Shader_SourcePlanarRGB = 0;
        static Function* Shader_SourcePlanarRGBUncrop = 0;
        static Function* Shader_SourcePlanarYUV = 0;
        static Function* Shader_SourcePlanar2YUV = 0;
        static Function* Shader_SourcePlanarYUVUncrop = 0;
        static Function* Shader_SourcePlanarYUVA = 0;
        static Function* Shader_SourcePlanarYUVAUncrop = 0;
        static Function* Shader_SourcePlanarYRYBY = 0;
        static Function* Shader_SourcePlanarYRYBYUncrop = 0;
        static Function* Shader_SourcePlanarYRYBYA = 0;
        static Function* Shader_SourcePlanarYRYBYAUncrop = 0;
        static Function* Shader_SourcePlanarYAC2 = 0;
        static Function* Shader_SourcePlanarYAC2Uncrop = 0;
        static Function* Shader_ColorLinearGray = 0;
        static Function* Shader_ColorTemperatureOffset = 0;
        static Function* Shader_ColorBlendWithConstant = 0;
        static Function* Shader_ColorSRGBLinear = 0;
        static Function* Shader_ColorRec709Linear = 0;
        static Function* Shader_ColorSMPTE240MLinear = 0;
        static Function* Shader_ColorLinearSMPTE240M = 0;
        static Function* Shader_ColorACESLogLinear = 0;
        static Function* Shader_ColorClamp = 0;
        static Function* Shader_ColorGamma = 0;
        static Function* Shader_ColorMatrix = 0;
        static Function* Shader_ColorMatrix4D = 0;
        static Function* Shader_ColorCineonLogLinear = 0;
        static Function* Shader_ColorLinearCineonLog = 0;
        static Function* Shader_ColorCineonSoftClipLogLinear = 0;
        static Function* Shader_ColorLogCLinear = 0;
        static Function* Shader_ColorLinearLogC = 0;
        static Function* Shader_ColorViperLogLinear = 0;
        static Function* Shader_ColorRedLogLinear = 0;
        static Function* Shader_ColorLinearRedLog = 0;
        static Function* Shader_ColorPremult = 0;
        static Function* Shader_ColorPremultLight = 0;
        static Function* Shader_ColorUnpremult = 0;
        static Function* Shader_ColorLinearACESLog = 0;
        static Function* Shader_ColorLinearSRGB = 0;
        static Function* Shader_ColorLinearRec709 = 0;
        static Function* Shader_Color3DLUTGLSampling = 0;
        static Function* Shader_Color3DLUT = 0;
        static Function* Shader_ColorChannelLUT = 0;
        static Function* Shader_ColorCDL = 0;
        static Function* Shader_ColorCDL_SAT = 0;
        static Function* Shader_ColorCDL_SAT_noClamp = 0;
        static Function* Shader_ColorCDLForACESLinear = 0;
        static Function* Shader_ColorCDLForACESLog = 0;
        static Function* Shader_ColorLuminanceLUT = 0;
        static Function* Shader_ColorOutOfRange = 0;
        static Function* Shader_ColorSRGBYCbCr = 0;
        static Function* Shader_ColorYCbCrSRGB = 0;
        static Function* Shader_ColorVibrance = 0;
        static Function* Shader_ColorCurve = 0;
        static Function* Shader_ColorShadow = 0;
        static Function* Shader_ColorHighlight = 0;
        static Function* Shader_ColorQuantize = 0;
        static Function* Shader_Merge2Over = 0;
        static Function* Shader_CrossDissolve = 0;
        static Function* Shader_InlineFilterSobel = 0;
        static Function* Shader_InlineBoxResize = 0;
        static Function* Shader_InlineAdaptiveBoxResize = 0;
        static Function* Shader_BoxFilter = 0;
        static Function* Shader_Dither = 0;
        static Function* Shader_StencilBox = 0;
        static Function* Shader_StencilBoxNegAlpha = 0;
        static Function* Shader_StereoScanline = 0;
        static Function* Shader_StereoChecker = 0;
        static Function* Shader_StereoAnaglyph = 0;
        static Function* Shader_StereoLumAnaglyph = 0;
        static Function* Shader_Over = 0;
        static Function* Shader_Add = 0;
        static Function* Shader_Difference = 0;
        static Function* Shader_ReverseDifference = 0;
        static Function* Shader_InlineDissolve = 0;
        static Function* Shader_LensWarpRadial = 0;
        static Function* Shader_LensWarpTangential = 0;
        static Function* Shader_LensWarpRadialAndTangential = 0;
        static Function* Shader_LensWarp3DE4AnamorphicDegree6 = 0;
        static Function* Shader_ResizeDownSample = 0;
        static Function* Shader_ResizeDownSampleFast = 0;
        static Function* Shader_ResizeDownSampleDerivative = 0;
        static Function* Shader_ResizeUpSampleVertical2 = 0;
        static Function* Shader_ResizeUpSampleHorizontal2 = 0;
        static Function* Shader_ResizeUpSampleVerticalMitchell2 = 0;
        static Function* Shader_ResizeUpSampleHorizontalMitchell2 = 0;
        static Function* Shader_FilterGaussianVertical = 0;
        static Function* Shader_FilterGaussianHorizontal = 0;
        static Function* Shader_FilterGaussianVerticalFast = 0;
        static Function* Shader_FilterGaussianHorizontalFast = 0;
        static Function* Shader_FilterUnsharpMask = 0;
        static Function* Shader_FilterNoiseReduction = 0;
        static Function* Shader_FilterClarity = 0;
        static Function* Shader_Histogram = 0;
        static Function* Shader_ICCLinearSRGB = 0;

// NOTE: SUBOPTIMAL: the value here MUST be greater than or equal to
// the value of MAX_TEXTURE_PER_BLEND in StackIPNode.cpp
#define MAX_TEXTURE_PER_SHADER 4

        Function* dither()
        {
            if (!Shader_Dither)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "scale",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "seed",
                                            Symbol::FloatType));
                Shader_Dither = new Shader::Function(
                    "main", Dither_glsl, Shader::Function::UndecidedType,
                    params, globals);
            }

            return Shader_Dither;
        }

        Function* stencilBox()
        {
            if (!Shader_StencilBox)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "boxX",
                                            Symbol::Vec2fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "boxY",
                                            Symbol::Vec2fType));
                Shader_StencilBox = new Shader::Function(
                    "main", StencilBox_glsl, Shader::Function::Color, params,
                    globals);
            }

            return Shader_StencilBox;
        }

        Function* stencilBoxNegAlpha()
        {
            if (!Shader_StencilBoxNegAlpha)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "boxX",
                                            Symbol::Vec2fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "boxY",
                                            Symbol::Vec2fType));
                Shader_StencilBoxNegAlpha = new Shader::Function(
                    "main", StencilBoxNegAlpha_glsl, Shader::Function::Color,
                    params, globals);
            }

            return Shader_StencilBoxNegAlpha;
        }

        Function* stereoChecker()
        {
            if (!Shader_StereoChecker)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in1",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "parityXOffset",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "parityYOffset",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                Shader_StereoChecker = new Shader::Function(
                    "main", StereoChecker_glsl, Shader::Function::UndecidedType,
                    params, globals);
            }

            return Shader_StereoChecker;
        }

        Function* stereoScanline()
        {
            if (!Shader_StereoScanline)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in1",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "parityOffset", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                Shader_StereoScanline = new Shader::Function(
                    "main", StereoScanline_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_StereoScanline;
        }

        Function* stereoAnaglyph()
        {
            if (!Shader_StereoAnaglyph)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in1",
                                            Symbol::Vec4fType));
                Shader_StereoAnaglyph = new Shader::Function(
                    "main", StereoAnaglyph_glsl, Shader::Function::Color,
                    params, globals);
            }

            return Shader_StereoAnaglyph;
        }

        Function* stereoLumAnaglyph()
        {
            if (!Shader_StereoLumAnaglyph)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in1",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "lumaCoefficients",
                                            Symbol::Vec3fType));
                Shader_StereoLumAnaglyph = new Shader::Function(
                    "main", StereoLumAnaglyph_glsl, Shader::Function::Color,
                    params, globals);
            }

            return Shader_StereoLumAnaglyph;
        }

        Function* unsharpMask()
        {
            if (!Shader_FilterUnsharpMask)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in1",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "amount",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "threshold", Symbol::FloatType));
                Shader_FilterUnsharpMask = new Shader::Function(
                    "FilterUnsharpMask", FilterUnsharpMask_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_FilterUnsharpMask;
        }

        Function* noiseReduction()
        {
            if (!Shader_FilterNoiseReduction)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in1",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "amount",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "threshold", Symbol::FloatType));
                Shader_FilterNoiseReduction = new Shader::Function(
                    "FilterNoiseReduction", FilterNoiseReduction_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_FilterNoiseReduction;
        }

        Function* clarity()
        {
            if (!Shader_FilterClarity)
            {
                SymbolVector params, globals;
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in0",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "in1",
                                            Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "amount",
                                            Symbol::FloatType));
                Shader_FilterClarity = new Shader::Function(
                    "FilterClarity", FilterClarity_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_FilterClarity;
        }

        Function* histogram()
        {
            if (!Shader_Histogram)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                Shader_Histogram = new Shader::Function(
                    "Histogram", Histogram_glsl, Shader::Function::Filter,
                    params, globals);
            }

            return Shader_Histogram;
        }

        Function* filterGaussianVertical()
        {
            if (!Shader_FilterGaussianVertical)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "radius",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "sigma",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                Shader_FilterGaussianVertical = new Shader::Function(
                    "FilterGaussianVertical", FilterGaussianVertical_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_FilterGaussianVertical;
        }

        Function* filterGaussianHorizontal()
        {
            if (!Shader_FilterGaussianHorizontal)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "radius",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "sigma",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                Shader_FilterGaussianHorizontal = new Shader::Function(
                    "FilterGaussianHorizontal", FilterGaussianHorizontal_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_FilterGaussianHorizontal;
        }

        Function* filterGaussianVerticalFast()
        {
            if (!Shader_FilterGaussianVerticalFast)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "RGB_sampler",
                                            Symbol::Sampler2DRectType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "radius",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                Shader_FilterGaussianVerticalFast = new Shader::Function(
                    "FilterGaussianVerticalFast",
                    FilterGaussianVerticalFast_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_FilterGaussianVerticalFast;
        }

        Function* filterGaussianHorizontalFast()
        {
            if (!Shader_FilterGaussianHorizontalFast)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "RGB_sampler",
                                            Symbol::Sampler2DRectType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "radius",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                Shader_FilterGaussianHorizontalFast = new Shader::Function(
                    "FilterGaussianHorizontalFast",
                    FilterGaussianHorizontalFast_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_FilterGaussianHorizontalFast;
        }

        Function* resizeDownSample()
        {
            if (!Shader_ResizeDownSample)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "scale",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));

                Shader_ResizeDownSample = new Shader::Function(
                    "ResizeDownSample", ResizeDownSample_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_ResizeDownSample;
        }

        Function* resizeDownSampleFast()
        {
            if (!Shader_ResizeDownSampleFast)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "scale",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));

                Shader_ResizeDownSampleFast = new Shader::Function(
                    "ResizeDownSampleFast", ResizeDownSampleFast_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_ResizeDownSampleFast;
        }

        Function* resizeDownSampleDerivative()
        {
            if (!Shader_ResizeDownSampleDerivative)
            {
                SymbolVector params, globals;
                params.push_back(in0());

                Shader_ResizeDownSampleDerivative = new Shader::Function(
                    "ResizeDownSampleDerivative",
                    ResizeDownSampleDerivative_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_ResizeDownSampleDerivative;
        }

        Function* resizeUpSampleVertical2()
        {
            if (!Shader_ResizeUpSampleVertical2)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                // params.push_back(_offset());

                Shader_ResizeUpSampleVertical2 = new Shader::Function(
                    "ResizeUpSampleVertical", ResizeUpSampleVertical_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_ResizeUpSampleVertical2;
        }

        Function* resizeUpSampleVerticalMitchell2()
        {
            if (!Shader_ResizeUpSampleVerticalMitchell2)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                // params.push_back(_offset());

                Shader_ResizeUpSampleVerticalMitchell2 = new Shader::Function(
                    "ResizeUpSampleVerticalMitchell",
                    ResizeUpSampleVerticalMitchell_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_ResizeUpSampleVerticalMitchell2;
        }

        Function* resizeUpSampleHorizontal2()
        {
            if (!Shader_ResizeUpSampleHorizontal2)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                // params.push_back(_offset());

                Shader_ResizeUpSampleHorizontal2 = new Shader::Function(
                    "ResizeUpSampleHorizontal", ResizeUpSampleHorizontal_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_ResizeUpSampleHorizontal2;
        }

        Function* resizeUpSampleHorizontalMitchell2()
        {
            if (!Shader_ResizeUpSampleHorizontalMitchell2)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "win",
                                            Symbol::OutputImageType));
                // params.push_back(_offset());

                Shader_ResizeUpSampleHorizontalMitchell2 = new Shader::Function(
                    "ResizeUpSampleHorizontalMitchell",
                    ResizeUpSampleHorizontalMitchell_glsl,
                    Shader::Function::UndecidedType, params, globals);
            }

            return Shader_ResizeUpSampleHorizontalMitchell2;
        }

        Function* inlineBoxResize()
        {
            if (!Shader_InlineBoxResize)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(_offset());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "invScale", Symbol::Vec2fType));
                Shader_InlineBoxResize = new Shader::Function(
                    "InlineBoxResize", InlineBoxResize_glsl,
                    Shader::Function::MorphologicalFilter, params, globals);
            }

            return Shader_InlineBoxResize;
        }

        Function* inlineAdaptiveBoxResize()
        {
            if (!Shader_InlineAdaptiveBoxResize)
            {
                SymbolVector params, globals;
                params.push_back(in0());
                params.push_back(_offset());
                Shader_InlineAdaptiveBoxResize = new Shader::Function(
                    "InlineAdaptiveBoxResize", InlineAdaptiveBoxResize_glsl,
                    Shader::Function::MorphologicalFilter, params, globals);
            }

            return Shader_InlineAdaptiveBoxResize;
        }

        Function* blend(int no, const char* funcName, const char* shaderCode)
        {
            assert(no >= 2);
            assert(no <= MAX_TEXTURE_PER_SHADER);

            SymbolVector params, globals;
            for (int i = 0; i < no; ++i)
            {
                ostringstream str;
                str << "i" << i;
                params.push_back(new Symbol(Symbol::ParameterConstIn, str.str(),
                                            Symbol::Vec4fType));
            }

            return new Shader::Function(
                funcName, shaderCode, Shader::Function::Color, params, globals);
        }

        Function* colorQuantize()
        {
            if (!Shader_ColorQuantize)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "partitions", Symbol::FloatType));
                Shader_ColorQuantize = new Shader::Function(
                    "ColorQuantize", ColorQuantize_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorQuantize;
        }

        Function* colorCurve()
        {
            if (!Shader_ColorCurve)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "p1",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "p2",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "p3",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "p4",
                                            Symbol::Vec3fType));
                Shader_ColorCurve = new Shader::Function(
                    "ColorCurve", ColorCurve_glsl, Shader::Function::Color,
                    params, globals);
            }

            return Shader_ColorCurve;
        }

        Function* colorHighlight()
        {
            if (!Shader_ColorHighlight)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "coefficents", Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "highlightPoint",
                                            Symbol::FloatType));
                Shader_ColorHighlight = new Shader::Function(
                    "ColorHighlight", ColorHighlight_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorHighlight;
        }

        Function* colorShadow()
        {
            if (!Shader_ColorShadow)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "coefficents", Symbol::Vec4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "shadowPoint", Symbol::FloatType));
                Shader_ColorShadow = new Shader::Function(
                    "ColorShadow", ColorShadow_glsl, Shader::Function::Color,
                    params, globals);
            }

            return Shader_ColorShadow;
        }

        Function* colorVibrance()
        {
            if (!Shader_ColorVibrance)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "rec",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "saturation", Symbol::FloatType));
                Shader_ColorVibrance = new Shader::Function(
                    "ColorVibrance", ColorVibrance_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorVibrance;
        }

        Function* colorSRGBYCbCr()
        {
            if (!Shader_ColorSRGBYCbCr)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorSRGBYCbCr = new Shader::Function(
                    "ColorSRGBYCbCr", ColorSRGBYCbCr_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorSRGBYCbCr;
        }

        Function* colorYCbCrSRGB()
        {
            if (!Shader_ColorYCbCrSRGB)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorYCbCrSRGB = new Shader::Function(
                    "ColorYCbCrSRGB", ColorYCbCrSRGB_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorYCbCrSRGB;
        }

        Function* colorOutOfRange()
        {
            if (!Shader_ColorOutOfRange)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorOutOfRange = new Shader::Function(
                    "ColorOutOfRange", ColorOutOfRange_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorOutOfRange;
        }

        Function* colorTemperatureOffset()
        {
            if (!Shader_ColorTemperatureOffset)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "color",
                                            Symbol::Vec4fType));

                Shader_ColorTemperatureOffset = new Shader::Function(
                    "ColorTemperatureOffset", ColorTemperatureOffset_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorTemperatureOffset;
        }

        Function* colorBlendWithConstant()
        {
            if (!Shader_ColorBlendWithConstant)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "color",
                                            Symbol::Vec4fType));

                Shader_ColorBlendWithConstant = new Shader::Function(
                    "ColorBlendWithConstant", ColorBlendWithConstant_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorBlendWithConstant;
        }

        Function* colorCDL()
        {
            if (!Shader_ColorCDL)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "scale",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "offset",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "power",
                                            Symbol::Vec3fType));

                Shader_ColorCDL = new Shader::Function(
                    "ColorCDL", ColorCDL_glsl, Shader::Function::Color, params,
                    globals);
            }

            return Shader_ColorCDL;
        }

        Function* colorCDL_SAT()
        {
            if (!Shader_ColorCDL_SAT)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "scale",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "offset",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "power",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "S",
                                            Symbol::Matrix4fType));

                Shader_ColorCDL_SAT = new Shader::Function(
                    "ColorCDL_SAT", ColorCDL_SAT_glsl, Shader::Function::Color,
                    params, globals);
            }

            return Shader_ColorCDL_SAT;
        }

        Function* colorCDL_SAT_noClamp()
        {
            if (!Shader_ColorCDL_SAT_noClamp)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "scale",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "offset",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "power",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "S",
                                            Symbol::Matrix4fType));

                Shader_ColorCDL_SAT_noClamp = new Shader::Function(
                    "ColorCDL_SAT_noClamp", ColorCDL_SAT_noClamp_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorCDL_SAT_noClamp;
        }

        Function* colorCDLForACESLinear()
        {
            if (!Shader_ColorCDLForACESLinear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "scale",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "offset",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "power",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "saturation", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "lumaCoefficients",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "refLow",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "refHigh",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "toACES",
                                            Symbol::Matrix4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "fromACES", Symbol::Matrix4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "minClamp", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "maxClamp", Symbol::FloatType));

                Shader_ColorCDLForACESLinear = new Shader::Function(
                    "ColorCDLForACESLinear", ColorCDLForACESLinear_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorCDLForACESLinear;
        }

        Function* colorCDLForACESLog()
        {
            if (!Shader_ColorCDLForACESLog)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "scale",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "offset",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "power",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "saturation", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "lumaCoefficients",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "refLow",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "refHigh",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "toACES",
                                            Symbol::Matrix4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "fromACES", Symbol::Matrix4fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "minClamp", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "maxClamp", Symbol::FloatType));

                Shader_ColorCDLForACESLog = new Shader::Function(
                    "ColorCDLForACESLog", ColorCDLForACESLog_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorCDLForACESLog;
        }

        Function* color3DLUT()
        {
            if (!Shader_Color3DLUT)
            {
                //
                //  NOTE: we're not using a "special" sampler symbol here (like
                //  RGBA_sampler) because its just a regular parameter. No STs
                //  are needed, etc.
                //

                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "RGB_sampler",
                                            Symbol::Sampler3DType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "sizeMinusOne", Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "inScale",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "inOffset", Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "outScale", Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "outOffset", Symbol::Vec3fType));

                Shader_Color3DLUT = new Shader::Function(
                    "Color3DLUT", Color3DLUT_glsl, Shader::Function::Color,
                    params, globals, 1);
            }

            return Shader_Color3DLUT;
        }

        Function* color3DLUTGLSampling()
        {
            if (!Shader_Color3DLUTGLSampling)
            {
                //
                //  NOTE: we're not using a "special" sampler symbol here (like
                //  RGBA_sampler) because its just a regular parameter. No STs
                //  are needed, etc.
                //

                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "RGB_sampler",
                                            Symbol::Sampler3DType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "inScale",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "inOffset", Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "outScale", Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "outOffset", Symbol::Vec3fType));

                Shader_Color3DLUTGLSampling = new Shader::Function(
                    "Color3DLUTGLSampling", Color3DLUTGLSampling_glsl,
                    Shader::Function::Color, params, globals, 1);
            }

            return Shader_Color3DLUTGLSampling;
        }

        Function* colorChannelLUT()
        {
            if (!Shader_ColorChannelLUT)
            {
                //
                //  NOTE: we're not using a "special" sampler symbol here (like
                //  RGBA_sampler) because its just a regular parameter. No STs
                //  are needed, etc.
                //

                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "RGB_sampler",
                                            Symbol::Sampler2DRectType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "widthMinusOne",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "outScale", Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "outOffset", Symbol::Vec3fType));

                Shader_ColorChannelLUT = new Shader::Function(
                    "ColorChannelLUT", ColorChannelLUT_glsl,
                    Shader::Function::Color, params, globals, 3);
            }

            return Shader_ColorChannelLUT;
        }

        Function* colorLuminanceLUT()
        {
            if (!Shader_ColorLuminanceLUT)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "RGB_sampler",
                                            Symbol::Sampler2DRectType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "widthMinusOne",
                                            Symbol::Vec2fType));

                Shader_ColorLuminanceLUT = new Shader::Function(
                    "ColorLuminanceLUT", ColorLuminanceLUT_glsl,
                    Shader::Function::Color, params, globals, 1);
            }

            return Shader_ColorLuminanceLUT;
        }

        Function* colorLogCToLinear()
        {
            if (!Shader_ColorLogCLinear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "logC_A",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "logC_B",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "logC_C",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "logC_D",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "logC_X",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "logC_Y",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "logC_cutoff", Symbol::FloatType));

                Shader_ColorLogCLinear = new Shader::Function(
                    "ColorLogCLinear", ColorLogCLinear_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorLogCLinear;
        }

        Function* colorLinearToLogC()
        {
            if (!Shader_ColorLinearLogC)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "pbs",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "eo",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "eg",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "gs",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "bo",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "ls",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "lo",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "cutoff",
                                            Symbol::FloatType));

                Shader_ColorLinearLogC = new Shader::Function(
                    "ColorLinearLogC", ColorLinearLogC_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorLinearLogC;
        }

        Function* colorCineonLogToLinear()
        {
            if (!Shader_ColorCineonLogLinear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "cinBlack", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "cinWhiteBlackDiff",
                                            Symbol::FloatType));

                Shader_ColorCineonLogLinear = new Shader::Function(
                    "ColorCineonLogLinear", ColorCineonLogLinear_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorCineonLogLinear;
        }

        Function* colorCineonSoftClipLogToLinear()
        {
            if (!Shader_ColorCineonSoftClipLogLinear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "cinBlack", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "cinWhiteBlackDiff",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "cinSoftClip", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "cinBreakpoint",
                                            Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "cinKneeGain", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "cinKneeOffset",
                                            Symbol::FloatType));

                Shader_ColorCineonSoftClipLogLinear = new Shader::Function(
                    "ColorCineonSoftClipLogLinear",
                    ColorCineonSoftClipLogLinear_glsl, Shader::Function::Color,
                    params, globals);
            }

            return Shader_ColorCineonSoftClipLogLinear;
        }

        Function* colorLinearToCineonLog()
        {
            if (!Shader_ColorLinearCineonLog)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "refBlack", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "refWhite", Symbol::FloatType));

                Shader_ColorLinearCineonLog = new Shader::Function(
                    "ColorLinearCineonLog", ColorLinearCineonLog_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorLinearCineonLog;
        }

        Function* colorViperLogToLinear()
        {
            if (!Shader_ColorViperLogLinear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorViperLogLinear = new Shader::Function(
                    "ColorViperLogLinear", ColorViperLogLinear_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorViperLogLinear;
        }

        Function* colorRedLogToLinear()
        {
            if (!Shader_ColorRedLogLinear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorRedLogLinear = new Shader::Function(
                    "ColorRedLogLinear", ColorRedLogLinear_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorRedLogLinear;
        }

        Function* colorLinearToRedLog()
        {
            if (!Shader_ColorLinearRedLog)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorLinearRedLog = new Shader::Function(
                    "ColorLinearRedLog", ColorLinearRedLog_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorLinearRedLog;
        }

        Function* colorPremult()
        {
            if (!Shader_ColorPremult)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorPremult = new Shader::Function(
                    "ColorPremult", ColorPremult_glsl,
                    Shader::Function::LinearColor, params, globals);
            }

            return Shader_ColorPremult;
        }

        Function* colorPremultLight()
        {
            if (!Shader_ColorPremultLight)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorPremultLight = new Shader::Function(
                    "ColorPremultLight", ColorPremultLight_glsl,
                    Shader::Function::LinearColor, params, globals);
            }

            return Shader_ColorPremultLight;
        }

        Function* colorUnpremult()
        {
            if (!Shader_ColorUnpremult)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorUnpremult = new Shader::Function(
                    "ColorUnpremult", ColorUnpremult_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorUnpremult;
        }

        Function* colorGamma()
        {
            if (!Shader_ColorGamma)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "gamma",
                                            Symbol::Vec3fType));
                Shader_ColorGamma = new Shader::Function(
                    "ColorGamma", ColorGamma_glsl, Shader::Function::Color,
                    params, globals);
            }

            return Shader_ColorGamma;
        }

        Function* colorClamp()
        {
            if (!Shader_ColorClamp)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "minValue", Symbol::FloatType));
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "maxValue", Symbol::FloatType));
                Shader_ColorClamp = new Shader::Function(
                    "ColorClamp", ColorClamp_glsl, Shader::Function::Color,
                    params, globals);
            }

            return Shader_ColorClamp;
        }

        Function* colorMatrix()
        {
            if (!Shader_ColorMatrix)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "M",
                                            Symbol::Matrix4fType));

                Shader_ColorMatrix = new Shader::Function(
                    "ColorMatrix", ColorMatrix_glsl,
                    Shader::Function::LinearColor, params, globals);
            }

            return Shader_ColorMatrix;
        }

        Function* colorMatrix4D()
        {
            if (!Shader_ColorMatrix4D)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "M",
                                            Symbol::Matrix4fType));

                Shader_ColorMatrix4D = new Shader::Function(
                    "ColorMatrix4D", ColorMatrix4D_glsl,
                    Shader::Function::LinearColor, params, globals);
            }

            return Shader_ColorMatrix4D;
        }

        Function* colorLinearToGray()
        {
            if (!Shader_ColorLinearGray)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorLinearGray = new Shader::Function(
                    "ColorLinearGray", ColorLinearGray_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorLinearGray;
        }

        Function* colorSRGBToLinear()
        {
            if (!Shader_ColorSRGBLinear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorSRGBLinear = new Shader::Function(
                    "ColorSRGBLinear", ColorSRGBLinear_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorSRGBLinear;
        }

        Function* colorLinearToSRGB()
        {
            if (!Shader_ColorLinearSRGB)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorLinearSRGB = new Shader::Function(
                    "ColorLinearSRGB", ColorLinearSRGB_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorLinearSRGB;
        }

        Function* ICCLinearToSRGB()
        {
            if (!Shader_ICCLinearSRGB)
            {
                SymbolVector params, globals;
                params.push_back(P());
                params.push_back(new Symbol(Symbol::ParameterConstIn, "a",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "b",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "c",
                                            Symbol::Vec3fType));
                params.push_back(new Symbol(Symbol::ParameterConstIn, "d",
                                            Symbol::Vec3fType));
                Shader_ICCLinearSRGB = new Shader::Function(
                    "ICCLinearSRGB", ICCLinearSRGB_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ICCLinearSRGB;
        }

        Function* colorRec709ToLinear()
        {
            if (!Shader_ColorRec709Linear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorRec709Linear = new Shader::Function(
                    "ColorRec709Linear", ColorRec709Linear_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorRec709Linear;
        }

        Function* colorLinearToRec709()
        {
            if (!Shader_ColorLinearRec709)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorLinearRec709 = new Shader::Function(
                    "ColorLinearRec709", ColorLinearRec709_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorLinearRec709;
        }

        Function* colorSMPTE240MToLinear()
        {
            if (!Shader_ColorSMPTE240MLinear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorSMPTE240MLinear = new Shader::Function(
                    "ColorSMPTE240MLinear", ColorSMPTE240MLinear_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorSMPTE240MLinear;
        }

        Function* colorLinearToSMPTE240M()
        {
            if (!Shader_ColorLinearSMPTE240M)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorLinearSMPTE240M = new Shader::Function(
                    "ColorLinearSMPTE240M", ColorLinearSMPTE240M_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorLinearSMPTE240M;
        }

        Function* colorACESLogToLinear()
        {
            if (!Shader_ColorACESLogLinear)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorACESLogLinear = new Shader::Function(
                    "ColorACESLogLinear", ColorACESLogLinear_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorACESLogLinear;
        }

        Function* colorLinearToACESLog()
        {
            if (!Shader_ColorLinearACESLog)
            {
                SymbolVector params, globals;
                params.push_back(P());
                Shader_ColorLinearACESLog = new Shader::Function(
                    "ColorLinearACESLog", ColorLinearACESLog_glsl,
                    Shader::Function::Color, params, globals);
            }

            return Shader_ColorLinearACESLog;
        }

        Function* sourceY()
        {
            if (!Shader_SourceY)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceY = new Shader::Function("SourceY", SourceY_glsl,
                                                      Shader::Function::Source,
                                                      params, globals);
            }

            return Shader_SourceY;
        }

        Function* sourceYUncrop()
        {
            if (!Shader_SourceYUncrop)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceYUncrop = new Shader::Function(
                    "SourceYUncrop", SourceYUncrop_glsl,
                    Shader::Function::Source, params, globals);
            }

            return Shader_SourceYUncrop;
        }

        Function* sourceYVYU()
        {
            if (!Shader_SourceYVYU)
            {
                SymbolVector params, globals;
                params.push_back(YVYU_sampler());
                params.push_back(ST_coord());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "YUVmatrix", Symbol::Matrix4fType));
                params.push_back(offset());
                params.push_back(samplerSize());

                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceYVYU = new Shader::Function(
                    "SourceYVYU", SourceYVYU_glsl, Shader::Function::Source,
                    params, globals, 2);
            }

            return Shader_SourceYVYU;
        }

        Function* sourceYA()
        {
            if (!Shader_SourceYA)
            {
                SymbolVector params, globals;
                params.push_back(YA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceYA = new Shader::Function(
                    "SourceYA", SourceYA_glsl, Shader::Function::Source, params,
                    globals, 1);
            }

            return Shader_SourceYA;
        }

        Function* sourceYAUncrop()
        {
            if (!Shader_SourceYAUncrop)
            {
                SymbolVector params, globals;
                params.push_back(YA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceYAUncrop = new Shader::Function(
                    "SourceYAUncrop", SourceYAUncrop_glsl,
                    Shader::Function::Source, params, globals, 1);
            }

            return Shader_SourceYAUncrop;
        }

        Function* sourcePlanarYRYBYA()
        {
            if (!Shader_SourcePlanarYRYBYA)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(RY_sampler());
                params.push_back(BY_sampler());
                params.push_back(A_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(ratio1());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "Yweights", Symbol::Vec3fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYRYBYA = new Shader::Function(
                    "SourcePlanarYRYBYA", SourcePlanarYRYBYA_glsl,
                    Shader::Function::Source, params, globals, 4);
            }

            return Shader_SourcePlanarYRYBYA;
        }

        Function* sourcePlanarYRYBYAUncrop()
        {
            if (!Shader_SourcePlanarYRYBYAUncrop)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(RY_sampler());
                params.push_back(BY_sampler());
                params.push_back(A_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(ratio1());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "Yweights", Symbol::Vec3fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYRYBYAUncrop = new Shader::Function(
                    "SourcePlanarYRYBYAUncrop", SourcePlanarYRYBYAUncrop_glsl,
                    Shader::Function::Source, params, globals, 4);
            }

            return Shader_SourcePlanarYRYBYAUncrop;
        }

        Function* sourcePlanarYRYBY()
        {
            if (!Shader_SourcePlanarYRYBY)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(RY_sampler());
                params.push_back(BY_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(ratio1());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "Yweights", Symbol::Vec3fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYRYBY = new Shader::Function(
                    "SourcePlanarYRYBY", SourcePlanarYRYBY_glsl,
                    Shader::Function::Source, params, globals, 3);
            }

            return Shader_SourcePlanarYRYBY;
        }

        Function* sourcePlanarYRYBYUncrop()
        {
            if (!Shader_SourcePlanarYRYBYUncrop)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(RY_sampler());
                params.push_back(BY_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(ratio1());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "Yweights", Symbol::Vec3fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYRYBYUncrop = new Shader::Function(
                    "SourcePlanarYRYBYUncrop", SourcePlanarYRYBYUncrop_glsl,
                    Shader::Function::Source, params, globals, 3);
            }

            return Shader_SourcePlanarYRYBYUncrop;
        }

        Function* sourcePlanarYAC2()
        {
            if (!Shader_SourcePlanarYAC2)
            {
                SymbolVector params, globals;
                params.push_back(YA_sampler());
                params.push_back(RYBY_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "Yweights", Symbol::Vec3fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYAC2 = new Shader::Function(
                    "SourcePlanarYAC2", SourcePlanarYAC2_glsl,
                    Shader::Function::Source, params, globals, 2);
            }

            return Shader_SourcePlanarYAC2;
        }

        Function* sourcePlanarYAC2Uncrop()
        {
            if (!Shader_SourcePlanarYAC2Uncrop)
            {
                SymbolVector params, globals;
                params.push_back(YA_sampler());
                params.push_back(RYBY_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "Yweights", Symbol::Vec3fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYAC2Uncrop = new Shader::Function(
                    "SourcePlanarYAC2Uncrop", SourcePlanarYAC2Uncrop_glsl,
                    Shader::Function::Source, params, globals, 2);
            }

            return Shader_SourcePlanarYAC2Uncrop;
        }

        Function* sourcePlanarYA()
        {
            if (!Shader_SourcePlanarYA)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(A_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYA = new Shader::Function(
                    "SourcePlanarYA", SourcePlanarYA_glsl,
                    Shader::Function::Source, params, globals, 2);
            }

            return Shader_SourcePlanarYA;
        }

        Function* sourcePlanarYAUncrop()
        {
            if (!Shader_SourcePlanarYAUncrop)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(A_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYAUncrop = new Shader::Function(
                    "SourcePlanarYAUncrop", SourcePlanarYAUncrop_glsl,
                    Shader::Function::Source, params, globals, 2);
            }

            return Shader_SourcePlanarYAUncrop;
        }

        Function* sourcePlanarYUVA()
        {
            if (!Shader_SourcePlanarYUVA)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(U_sampler());
                params.push_back(V_sampler());
                params.push_back(A_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(ratio1());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "YUVmatrix", Symbol::Matrix4fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYUVA = new Shader::Function(
                    "SourcePlanarYUVA", SourcePlanarYUVA_glsl,
                    Shader::Function::Source, params, globals, 4);
            }

            return Shader_SourcePlanarYUVA;
        }

        Function* sourceYUVA()
        {
            if (!Shader_SourceYUVA)
            {
                SymbolVector params, globals;
                params.push_back(YUVA_sampler());
                params.push_back(ST_coord());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "YUVmatrix", Symbol::Matrix4fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceYUVA = new Shader::Function(
                    "SourceYUVA", SourceYUVA_glsl, Shader::Function::Source,
                    params, globals, 4);
            }

            return Shader_SourceYUVA;
        }

        Function* sourceAYUV()
        {
            if (!Shader_SourceAYUV)
            {
                SymbolVector params, globals;
                params.push_back(YUVA_sampler());
                params.push_back(ST_coord());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "YUVmatrix", Symbol::Matrix4fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceAYUV = new Shader::Function(
                    "SourceAYUV", SourceAYUV_glsl, Shader::Function::Source,
                    params, globals, 4);
            }

            return Shader_SourceAYUV;
        }

        Function* sourcePlanarYUVAUncrop()
        {
            if (!Shader_SourcePlanarYUVAUncrop)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(U_sampler());
                params.push_back(V_sampler());
                params.push_back(A_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(ratio1());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "YUVmatrix", Symbol::Matrix4fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYUVAUncrop = new Shader::Function(
                    "SourcePlanarYUVAUncrop", SourcePlanarYUVAUncrop_glsl,
                    Shader::Function::Source, params, globals, 4);
            }

            return Shader_SourcePlanarYUVAUncrop;
        }

        Function* sourcePlanarYUV()
        {
            if (!Shader_SourcePlanarYUV)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(U_sampler());
                params.push_back(V_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(ratio1());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "YUVmatrix", Symbol::Matrix4fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYUV = new Shader::Function(
                    "SourcePlanarYUV", SourcePlanarYUV_glsl,
                    Shader::Function::Source, params, globals, 3);
            }

            return Shader_SourcePlanarYUV;
        }

        Function* sourcePlanar2YUV()
        {
            if (!Shader_SourcePlanar2YUV)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(UV_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "YUVmatrix", Symbol::Matrix4fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanar2YUV = new Shader::Function(
                    "SourcePlanar2YUV", SourcePlanar2YUV_glsl,
                    Shader::Function::Source, params, globals, 2);
            }

            return Shader_SourcePlanar2YUV;
        }

        Function* sourcePlanarYUVUncrop()
        {
            if (!Shader_SourcePlanarYUVUncrop)
            {
                SymbolVector params, globals;
                params.push_back(Y_sampler());
                params.push_back(U_sampler());
                params.push_back(V_sampler());
                params.push_back(ST_coord());
                params.push_back(ratio0());
                params.push_back(ratio1());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "YUVmatrix", Symbol::Matrix4fType));
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarYUVUncrop = new Shader::Function(
                    "SourcePlanarYUVUncrop", SourcePlanarYUVUncrop_glsl,
                    Shader::Function::Source, params, globals, 3);
            }

            return Shader_SourcePlanarYUVUncrop;
        }

        Function* sourcePlanarRGB()
        {
            if (!Shader_SourcePlanarRGB)
            {
                SymbolVector params, globals;
                params.push_back(R_sampler());
                params.push_back(G_sampler());
                params.push_back(B_sampler());
                params.push_back(ST_coord());
                // params.push_back(ratio0());
                // params.push_back(ratio1());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarRGB = new Shader::Function(
                    "SourcePlanarRGB", SourcePlanarRGB_glsl,
                    Shader::Function::Source, params, globals, 3);
            }

            return Shader_SourcePlanarRGB;
        }

        Function* sourcePlanarRGBUncrop()
        {
            if (!Shader_SourcePlanarRGBUncrop)
            {
                SymbolVector params, globals;
                params.push_back(R_sampler());
                params.push_back(G_sampler());
                params.push_back(B_sampler());
                params.push_back(ST_coord());
                // params.push_back(ratio0());
                // params.push_back(ratio1());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarRGBUncrop = new Shader::Function(
                    "SourcePlanarRGBUncrop", SourcePlanarRGBUncrop_glsl,
                    Shader::Function::Source, params, globals, 3);
            }

            return Shader_SourcePlanarRGBUncrop;
        }

        Function* sourcePlanarRGBA()
        {
            if (!Shader_SourcePlanarRGBA)
            {
                SymbolVector params, globals;
                params.push_back(R_sampler());
                params.push_back(G_sampler());
                params.push_back(B_sampler());
                params.push_back(A_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarRGBA = new Shader::Function(
                    "SourcePlanarRGBA", SourcePlanarRGBA_glsl,
                    Shader::Function::Source, params, globals, 4);
            }

            return Shader_SourcePlanarRGBA;
        }

        Function* sourcePlanarRGBAUncrop()
        {
            if (!Shader_SourcePlanarRGBAUncrop)
            {
                SymbolVector params, globals;
                params.push_back(R_sampler());
                params.push_back(G_sampler());
                params.push_back(B_sampler());
                params.push_back(A_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourcePlanarRGBAUncrop = new Shader::Function(
                    "SourcePlanarRGBAUncrop", SourcePlanarRGBAUncrop_glsl,
                    Shader::Function::Source, params, globals, 4);
            }

            return Shader_SourcePlanarRGBAUncrop;
        }

        Function* sourceRGBA()
        {
            if (!Shader_SourceRGBA)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceRGBA = new Shader::Function(
                    "SourceRGBA", SourceRGBA_glsl, Shader::Function::Source,
                    params, globals, 1);
            }

            return Shader_SourceRGBA;
        }

        Function* sourceRGBAUncrop()
        {
            if (!Shader_SourceRGBAUncrop)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceRGBAUncrop = new Shader::Function(
                    "SourceRGBAUncrop", SourceRGBAUncrop_glsl,
                    Shader::Function::Source, params, globals, 1);
            }

            return Shader_SourceRGBAUncrop;
        }

        Function* sourceBGRA()
        {
            if (!Shader_SourceBGRA)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceBGRA = new Shader::Function(
                    "SourceBGRA", SourceBGRA_glsl, Shader::Function::Source,
                    params, globals, 1);
            }

            return Shader_SourceBGRA;
        }

        Function* sourceBGRAUncrop()
        {
            if (!Shader_SourceBGRAUncrop)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceBGRAUncrop = new Shader::Function(
                    "SourceBGRAUncrop", SourceBGRAUncrop_glsl,
                    Shader::Function::Source, params, globals, 1);
            }

            return Shader_SourceBGRAUncrop;
        }

        Function* sourceARGB()
        {
            if (!Shader_SourceARGB)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceARGB = new Shader::Function(
                    "SourceARGB", SourceARGB_glsl, Shader::Function::Source,
                    params, globals, 1);
            }

            return Shader_SourceARGB;
        }

        Function* sourceARGBUncrop()
        {
            if (!Shader_SourceARGBUncrop)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceARGBUncrop = new Shader::Function(
                    "SourceARGBUncrop", SourceARGBUncrop_glsl,
                    Shader::Function::Source, params, globals, 1);
            }

            return Shader_SourceARGBUncrop;
        }

        Function* sourceARGBfromBGRA()
        {
            if (!Shader_SourceARGBfromBGRA)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceARGBfromBGRA = new Shader::Function(
                    "SourceARGBfromBGRA", SourceARGBfromBGRA_glsl,
                    Shader::Function::Source, params, globals, 1);
            }

            return Shader_SourceARGBfromBGRA;
        }

        Function* sourceARGBfromBGRAUncrop()
        {
            if (!Shader_SourceARGBfromBGRAUncrop)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceARGBfromBGRAUncrop = new Shader::Function(
                    "SourceARGBfromBGRAUncrop", SourceARGBfromBGRAUncrop_glsl,
                    Shader::Function::Source, params, globals, 1);
            }

            return Shader_SourceARGBfromBGRAUncrop;
        }

        Function* sourceABGR()
        {
            if (!Shader_SourceABGR)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceABGR = new Shader::Function(
                    "SourceABGR", SourceABGR_glsl, Shader::Function::Source,
                    params, globals, 1);
            }

            return Shader_SourceABGR;
        }

        Function* sourceABGRUncrop()
        {
            if (!Shader_SourceABGRUncrop)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceABGRUncrop = new Shader::Function(
                    "SourceABGRUncrop", SourceABGRUncrop_glsl,
                    Shader::Function::Source, params, globals, 1);
            }

            return Shader_SourceABGRUncrop;
        }

        Function* sourceABGRfromBGRA()
        {
            if (!Shader_SourceABGRfromBGRA)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceABGRfromBGRA = new Shader::Function(
                    "SourceABGRfromBGRA", SourceABGRfromBGRA_glsl,
                    Shader::Function::Source, params, globals, 1);
            }

            return Shader_SourceABGRfromBGRA;
        }

        Function* sourceABGRfromBGRAUncrop()
        {
            if (!Shader_SourceABGRfromBGRAUncrop)
            {
                SymbolVector params, globals;
                params.push_back(RGBA_sampler());
                params.push_back(ST_coord());
                params.push_back(offset());
                params.push_back(samplerSize());
                params.push_back(uncropOrigin());
                params.push_back(uncropDimension());
                params.push_back(new Symbol(Symbol::ParameterConstIn,
                                            "orientation", Symbol::FloatType));

                Shader_SourceABGRfromBGRAUncrop = new Shader::Function(
                    "SourceABGRfromBGRAUncrop", SourceABGRfromBGRAUncrop_glsl,
                    Shader::Function::Source, params, globals, 1);
            }

            return Shader_SourceABGRfromBGRAUncrop;
        }

        Function* boxFilter()
        {
            if (!Shader_BoxFilter)
            {
                Shader_BoxFilter = new Shader::Function(
                    "BoxFilter", BoxFilter_glsl, Shader::Function::Filter);
            }

            return Shader_BoxFilter;
        }

        //
        // LensWarpNode shaders
        //
        Function* lensWarpRadial()
        {
            if (!Shader_LensWarpRadial)
            {
                Shader_LensWarpRadial =
                    new Shader::Function("LensWarpRadial", LensWarpRadial_glsl,
                                         Shader::Function::Filter);
            }

            return Shader_LensWarpRadial;
        }

        Function* lensWarpTangential()
        {
            if (!Shader_LensWarpTangential)
            {
                Shader_LensWarpTangential = new Shader::Function(
                    "LensWarpTangential", LensWarpTangential_glsl,
                    Shader::Function::Filter);
            }

            return Shader_LensWarpTangential;
        }

        Function* lensWarpRadialAndTangential()
        {
            if (!Shader_LensWarpRadialAndTangential)
            {
                Shader_LensWarpRadialAndTangential = new Shader::Function(
                    "LensWarpRadialAndTangential",
                    LensWarpRadialAndTangential_glsl, Shader::Function::Filter);
            }

            return Shader_LensWarpRadialAndTangential;
        }

        Function* LensWarp3DE4AnamorphicDegree6()
        {
            if (!Shader_LensWarp3DE4AnamorphicDegree6)
            {
                Shader_LensWarp3DE4AnamorphicDegree6 =
                    new Shader::Function("LensWarp3DE4AnamorphicDegree6",
                                         LensWarp3DE4AnamorphicDegree6_glsl,
                                         Shader::Function::Filter);
            }

            return Shader_LensWarp3DE4AnamorphicDegree6;
        }

        //--

        Expression* newDither(const IPImage* image, Expression* A,
                              size_t displayBits, size_t seed)
        {
            const float scale = float((1 << displayBits) - 1);
            const float fSeed = float(seed % 1000) / 1000.0;
            const Function* F = dither();
            ArgumentVector args(F->parameters().size());

            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], A);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], scale);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], fSeed);
            i++;
            return new Expression(F, args, image);
        }

        Expression* newStencilBox(const IPImage* image, Expression* A)
        {
            const float xmin = image->stencilBox.min.x;
            const float ymin = image->stencilBox.min.y;
            const float xmax = image->stencilBox.max.x;
            const float ymax = image->stencilBox.max.y;

            const Function* F = stencilBox();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], A);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(xmin, xmax));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(ymin, ymax));
            i++;
            return new Expression(F, args, image);
        }

        Expression* newStencilBoxNegAlpha(const IPImage* image, Expression* A)
        {
            const float xmin = image->stencilBox.min.x;
            const float ymin = image->stencilBox.min.y;
            const float xmax = image->stencilBox.max.x;
            const float ymax = image->stencilBox.max.y;

            const Function* F = stencilBoxNegAlpha();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], A);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(xmin, xmax));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(ymin, ymax));
            i++;
            return new Expression(F, args, image);
        }

        Expression* newStereoScanline(const IPImage* image,
                                      const float parityYOffset,
                                      const vector<Expression*>& FA1)
        {
            assert(FA1.size() == 2);
            const Function* F = stereoScanline();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA1[0]);
            i++;
            args[i] = new BoundExpression(F->parameters()[i], FA1[1]);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], parityYOffset);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            return new Expression(F, args, image);
        }

        Expression* newStereoChecker(const IPImage* image,
                                     const float parityXOffset,
                                     const float parityYOffset,
                                     const vector<Expression*>& FA1)
        {
            assert(FA1.size() == 2);
            const Function* F = stereoChecker();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA1[0]);
            i++;
            args[i] = new BoundExpression(F->parameters()[i], FA1[1]);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], parityXOffset);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], parityYOffset);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            return new Expression(F, args, image);
        }

        Expression* newStereoAnaglyph(const IPImage* image,
                                      const vector<Expression*>& FA1)
        {
            assert(FA1.size() == 2);
            const Function* F = stereoAnaglyph();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA1[0]);
            i++;
            args[i] = new BoundExpression(F->parameters()[i], FA1[1]);
            i++;
            return new Expression(F, args, image);
        }

        Expression* newStereoLumAnaglyph(const IPImage* image,
                                         const vector<Expression*>& FA1)
        {
            assert(FA1.size() == 2);

            Mat44f M = TwkMath::Rec709FullRangeRGBToYUV8<float>();
            const Vec3f lumaCoefficients(M.m00, M.m01, M.m02);

            const Function* F = stereoLumAnaglyph();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA1[0]);
            i++;
            args[i] = new BoundExpression(F->parameters()[i], FA1[1]);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], lumaCoefficients);
            i++;
            return new Expression(F, args, image);
        }

        Expression* newInlineBoxResize(bool adaptive, const IPImage* image,
                                       Expression* A, const Vec2f& scale)
        {
            const Function* F =
                adaptive ? inlineAdaptiveBoxResize() : inlineBoxResize();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            assert(A);
            args[i] = new BoundExpression(F->parameters()[i], A);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            if (!adaptive)
                args[i] =
                    new BoundVec2f(F->parameters()[i], Vec2f(1.0f) / scale);
            i++;
            return new Expression(F, args, image);
        }

        Expression* newSimpleBoxFilter(const IPImage* image, Expression* A,
                                       const float size)
        {
            const Function* F = boxFilter();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], A);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], size);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            return new Expression(F, args, image);
        }

        Expression* newLensWarp(const IPImage* image, Expression* FA, float k1,
                                float k2, float k3, float d, float p1, float p2,
                                const Vec2f& center, const Vec2f& f,
                                const Vec2f& cropRatio)
        {
            bool doRadial = (k1 != 0.0f) || (k2 != 0.0f) || (k3 != 0.0f);
            bool doTangential = (p1 != 0.0f) || (p2 != 0.0f);
            Vec2f fInPixels =
                Vec2f((float)std::max(image->width, image->height)) * f
                * cropRatio;
            assert(FA);

            // The convention for center property was specified in terms on
            // TOPLEFT framebuffer, so we adjust accordingly since our fb is now
            // internally BOTTOMLEFT after the source shaders.
            Vec2f offsetInPixels =
                Vec2f((float)image->width, (float)image->height)
                * Vec2f(center.x, 1.0f - center.y);

            if (doRadial)
            {
                if (doTangential)
                {
                    const Function* F = lensWarpRadialAndTangential();
                    ArgumentVector args(F->parameters().size());
                    size_t i = 0;
                    args[i] = new BoundExpression(F->parameters()[i], FA);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], k1);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], k2);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], k3);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], d);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], p1);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], p2);
                    i++;
                    args[i] =
                        new BoundVec2f(F->parameters()[i], offsetInPixels);
                    i++;
                    args[i] = new BoundVec2f(F->parameters()[i], fInPixels);
                    i++;
                    args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
                    i++;
                    return new Expression(F, args, image);
                }
                else
                {
                    const Function* F = lensWarpRadial();
                    ArgumentVector args(F->parameters().size());
                    size_t i = 0;
                    args[i] = new BoundExpression(F->parameters()[i], FA);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], k1);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], k2);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], k3);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], d);
                    i++;
                    args[i] =
                        new BoundVec2f(F->parameters()[i], offsetInPixels);
                    i++;
                    args[i] = new BoundVec2f(F->parameters()[i], fInPixels);
                    i++;
                    args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
                    i++;
                    return new Expression(F, args, image);
                }
            }
            else
            {
                if (doTangential)
                {
                    const Function* F = lensWarpTangential();
                    ArgumentVector args(F->parameters().size());
                    size_t i = 0;
                    args[i] = new BoundExpression(F->parameters()[i], FA);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], p1);
                    i++;
                    args[i] = new BoundFloat(F->parameters()[i], p2);
                    i++;
                    args[i] =
                        new BoundVec2f(F->parameters()[i], offsetInPixels);
                    i++;
                    args[i] = new BoundVec2f(F->parameters()[i], fInPixels);
                    i++;
                    args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
                    i++;
                    return new Expression(F, args, image);
                }
                else
                {
                    return FA;
                }
            }
        }

        Expression* newLensWarp3DE4AnamorphicDegree6(
            const IPImage* image, Expression* FA, const Vec2f& c02,
            const Vec2f& c22, const Vec2f& c04, const Vec2f& c24,
            const Vec2f& c44, const Vec2f& c06, const Vec2f& c26,
            const Vec2f& c46, const Vec2f& c66, const Vec2f& center,
            const Vec2f& f, const Vec2f& cropRatio)
        {
            bool doDistort = (c02 != Vec2f(0.0f)) || (c22 != Vec2f(0.0f))
                             || (c04 != Vec2f(0.0f)) || (c24 != Vec2f(0.0f))
                             || (c44 != Vec2f(0.0f)) || (c06 != Vec2f(0.0f))
                             || (c26 != Vec2f(0.0f)) || (c46 != Vec2f(0.0f))
                             || (c66 != Vec2f(0.0f));

            assert(FA);

            if (doDistort)
            {
                Vec2f offsetInPixels =
                    Vec2f((float)image->width, (float)image->height) * center;
                Vec2f fInPixels =
                    Vec2f((float)0.5f
                          * sqrtf(image->width * image->width
                                  + image->height * image->height))
                    * f * cropRatio;
                const Function* F = LensWarp3DE4AnamorphicDegree6();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], c02);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], c22);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], c04);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], c24);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], c44);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], c06);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], c26);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], c46);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], c66);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], offsetInPixels);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], fInPixels);
                i++;
                args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
                i++;
                return new Expression(F, args, image);
            }
            return FA;
        }

        const char* overShaders[] = {Over2_glsl, Over3_glsl, Over4_glsl};
        const char* replaceShaders[] = {Replace2_glsl, Replace3_glsl,
                                        Replace4_glsl};
        const char* addShaders[] = {Add2_glsl, Add3_glsl, Add4_glsl};
        const char* differenceShaders[] = {Difference2_glsl, Difference3_glsl,
                                           Difference4_glsl};
        const char* reverseDifferenceShaders[] = {ReverseDifference2_glsl,
                                                  ReverseDifference3_glsl,
                                                  ReverseDifference4_glsl};

        // generate a blend expr of a certain mode
        // with the input Expressions as input to the blend shaders (over, add,
        // etc.) the input Expressions are held in FA1, with starting position
        // as 'startPos', and number of Expressions as 'size'
        Expression* newBlend(const IPImage* image,
                             const vector<Expression*>& FA1,
                             const IPImage::BlendMode mode)
        {
            int size = FA1.size();
            assert(size <= MAX_TEXTURE_PER_SHADER);
            if (size == 1)
            {
                return FA1[0];
            }

            // at least 2 inputs
            Function* F = NULL;
            const char* name = "main";

            switch (mode)
            {
            case IPImage::Add:
                F = blend(size, name,
                          addShaders[size - 2]); //*2_glsl is at position 0
                break;
            case IPImage::Difference:
                F = blend(
                    size, name,
                    differenceShaders[size - 2]); //*2_glsl is at position 0
                break;
            case IPImage::ReverseDifference:
                F = blend(size, name,
                          reverseDifferenceShaders[size - 2]); //*2_glsl is at
                                                               // position 0
                break;
            case IPImage::Replace:
                F = blend(size, name,
                          replaceShaders[size - 2]); //*2_glsl is at position 0
                break;
            default: /*case IPImage::Over:*/
                F = blend(size, name,
                          overShaders[size - 2]); //*2_glsl is at position 0
                break;
                // TODO: dissolve
            }

            ArgumentVector args(F->parameters().size());

            for (size_t i = 0; i < size; ++i)
            {
                args[i] = new BoundExpression(F->parameters()[i], FA1[i]);
            }

            return new Expression(F, args, image);
        }

        Expression* newHistogram(const IPImage* image,
                                 const std::vector<Expression*>& FA1)
        {
            const Function* F = histogram();
            ArgumentVector args(F->parameters().size());
            int size = FA1.size();
            assert(size == 1);
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA1[0]);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            return new Expression(F, args, image);
        }

        // fb carries precomputed weights
        Expression* newFilterGaussianVerticalFast(Expression* FA,
                                                  const TwkFB::FrameBuffer* fb,
                                                  const float radius,
                                                  const float sigma)
        {
            const Function* F = filterGaussianVerticalFast();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(fb, 0));
            i++;
            args[i] = new BoundFloat(F->parameters()[i], radius);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newFilterGaussianVertical(Expression* FA,
                                              const float radius,
                                              const float sigma)
        {
            const Function* F = filterGaussianVertical();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], radius);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], sigma);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            return new Expression(F, args, FA->image());
        }

        // fb carries precomputed weights
        Expression*
        newFilterGaussianHorizontalFast(Expression* FA,
                                        const TwkFB::FrameBuffer* fb,
                                        const float radius, const float sigma)
        {
            const Function* F = filterGaussianHorizontalFast();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(fb, 0));
            i++;
            args[i] = new BoundFloat(F->parameters()[i], radius);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newFilterGaussianHorizontal(Expression* FA,
                                                const float radius,
                                                const float sigma)
        {
            const Function* F = filterGaussianHorizontal();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], radius);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], sigma);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newFilterUnsharpMask(const IPImage* image,
                                         const std::vector<Expression*>& FA1,
                                         const float amount,
                                         const float threshold)
        {
            assert(FA1.size() == 2);
            const Function* F = unsharpMask();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA1[0]);
            i++;
            args[i] = new BoundExpression(F->parameters()[i], FA1[1]);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], amount);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], threshold);
            i++;
            return new Expression(F, args, image);
        }

        Expression* newFilterNoiseReduction(const IPImage* image,
                                            const std::vector<Expression*>& FA1,
                                            const float amount,
                                            const float threshold)
        {
            assert(FA1.size() == 2);
            const Function* F = noiseReduction();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA1[0]);
            i++;
            args[i] = new BoundExpression(F->parameters()[i], FA1[1]);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], amount);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], threshold);
            i++;
            return new Expression(F, args, image);
        }

        Expression* newFilterClarity(const IPImage* image,
                                     const std::vector<Expression*>& FA1,
                                     const float amount)
        {
            assert(FA1.size() == 2);
            const Function* F = clarity();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA1[0]);
            i++;
            args[i] = new BoundExpression(F->parameters()[i], FA1[1]);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], amount);
            i++;
            return new Expression(F, args, image);
        }

        Expression* newColorQuantize(Expression* FA, int partitions)
        {
            const Function* F = colorQuantize();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], float(partitions));
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorLineartoGray(Expression* FA)
        {
            const Function* F = colorLinearToGray();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            return new Expression(F, args, FA->image());
        }

        // p1 - > p4 is monotonically increasing in x, y
        Expression* newColorCurveonY(Expression* FA, const TwkMath::Vec3f& p1,
                                     const TwkMath::Vec3f& p2,
                                     const TwkMath::Vec3f& p3,
                                     const TwkMath::Vec3f& p4)
        {
            const Function* F = colorCurve();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], p1);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], p2);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], p3);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], p4);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorHighlightonY(Expression* FA,
                                         const TwkMath::Vec4f& coeff,
                                         const float highlight)
        {
            const Function* F = colorHighlight();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundVec4f(F->parameters()[i], coeff);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], highlight);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorShadowonY(Expression* FA,
                                      const TwkMath::Vec4f& coeff,
                                      const float shadow)
        {
            const Function* F = colorShadow();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundVec4f(F->parameters()[i], coeff);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], shadow);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorVibrance(Expression* FA,
                                     const TwkMath::Vec3f& rgb709,
                                     const float saturation)
        {
            const Function* F = colorVibrance();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], rgb709);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], saturation);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorSRGBYCbCr(Expression* FA)
        {
            if (FA->function() == colorYCbCrSRGB())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }

            const Function* F = colorSRGBYCbCr();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            return new Expression(F, args, FA->image());
        }

        Expression* newColorYCbCrSRGB(Expression* FA)
        {
            if (FA->function() == colorSRGBYCbCr())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }

            const Function* F = colorYCbCrSRGB();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            return new Expression(F, args, FA->image());
        }

        Expression* newColorViperLogToLinear(Expression* FA)
        {
            const Function* F = colorViperLogToLinear();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            return new Expression(F, args, FA->image());
        }

        Expression* newColorRedLogToLinear(Expression* FA)
        {
            const Function* F = colorRedLogToLinear();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            return new Expression(F, args, FA->image());
        }

        Expression* newColorLinearToRedLog(Expression* FA)
        {
            const Function* F = colorLinearToRedLog();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            return new Expression(F, args, FA->image());
        }

        Expression* newColorLogCLinear(Expression* FA, float pbs, float eo,
                                       float eg, float gs, float bo, float ls,
                                       float lo, float cutoff)
        {
            const float A = 1.0f / eg;
            const float B = -eo / eg;
            const float C = gs;
            const float D = pbs - bo * gs;
            const float X = gs / (eg * ls);
            const float Y = -(eo + eg * (ls * (bo - pbs / gs) + lo)) * X;

            const Function* F = colorLogCToLinear();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], A);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], B);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], C);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], D);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], X);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], Y);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], cutoff);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorLinearLogC(Expression* FA, float pbs, float eo,
                                       float eg, float gs, float bo, float ls,
                                       float lo, float cutoff)
        {
            const Function* F = colorLinearToLogC();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], pbs);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], eo);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], eg);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], gs);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], bo);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], ls);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], lo);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], cutoff);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorCineonLogToLinear(Expression* FA, double refBlack,
                                              double refWhite, double softClip)
        {
            const double tf = 0.002 / 0.6;

            // Implies the parameters are passed in normalized.
            bool isNormalized = (refWhite >= 0.0 && refWhite <= 1.0);

            double black;
            double white;
            if (isNormalized)
            {
                black = pow(10.0, refBlack * 1023.0 * tf);
                white = pow(10.0, refWhite * 1023.0 * tf);
            }
            else
            {
                black = pow(10.0, refBlack * tf);
                white = pow(10.0, refWhite * tf);
            }

            double whiteBlackDiff = white - black;

            if (softClip <= 0.0)
            {
                const Function* F = colorCineonLogToLinear();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                args[i] = new BoundFloat(F->parameters()[i], black);
                i++;
                args[i] = new BoundFloat(F->parameters()[i], whiteBlackDiff);
                i++;
                return new Expression(F, args, FA->image());
            }
            else
            {
                double breakpoint;
                if (isNormalized)
                {
                    breakpoint = refWhite - softClip;
                    softClip = softClip * 10.23;
                }
                else
                {
                    breakpoint = (refWhite - softClip) / 1023.0;
                    softClip = softClip / 100.0;
                }

                double kneeOffset =
                    (pow(10.0, breakpoint * 1023.0f * tf) - black)
                    / whiteBlackDiff;
                double kneeGain =
                    (1.0 - kneeOffset) / pow(500 * softClip, softClip);

                const Function* F = colorCineonSoftClipLogToLinear();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                args[i] = new BoundFloat(F->parameters()[i], black);
                i++;
                args[i] = new BoundFloat(F->parameters()[i], whiteBlackDiff);
                i++;
                args[i] = new BoundFloat(F->parameters()[i], softClip);
                i++;
                args[i] = new BoundFloat(F->parameters()[i], breakpoint);
                i++;
                args[i] = new BoundFloat(F->parameters()[i], kneeGain);
                i++;
                args[i] = new BoundFloat(F->parameters()[i], kneeOffset);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorLinearToCineonLog(Expression* FA, double refBlack,
                                              double refWhite)
        {
            const Function* F = colorLinearToCineonLog();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], refBlack);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], refWhite);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorClamp(Expression* FA, float minValue,
                                  float maxValue)
        {
            const Function* F = colorClamp();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], minValue);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], maxValue);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorGamma(Expression* FA, const Vec3f& v)
        {
            //
            //  Early exit if v would result in identity function
            //

            if (v == Vec3f(1, 1, 1))
                return FA;

            if (FA->function() == colorGamma())
            {
                Vec3f g =
                    static_cast<const BoundVec3f*>(FA->arguments()[1])->value()
                    * v;

                //
                //  Make copy of the incoming expression argument
                //

                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();

                delete FA;

                if (g == Vec3f(1.0f))
                {
                    // elide gamma completely if its 1.0 after concatenation
                    return fexpr;
                }
                else
                {
                    return newColorGamma(fexpr, g);
                }
            }
            else
            {
                const Function* F = colorGamma();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                args[i] = new BoundVec3f(F->parameters()[i], v);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorMatrix(Expression* FA, const Mat44f& M)
        {
            if (M == Mat44f())
                return FA;

            if (FA->function() == colorMatrix())
            {
                //
                //  If the existing argument is a matrix then concatenate
                //  them together.
                //

                //
                //  Get the existing Matrix and multiply it by incoming
                //

                Mat44f C = M
                           * static_cast<const BoundMat44f*>(FA->arguments()[1])
                                 ->value();

                //
                //  Make copy of the incoming expression argument
                //

                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();

                delete FA;

                if (C == Mat44f())
                {
                    //
                    //  Unlikely to happen numerically but go for it anyway
                    //

                    return fexpr;
                }
                else
                {
                    //
                    //  Apply a matrix with concatenated value.
                    //

                    return newColorMatrix(fexpr, C);
                }
            }
            else
            {
                const Function* F = colorMatrix();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                args[i] = new BoundMat44f(F->parameters()[i], M);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorMatrix4D(Expression* FA, const Mat44f& M)
        {
            if (M == Mat44f())
                return FA;

            if (FA->function() == colorMatrix4D())
            {
                //
                //  If the existing argument is a matrix then concatenate
                //  them together.
                //

                //
                //  Get the existing Matrix and multiply it by incoming
                //

                Mat44f C = M
                           * static_cast<const BoundMat44f*>(FA->arguments()[1])
                                 ->value();

                //
                //  Make copy of the incoming expression argument
                //

                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();

                delete FA;

                if (C == Mat44f())
                {
                    //
                    //  Unlikely to happen numerically but go for it anyway
                    //

                    return fexpr;
                }
                else
                {
                    //
                    //  Apply a matrix with concatenated value.
                    //

                    return newColorMatrix4D(fexpr, C);
                }
            }
            else
            {
                const Function* F = colorMatrix4D();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                args[i] = new BoundMat44f(F->parameters()[i], M);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorSRGBToLinear(Expression* FA)
        {
            if (FA->function() == colorLinearToSRGB())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorSRGBToLinear();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorLinearToSRGB(Expression* FA)
        {
            if (FA->function() == colorSRGBToLinear())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorLinearToSRGB();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression*
        newICCLinearToSRGB(Expression* FA, const TwkMath::Vec3f& gamma,
                           const TwkMath::Vec3f& a, const TwkMath::Vec3f& b,
                           const TwkMath::Vec3f& c, const TwkMath::Vec3f& d)
        {
            const Function* F = ICCLinearToSRGB();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], gamma);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], a);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], b);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], c);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], d);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorLinearToRec709(Expression* FA)
        {
            if (FA->function() == colorRec709ToLinear())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorLinearToRec709();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorLinearToSMPTE240M(Expression* FA)
        {
            if (FA->function() == colorSMPTE240MToLinear())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorLinearToSMPTE240M();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorLinearToACESLog(Expression* FA)
        {
            if (FA->function() == colorACESLogToLinear())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorLinearToACESLog();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorOutOfRange(Expression* FA)
        {
            const Function* F = colorOutOfRange();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorTemperatureOffset(Expression* FA,
                                              const Vec4f& color)
        {
            const Function* F = colorTemperatureOffset();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundVec4f(F->parameters()[i], color);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorBlendWithConstant(Expression* FA,
                                              const Vec4f& color)
        {
            const Function* F = colorBlendWithConstant();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundVec4f(F->parameters()[i], color);
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorPremult(Expression* FA)
        {
            if (FA->function() == colorUnpremult())
            {
                //
                //  If the incoming expression is unpremult just return its
                //  argument since adding a premult here is basically a no-op
                //

                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorPremult();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorPremultLight(Expression* FA)
        {
            if (FA->function() == colorUnpremult())
            {
                //
                //  If the incoming expression is unpremult just return its
                //  argument since adding a premult here is basically a no-op
                //

                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorPremultLight();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorUnpremult(Expression* FA)
        {
            if (FA->function() == colorPremultLight())
            {
                //
                //  If the incoming expression is PremultLight just return its
                //  argument since adding an unpremult here is basically a
                //  no-op
                //

                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorUnpremult();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorRec709ToLinear(Expression* FA)
        {
            if (FA->function() == colorLinearToRec709())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorRec709ToLinear();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorSMPTE240MToLinear(Expression* FA)
        {
            if (FA->function() == colorLinearToSMPTE240M())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorSMPTE240MToLinear();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        Expression* newColorACESLogToLinear(Expression* FA)
        {
            if (FA->function() == colorLinearToACESLog())
            {
                Expression* fexpr =
                    static_cast<const BoundExpression*>(FA->arguments()[0])
                        ->value()
                        ->copy();
                delete FA;
                return fexpr;
            }
            else
            {
                const Function* F = colorACESLogToLinear();
                ArgumentVector args(F->parameters().size());
                size_t i = 0;
                args[i] = new BoundExpression(F->parameters()[i], FA);
                i++;
                return new Expression(F, args, FA->image());
            }
        }

        //
        //  Uncrop offset X, Y are always in a BOTTOMLEFT coordinate frame, but
        //  we are about to use them in the native coord frame of the FB, so
        //  convert to that frame before we use them.
        //

        static int nativeUncropX(const FrameBuffer* fb)
        {
            return fb->uncropX();
        }

        static int nativeUncropY(const FrameBuffer* fb)
        {
            return fb->uncropY();
        }

        static float computeOrientation(const FrameBuffer* fb)
        {
            float orientation = 0.0;
            if (fb)
            {
                if (fb->orientation() == FrameBuffer::TOPLEFT)
                {
                    orientation = 1.0;
                }
                else if (fb->orientation() == FrameBuffer::BOTTOMRIGHT)
                {
                    orientation = 2.0;
                }
                else if (fb->orientation() == FrameBuffer::TOPRIGHT)
                {
                    orientation = 3.0;
                }
            }
            return orientation;
        }

        Expression* newSourcePlanarYUVA(const IPImage* img, const Mat44f& M)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYUVA();
            const FrameBuffer* U = fb->nextPlane();
            const FrameBuffer* V = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(U->width()) / float(fb->width()),
                                   float(U->height()) / float(fb->height()));

            const Vec2f r1 = Vec2f(float(V->width()) / float(fb->width()),
                                   float(V->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 3));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r1);
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], M);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYUVAUncrop(const IPImage* img,
                                              const Mat44f& M)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYUVAUncrop();
            const FrameBuffer* U = fb->nextPlane();
            const FrameBuffer* V = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(U->width()) / float(fb->width()),
                                   float(U->height()) / float(fb->height()));

            const Vec2f r1 = Vec2f(float(V->width()) / float(fb->width()),
                                   float(V->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 3));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r1);
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], M);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;
            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYUV(const IPImage* img, const Mat44f& M)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYUV();
            const FrameBuffer* U = fb->nextPlane();
            const FrameBuffer* V = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(U->width()) / float(fb->width()),
                                   float(U->height()) / float(fb->height()));

            const Vec2f r1 = Vec2f(float(V->width()) / float(fb->width()),
                                   float(V->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r1);
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], M);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;
            return new Expression(F, args, img);
        }

        Expression* newSourceYVYU(const IPImage* img, const Mat44f& M)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourceYVYU();

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], M);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;
            return new Expression(F, args, img);
        }

        Expression* newSourcePlanar2YUV(const IPImage* img, const Mat44f& M)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanar2YUV();
            const FrameBuffer* UV = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(UV->width()) / float(fb->width()),
                                   float(UV->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], M);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYUVUncrop(const IPImage* img,
                                             const Mat44f& M)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYUVUncrop();
            const FrameBuffer* U = fb->nextPlane();
            const FrameBuffer* V = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(U->width()) / float(fb->width()),
                                   float(U->height()) / float(fb->height()));

            const Vec2f r1 = Vec2f(float(V->width()) / float(fb->width()),
                                   float(V->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r1);
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], M);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYRYBY(const IPImage* img, const Vec3f& w)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYRYBY();
            const FrameBuffer* RY = fb->nextPlane();
            const FrameBuffer* BY = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(RY->width()) / float(fb->width()),
                                   float(RY->height()) / float(fb->height()));

            const Vec2f r1 = Vec2f(float(BY->width()) / float(fb->width()),
                                   float(BY->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r1);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], w);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYRYBYUncrop(const IPImage* img,
                                               const Vec3f& w)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYRYBYUncrop();
            const FrameBuffer* RY = fb->nextPlane();
            const FrameBuffer* BY = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(RY->width()) / float(fb->width()),
                                   float(RY->height()) / float(fb->height()));

            const Vec2f r1 = Vec2f(float(BY->width()) / float(fb->width()),
                                   float(BY->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r1);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], w);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYAC2(const IPImage* img, const Vec3f& w)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYAC2();
            const FrameBuffer* C2 = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(C2->width()) / float(fb->width()),
                                   float(C2->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], w);
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYAC2Uncrop(const IPImage* img,
                                              const Vec3f& w)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYAC2Uncrop();
            const FrameBuffer* C2 = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(C2->width()) / float(fb->width()),
                                   float(C2->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], w);
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceYUVA(const IPImage* img, const Mat44f& M)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourceYUVA();

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], M);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceAYUV(const IPImage* img, const Mat44f& M)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourceAYUV();

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], M);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYA(const IPImage* img)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYA();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYAUncrop(const IPImage* img)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYAUncrop();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYRYBYA(const IPImage* img, const Vec3f& w)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYRYBYA();
            const FrameBuffer* RY = fb->nextPlane();
            const FrameBuffer* BY = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(RY->width()) / float(fb->width()),
                                   float(RY->height()) / float(fb->height()));

            const Vec2f r1 = Vec2f(float(BY->width()) / float(fb->width()),
                                   float(BY->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 3));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r1);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], w);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarYRYBYAUncrop(const IPImage* img,
                                                const Vec3f& w)
        {
            const FrameBuffer* fb = img->fb;
            const Function* F = sourcePlanarYRYBYAUncrop();
            const FrameBuffer* RY = fb->nextPlane();
            const FrameBuffer* BY = fb->nextPlane();

            const Vec2f r0 = Vec2f(float(RY->width()) / float(fb->width()),
                                   float(RY->height()) / float(fb->height()));

            const Vec2f r1 = Vec2f(float(BY->width()) / float(fb->width()),
                                   float(BY->height()) / float(fb->height()));

            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 3));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r0);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], r1);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], w);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarRGB(const IPImage* img)
        {
            const Function* F = sourcePlanarRGB();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarRGBUncrop(const IPImage* img)
        {
            const Function* F = sourcePlanarRGBUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarRGBA(const IPImage* img)
        {
            const Function* F = sourcePlanarRGBA();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 3));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourcePlanarRGBAUncrop(const IPImage* img)
        {
            const Function* F = sourcePlanarRGBAUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 1));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 2));
            i++;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 3));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceY(const IPImage* img)
        {
            const Function* F = sourceY();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceYUncrop(const IPImage* img)
        {
            const Function* F = sourceYUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceYA(const IPImage* img)
        {
            const Function* F = sourceYA();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceYAUncrop(const IPImage* img)
        {
            const Function* F = sourceYAUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceRGBA(const IPImage* img)
        {
            const Function* F = sourceRGBA();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;

            if (fb)
            {
                args[i] =
                    new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
                i++;
            }
            else
            {
                args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(img));
                i++;
            }

            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            if (fb)
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(fb->width(), fb->height()));
                i++;
            }
            else
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(img->width, img->height));
                i++;
            }

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceRGBAUncrop(const IPImage* img)
        {
            const Function* F = sourceRGBAUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceBGRA(const IPImage* img)
        {
            const Function* F = sourceBGRA();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;

            if (fb)
            {
                args[i] =
                    new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
                i++;
            }
            else
            {
                args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(img));
                i++;
            }

            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            if (fb)
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(fb->width(), fb->height()));
                i++;
            }
            else
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(img->width, img->height));
                i++;
            }

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceBGRAUncrop(const IPImage* img)
        {
            const Function* F = sourceBGRAUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceARGB(const IPImage* img)
        {
            const Function* F = sourceARGB();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;

            if (fb)
            {
                args[i] =
                    new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
                i++;
            }
            else
            {
                args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(img));
                i++;
            }

            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            if (fb)
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(fb->width(), fb->height()));
                i++;
            }
            else
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(img->width, img->height));
                i++;
            }

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceARGBUncrop(const IPImage* img)
        {
            const Function* F = sourceARGBUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceARGBfromBGRA(const IPImage* img)
        {
            const Function* F = sourceARGBfromBGRA();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;

            if (fb)
            {
                args[i] =
                    new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
                i++;
            }
            else
            {
                args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(img));
                i++;
            }

            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            if (fb)
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(fb->width(), fb->height()));
                i++;
            }
            else
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(img->width, img->height));
                i++;
            }

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceARGBfromBGRAUncrop(const IPImage* img)
        {
            const Function* F = sourceARGBfromBGRAUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceABGR(const IPImage* img)
        {
            const Function* F = sourceABGR();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;

            if (fb)
            {
                args[i] =
                    new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
                i++;
            }
            else
            {
                args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(img));
                i++;
            }

            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            if (fb)
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(fb->width(), fb->height()));
                i++;
            }
            else
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(img->width, img->height));
                i++;
            }

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceABGRUncrop(const IPImage* img)
        {
            const Function* F = sourceABGRUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceABGRfromBGRA(const IPImage* img)
        {
            const Function* F = sourceABGRfromBGRA();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;

            if (fb)
            {
                args[i] =
                    new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
                i++;
            }
            else
            {
                args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(img));
                i++;
            }

            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            if (fb)
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(fb->width(), fb->height()));
                i++;
            }
            else
            {
                args[i] = new BoundVec2f(F->parameters()[i],
                                         Vec2f(img->width, img->height));
                i++;
            }

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newSourceABGRfromBGRAUncrop(const IPImage* img)
        {
            const Function* F = sourceABGRfromBGRAUncrop();
            const FrameBuffer* fb = img->fb;
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] =
                new BoundSampler(F->parameters()[i], ImageOrFB(img, fb, 0));
            i++;
            args[i] = new BoundImageCoord(F->parameters()[i], ImageCoord(img));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;
            args[i] = new BoundVec2f(F->parameters()[i],
                                     Vec2f(fb->width(), fb->height()));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(nativeUncropX(fb), nativeUncropY(fb)));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i],
                               Vec2f(fb->uncropWidth(), fb->uncropHeight()));
            i++;

            float orientation = computeOrientation(fb);
            args[i] = new BoundFloat(F->parameters()[i], orientation);
            i++;

            return new Expression(F, args, img);
        }

        Expression* newColor3DLUT(Expression* FA, const TwkFB::FrameBuffer* fb,
                                  const Vec3f& outScale, const Vec3f& outOffset)
        {
            const Function* F = color3DLUT();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;

            const int w = fb->width();
            const int h = fb->height();
            const int d = fb->depth();
            const Vec3f sizeMinusOne = Vec3f(w - 1, h - 1, d - 1);
            const Vec3f grid = Vec3f(1.0 / w, 1.0 / h, 1.0 / d);
            const Vec3f rs = Vec3f(1.0f) / sizeMinusOne;
            const Vec3f inScale = (Vec3f(1.0) - grid) * rs;
            const Vec3f inOffset = Vec3f(0.5) * grid;

            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(fb, 0));
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], sizeMinusOne);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], inScale);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], inOffset);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], outScale);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], outOffset);
            i++;

            return new Expression(F, args, FA->image());
        }

        Expression*
        newColor3DLUTGLSampling(Expression* FA, const TwkFB::FrameBuffer* fb,
                                const Vec3f& inScale, const Vec3f& inOffset,
                                const Vec3f& outScale, const Vec3f& outOffset)
        {
            const Function* F = color3DLUTGLSampling();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(fb, 0));
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], inScale);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], inOffset);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], outScale);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], outOffset);
            i++;

            return new Expression(F, args, FA->image());
        }

        Expression* newColorChannelLUT(Expression* FA,
                                       const TwkFB::FrameBuffer* fb,
                                       const Vec3f& outScale,
                                       const Vec3f& outOffset)
        {
            const Function* F = colorChannelLUT();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(fb, 0));
            i++;
            args[i] =
                new BoundVec3f(F->parameters()[i], Vec3f(fb->width() - 1.0f));
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], outScale);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], outOffset);
            i++;

            return new Expression(F, args, FA->image());
        }

        Expression* newColorLuminanceLUT(Expression* FA,
                                         const TwkFB::FrameBuffer* fb)
        {
            const Function* F = colorLuminanceLUT();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundSampler(F->parameters()[i], ImageOrFB(fb, 0));
            i++;
            args[i] =
                new BoundVec2f(F->parameters()[i], Vec2f(fb->width() - 1.0f));
            i++;
            return new Expression(F, args, FA->image());
        }

        Expression* newColorCDL(Expression* FA, const Vec3f& scale,
                                const Vec3f& offset, const Vec3f& power,
                                float s, bool noClamp)
        {
            bool useSAT = s != 1.0f || noClamp;

            const Function* F = noClamp
                                    ? colorCDL_SAT_noClamp()
                                    : (useSAT ? colorCDL_SAT() : colorCDL());
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], scale);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], offset);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], power);
            i++;

            if (useSAT)
            {
                //
                //  NOTE: saturation matrix is using Rec709 weightings not the
                //  601 versions.
                //

                Mat44f M = Rec709FullRangeRGBToYUV8<float>();
                const float rw709 = M.m00;
                const float gw709 = M.m01;
                const float bw709 = M.m02;

                const float a = (1.0 - s) * rw709 + s;
                const float b = (1.0 - s) * rw709;
                const float c = (1.0 - s) * rw709;
                const float d = (1.0 - s) * gw709;
                const float e = (1.0 - s) * gw709 + s;
                const float f = (1.0 - s) * gw709;
                const float g = (1.0 - s) * bw709;
                const float h = (1.0 - s) * bw709;
                const float j = (1.0 - s) * bw709 + s;

                Mat44f S(a, d, g, 0, b, e, h, 0, c, f, j, 0, 0, 0, 0, 1);

                args[i] = new BoundMat44f(F->parameters()[i], S);
                i++;
            }

            return new Expression(F, args, FA->image());
        }

        Expression* newColorCDLForACES(Expression* FA, const Vec3f& scale,
                                       const Vec3f& offset, const Vec3f& power,
                                       float saturation, bool noClamp,
                                       bool isACESLog)
        {
            const float minClamp =
                (noClamp ? -std::numeric_limits<float>::max() : 0.0f);
            const float maxClamp =
                (noClamp ? std::numeric_limits<float>::max() : 1.0f);

            Mat44f M = TwkMath::Rec709FullRangeRGBToYUV8<float>();
            const Vec3f lumaCoefficients(M.m00, M.m01, M.m02);

            // Sees ACES spec for ACES Log for CDL.
            const Vec3f refLow =
                Vec3f(isACESLog ? 12860.643f : 0.001185417175293f);
            const Vec3f refHigh = Vec3f(isACESLog ? 48742.586f : 222.875f);

            Mat44f toACES = TwkMath::Rec709ToACES<float>();
            Mat44f fromACES = TwkMath::ACESToRec709<float>();
            ;

            const Function* F =
                (isACESLog ? colorCDLForACESLog() : colorCDLForACESLinear());
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], FA);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], scale);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], offset);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], power);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], saturation);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], lumaCoefficients);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], refLow);
            i++;
            args[i] = new BoundVec3f(F->parameters()[i], refHigh);
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], toACES);
            i++;
            args[i] = new BoundMat44f(F->parameters()[i], fromACES);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], minClamp);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], maxClamp);
            i++;

            return new Expression(F, args, FA->image());
        }

        Expression* newDownSample(const IPImage* image, Expression* expr,
                                  const int scale)
        {
            const Function* F =
                scale % 2 ? resizeDownSample() : resizeDownSampleFast();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], expr);
            i++;
            args[i] = new BoundFloat(F->parameters()[i], (float)scale);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;

            return new Expression(F, args, image);
        }

        Expression* newDerivativeDownSample(Expression* expr)
        {
            const Function* F = resizeDownSampleDerivative();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], expr);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            return new Expression(F, args, expr->image());
        }

        Expression* newUpSampleVertical(const IPImage* image, Expression* expr,
                                        bool highQuality)
        {
            const Function* F = highQuality ? resizeUpSampleVerticalMitchell2()
                                            : resizeUpSampleVertical2();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], expr);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;

            return new Expression(F, args, image);
        }

        Expression* newUpSampleHorizontal(const IPImage* image,
                                          Expression* expr, bool highQuality)
        {
            const Function* F = highQuality
                                    ? resizeUpSampleHorizontalMitchell2()
                                    : resizeUpSampleHorizontal2();
            ArgumentVector args(F->parameters().size());
            size_t i = 0;
            args[i] = new BoundExpression(F->parameters()[i], expr);
            i++;
            args[i] = new BoundSpecial(F->parameters()[i]);
            i++;
            args[i] = new BoundVec2f(F->parameters()[i], Vec2f(0.0f));
            i++;

            return new Expression(F, args, image);
        }

        //--

        Expression* sourceAssemblyShader(IPImage* image, bool bgra, bool force)
        {
            FrameBuffer* fb = image->fb;

            assert(image);

            assert(
                !(image->shaderExpr
                  && image->shaderExpr->function()->type() == Function::Source
                  && !force));

            if (!fb)
            {
                //
                //  This is (presumably) an ImageFBO on the card
                //
                return newSourceRGBA(image);
            }

            //
            //  Possibly swap in BGRA for RGBA or vice versa
            //

            bgra = fb && bgra && fb->dataType() == FrameBuffer::UCHAR
                   && fb->numChannels() == 4;

#if 0
    if (image->shaderExpr && force && bgra &&
        image->shaderExpr->function()->type() == Function::Source &&
        (image->shaderExpr->function()->name() == "SourceRGBA" ||
         image->shaderExpr->function()->name() == "SourceBGRA"))
    {
        delete image->shaderExpr; /// FIX THIS... SHOULD BE HANDLED AT
        /// CALL SITE?
        image->shaderExpr = 0;

        if (image->shaderExpr->function()->name() == "SourceRGBA")
        {
            return newSourceBGRA(fb);
        }
        else
        {
            return newSourceRGBA(fb);
        }

        return 0;
    }
#endif

            //
            //  NOTE: don't worry about any 8 bit BGRA upload optimizations
            //  (like the ARB100 code does). The shader program assembler
            //  should handle that case if asked to by swapping functions if
            //  necessary right before usage.
            //

            if (fb->isPlanar())
            {
                FrameBuffer* P0 = fb->firstPlane();

                switch (fb->numPlanes())
                {
                case 4:
                {
                    FrameBuffer* P1 = P0->nextPlane();
                    FrameBuffer* P2 = P1->nextPlane();
                    FrameBuffer* P3 = P2->nextPlane();

                    if (P0->channelName(0) == "R" && P1->channelName(0) == "G"
                        && P2->channelName(0) == "B"
                        && P3->channelName(0) == "A")
                    {
                        return newSourcePlanarRGBA(image);
                    }
                    else if (P0->channelName(0) == "Y"
                             && P1->channelName(0) == "U"
                             && P2->channelName(0) == "V"
                             && P3->channelName(0) == "A")
                    {
                        //
                        //    These are only for 8 bit
                        //

                        return newSourcePlanarYUVA(image, YUVtoRGBMatrix(fb));
                    }
                    else if (P0->channelName(0) == "Y"
                             && P1->channelName(0) == "RY"
                             && P2->channelName(0) == "BY"
                             && P3->channelName(0) == "A")
                    {
                        Vec3f w;
                        yrybyYweights(fb, w.x, w.y, w.z);
                        return newSourcePlanarYRYBYA(image, w);
                    }
                    else
                    {
                        return newSourcePlanarRGBA(image);
                    }

                    break;
                }
                case 3:
                {
                    FrameBuffer* P1 = P0->nextPlane();
                    FrameBuffer* P2 = P1->nextPlane();

                    if (P0->channelName(0) == "R" && P1->channelName(0) == "G"
                        && P2->channelName(0) == "B")
                    {
                        return newSourcePlanarRGB(image);
                    }
                    else if (P0->channelName(0) == "Y"
                             && P1->channelName(0) == "U"
                             && P2->channelName(0) == "V")
                    {
                        //
                        //    These are only for 8 bit
                        //
                        return newSourcePlanarYUV(image, YUVtoRGBMatrix(fb));
                    }
                    else if (P0->channelName(0) == "Y"
                             && P1->channelName(0) == "RY"
                             && P2->channelName(0) == "BY")
                    {
                        Vec3f w;
                        yrybyYweights(fb, w.x, w.y, w.z);
                        return newSourcePlanarYRYBY(image, w);
                    }
                    else
                    {
                        return newSourcePlanarRGB(image);
                    }

                    break;
                }
                case 2:
                {
                    FrameBuffer* P1 = P0->nextPlane();

                    if (P0->numChannels() == 1 && P1->numChannels() == 2
                        && P0->channelName(0) == "Y"
                        && P1->channelName(0) == "U"
                        && P1->channelName(1) == "V")
                    {
                        //
                        //    These are only for 8 bit
                        //

                        return newSourcePlanar2YUV(image, YUVtoRGBMatrix(fb));
                    }
                    else if (P0->numChannels() == 2 && P1->numChannels() == 2
                             && P0->channelName(0) == "Y"
                             && P0->channelName(1) == "A"
                             && P1->channelName(0) == "RY"
                             && P1->channelName(1) == "BY")
                    {
                        Vec3f w;
                        yrybyYweights(fb, w.x, w.y, w.z);
                        return newSourcePlanarYAC2(image, w);
                    }
                    else if (P0->numChannels() == 1 && P1->numChannels() == 1)
                    {
                        return newSourcePlanarYA(image);
                    }

                    break;
                }

                default:
                    break;
                }
            }
            else
            {
                const string& C0 = fb->channelName(0);

                switch (fb->numChannels())
                {
                case 4:
                {
                    const string& C1 = fb->channelName(1);
                    const string& C2 = fb->channelName(2);
                    const string& C3 = fb->channelName(3);

                    if (C0 == "R" && C1 == "G" && C2 == "B" && C3 == "A")
                    {
                        return bgra ? newSourceBGRA(image)
                                    : newSourceRGBA(image);
                    }
                    else if (C0 == "B" && C1 == "G" && C2 == "R" && C3 == "A")
                    {
                        return bgra ? newSourceRGBA(image)
                                    : newSourceBGRA(image);
                    }
                    else if (C0 == "A" && C1 == "R" && C2 == "G" && C3 == "B")
                    {
                        return bgra ? newSourceARGBfromBGRA(image)
                                    : newSourceARGB(image);
                    }
                    else if (C0 == "A" && C1 == "B" && C2 == "G" && C3 == "R")
                    {
                        return bgra ? newSourceABGRfromBGRA(image)
                                    : newSourceABGR(image);
                    }
                    else if (C0 == "Y" && C1 == "U" && C2 == "V" && C3 == "A")
                    {
                        return newSourceYUVA(image, YUVtoRGBMatrix(fb));
                    }
                    else if (C0 == "A" && C1 == "Y" && C2 == "U" && C3 == "V")
                    {
                        return newSourceAYUV(image, YUVtoRGBMatrix(fb));
                    }
                    else
                    {
                        return bgra ? newSourceBGRA(image)
                                    : newSourceRGBA(image);
                    }

                    break;
                }

                case 3:
                {
                    const string& C1 = fb->channelName(1);
                    const string& C2 = fb->channelName(2);

                    if (C0 == "R" && C1 == "G" && C2 == "B")
                    {
                        return bgra ? newSourceBGRA(image)
                                    : newSourceRGBA(image);
                    }
                    else if (C0 == "B" && C1 == "G" && C2 == "R")
                    {
                        return bgra ? newSourceRGBA(image)
                                    : newSourceBGRA(image);
                    }
                    else
                    {
                        return bgra ? newSourceBGRA(image)
                                    : newSourceRGBA(image);
                    }
                }

                break;

                case 2:
                {
                    return newSourceYA(image);
                }

                case 1:
                {
                    if (fb->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
                    {
                        return newSourceRGBA(image);
                    }
                    else if (fb->dataType()
                             == FrameBuffer::PACKED_X2_B10_G10_R10)
                    {
                        return newSourceRGBA(image);
                    }
                    else if (fb->dataType() == FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8
                             || fb->dataType()
                                    == FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8)
                    {
                        return newSourceYVYU(image, YUVtoRGBMatrix(fb));
                    }
                    else
                    {
                        return newSourceY(image);
                    }
                }

                default:
                    break;
                }
            }

            return newSourceRGBA(image);
        }

        Expression* replaceSourceWithExpression(Expression* root,
                                                Expression* expr)
        {
            if (root->function()->type() == Function::Source)
            {
                delete root;
                return expr;
            }

            const ArgumentVector& args = root->arguments();

            for (size_t i = 0; i < args.size(); i++)
            {
                if (BoundExpression* be =
                        dynamic_cast<BoundExpression*>(args[i]))
                {
                    Expression* value = be->value();

                    if (value->function()->type() == Function::Source)
                    {
                        be->setValue(expr);
                        delete value;
                        return root;
                    }
                    else if (Expression* rexpr =
                                 replaceSourceWithExpression(value, expr))
                    {
                        return root;
                    }
                }
            }

            return 0;
        }

        ///////////////////////////FAST GAUSSIAN ////////////////////////////
        /// ---Utilize gl bilinear interp to reduce the number of samples we
        /// need
        ///    to roughly 50% of the plain case
        /// ---We maintain a list of gaussian weights up to kernel size 100
        /// ---When a node calls ApplyFastGaussianFilter, it will simply get the
        ///    resulting IPImage back.
        /////////////////////////////////////////////////////////////////////

        // NOTE: global variable used to main gaussian weights
        // map key defined by radius and sigma
        static std::map<unsigned int, TwkFB::FrameBuffer*>
            allGaussianWeightsMap;
        static boost::mutex gaussMutex;

        static unsigned int generateKeyForGaussian(size_t radius, float sigma)
        {
            std::stringstream id;
            id << radius << sigma;
            boost::hash<string> string_hash;
            size_t v = string_hash(id.str());

            if (sizeof(size_t) == 8)
            {
                return (v & 0xffffffff) ^ ((v >> 32) & 0xffffffff);
            }
            else
            {
                return v;
            }
        }

        FrameBuffer* generateFastGaussianWeightsFB(size_t radius, float sigma)
        {

            unsigned int gkey = generateKeyForGaussian(radius, sigma);

            // lock up around map reading/writing
            boost::mutex::scoped_lock lock(gaussMutex);

            std::map<unsigned int, TwkFB::FrameBuffer*>::iterator it;
            it = allGaussianWeightsMap.find(gkey);
            FrameBuffer* result = NULL;
            if (it != allGaussianWeightsMap.end())
            {
                result = it->second;
            }
            else
            {
                // generate plain gaussian weights
                vector<float> rawweights(
                    radius + 1); // these are the plain gaussian weights
                rawweights[0] = 1.0;
                for (int i = 1; i <= radius; ++i)
                {
                    rawweights[i] = exp(-i * i * sigma);
                }

                // utilize gl bilinear interp to reduce the number of samples we
                // need if radius is 2*n, we need 1 + n weights if radius is 2*n
                // + 1, we need 2 + n weights data contains in order, offset and
                // weight (vec2) for each point we need to sample
                size_t fastNo = radius / 2;
                size_t dataSize = 1 + fastNo + radius % 2;
                void* fbdata = FrameBuffer::allocateLargeBlock(3 * sizeof(float)
                                                               * dataSize);
                Vec3f* data = (Vec3f*)fbdata;
                data[0] = Vec3f(0.0, 1.0, 0.0);
                for (size_t i = 1; i <= fastNo; ++i)
                {
                    data[i].y = rawweights[2 * i - 1] + rawweights[2 * i];
                    data[i].x = ((2 * i - 1) * rawweights[2 * i - 1]
                                 + 2 * i * rawweights[2 * i])
                                / data[i].y;
                    data[i].z = 0;
                }
                if (radius % 2 == 1)
                {
                    // this sample is on its own, all the previous ones have
                    // been handled in pairs
                    data[fastNo + 1] = Vec3f(radius, rawweights[radius], 0);
                }

                result = new FrameBuffer(dataSize, 1, 3, FrameBuffer::FLOAT,
                                         (unsigned char*)data, 0,
                                         FrameBuffer::BOTTOMLEFT, false);
                result->idstream()
                    << "FastGaussianWeights" << radius << "_" << sigma;

                allGaussianWeightsMap[gkey] = result;
            }
            return result->referenceCopy();
        }

        // use default sigma (e^-3)
        FrameBuffer* generateFastGaussianWeightsFB(size_t radius)
        {
            // 3 sigma away is 0.05 (this is a good default value)
            // 4.5 sigma away is 0.01 effectively 0 as far as filter is
            // concerned
            float sigma = radius * radius / 3.0;
            sigma = 1.0 / sigma; // to avoid division in shader, we send 1/sigma
            return generateFastGaussianWeightsFB(radius, sigma);
        }

        // utilize gl bilinear interp to reduce the number of samples we need
        IPImage* applyFastGaussianFilter(const IPNode* node, IPImage* image,
                                         size_t radius, float sigma)
        {
            size_t width = image->width;
            size_t height = image->height;

            // these are reference copies of the one in the global map
            FrameBuffer* gaussFBH =
                generateFastGaussianWeightsFB(radius, sigma);
            FrameBuffer* gaussFBV = gaussFBH->referenceCopy();

            // horizontal
            // this image doesn't necessarily have to be intermediate. if the
            // upcoming image already has a number of complicated shaders on it,
            // it would be worth making this one intermediate. otherwise it
            // should be faster to make this current
            IPImage* gaussHorizontal =
                new IPImage(node, IPImage::BlendRenderType, width, height, 1.0,
                            IPImage::IntermediateBuffer);
            gaussHorizontal->shaderExpr =
                Shader::newSourceRGBA(gaussHorizontal);
            gaussHorizontal->shaderExpr =
                Shader::newFilterGaussianHorizontalFast(
                    gaussHorizontal->shaderExpr, gaussFBH, radius);

            gaussHorizontal->appendChild(image);

            // vertical
            IPImage* gaussVertical =
                new IPImage(node, IPImage::BlendRenderType, width, height, 1.0,
                            IPImage::IntermediateBuffer);
            gaussVertical->shaderExpr = Shader::newSourceRGBA(gaussVertical);
            gaussVertical->shaderExpr = Shader::newFilterGaussianVerticalFast(
                gaussVertical->shaderExpr, gaussFBV, radius);
            gaussVertical->appendChild(gaussHorizontal);

            // this is to make sure the gaussian (both v and h) are rendered
            // into intermediate, and cached
            IPImage* root =
                new IPImage(node, IPImage::BlendRenderType, width, height, 1.0,
                            IPImage::IntermediateBuffer);
            root->shaderExpr = Shader::newSourceRGBA(root);
            root->appendChild(gaussVertical);

            return root;
        }

        // use default sigma (e^-3)
        IPImage* applyFastGaussianFilter(const IPNode* node, IPImage* image,
                                         size_t radius)
        {
            // 3 sigma away is 0.05 (this is a good default value)
            // 4.5 sigma away is 0.01 effectively 0 as far as filter is
            // concerned
            float sigma = radius * radius / 3.0;
            sigma = 1.0 / sigma; // to avoid division in shader, we send 1/sigma
            return applyFastGaussianFilter(node, image, radius, sigma);
        }

        // plain gaussian, no magic. compute exp in shader. for small kernels
        // like 3 by 3, it is not worth sending weights in as a framebuffer. do
        // it the old fashioned way
        IPImage* applyGaussianFilter(const IPNode* node, IPImage* image,
                                     size_t radius, float sigma)
        {

            size_t width = image->width;
            size_t height = image->height;

            // horizontal
            // this image doesn't necessarily have to be intermediate. if the
            // upcoming image already has a number of complicated shaders on it,
            // it would be worth making this one intermediate. otherwise it
            // should be faster to make this current
            IPImage* gaussHorizontal =
                new IPImage(node, IPImage::BlendRenderType, width, height, 1.0,
                            IPImage::IntermediateBuffer);
            gaussHorizontal->shaderExpr =
                Shader::newSourceRGBA(gaussHorizontal);
            gaussHorizontal->shaderExpr = Shader::newFilterGaussianHorizontal(
                gaussHorizontal->shaderExpr, radius, sigma);
            gaussHorizontal->appendChild(image);

            // vertical
            IPImage* gaussVertical =
                new IPImage(node, IPImage::BlendRenderType, width, height, 1.0,
                            IPImage::IntermediateBuffer);
            gaussVertical->shaderExpr = Shader::newSourceRGBA(gaussVertical);
            gaussVertical->shaderExpr = Shader::newFilterGaussianVertical(
                gaussVertical->shaderExpr, radius, sigma);
            gaussVertical->appendChild(gaussHorizontal);

            // this is to make sure the gaussian (both v and h) are rendered
            // into intermediate, and cached
            IPImage* root =
                new IPImage(node, IPImage::BlendRenderType, width, height, 1.0,
                            IPImage::IntermediateBuffer);
            root->shaderExpr = Shader::newSourceRGBA(root);
            root->appendChild(gaussVertical);

            return root;
        }

        // use default sigma (e^-3)
        IPImage* applyGaussianFilter(const IPNode* node, IPImage* image,
                                     size_t radius)
        {
            // 3 sigma away is 0.05 (this is a good default value)
            // 4.5 sigma away is 0.01 effectively 0 as far as filter is
            // concerned
            float sigma = radius * radius / 3.0;
            sigma = 1.0 / sigma; // to avoid division in shader, we send 1/sigma
            return applyGaussianFilter(node, image, radius, sigma);
        }
    } // namespace Shader
} // namespace IPCore
