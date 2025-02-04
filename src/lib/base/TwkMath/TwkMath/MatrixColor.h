//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
/* THIS IS AN AUTO-GENERATED FILE     */

/* CREATED BY bin/imgtools/yuvconvert.*/

#ifndef __TwkMath__MatrixColor_h__
#define __TwkMath__MatrixColor_h__

namespace TwkMath
{

    template <typename T> inline Mat44<T> Rec601VideoRangeRGBToYUV8()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.567883e-01), T(5.041294e-01), T(9.790588e-02),
                        T(6.274512e-02), T(-1.482229e-01), T(-2.909928e-01),
                        T(4.392157e-01), T(5.019608e-01), T(4.392157e-01),
                        T(-3.677883e-01), T(-7.142738e-02), T(5.019608e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601VideoRangeYUVToRGB8()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.164384e+00), T(0.000000e+00), T(1.596027e+00),
                        T(-8.742022e-01), T(1.164384e+00), T(-3.917623e-01),
                        T(-8.129675e-01), T(5.316678e-01), T(1.164384e+00),
                        T(2.017232e+00), T(0.000000e+00), T(-1.085631e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601VideoRangeRGBToYUV10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.560352e-01), T(5.026510e-01), T(9.761877e-02),
                        T(6.256112e-02), T(-1.477882e-01), T(-2.901394e-01),
                        T(4.379277e-01), T(5.004887e-01), T(4.379277e-01),
                        T(-3.667098e-01), T(-7.121791e-02), T(5.004888e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601VideoRangeYUVToRGB10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.167808e+00), T(0.000000e+00), T(1.600721e+00),
                        T(-8.742022e-01), T(1.167808e+00), T(-3.929145e-01),
                        T(-8.153587e-01), T(5.316678e-01), T(1.167808e+00),
                        T(2.023165e+00), T(0.000000e+00), T(-1.085631e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601VideoRangeRGBToYUV16()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.557891e-01), T(5.021678e-01), T(9.752491e-02),
                        T(6.250107e-02), T(-1.476462e-01), T(-2.898605e-01),
                        T(4.375067e-01), T(5.000076e-01), T(4.375066e-01),
                        T(-3.663572e-01), T(-7.114944e-02), T(5.000076e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601VideoRangeYUVToRGB16()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.168932e+00), T(0.000000e+00), T(1.602261e+00),
                        T(-8.742022e-01), T(1.168932e+00), T(-3.932926e-01),
                        T(-8.161433e-01), T(5.316678e-01), T(1.168932e+00),
                        T(2.025112e+00), T(0.000000e+00), T(-1.085631e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601FullRangeRGBToYUV8()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.990000e-01), T(5.870001e-01), T(1.140000e-01),
                        T(0.000000e+00), T(-1.687359e-01), T(-3.312642e-01),
                        T(5.000000e-01), T(5.019608e-01), T(5.000000e-01),
                        T(-4.186876e-01), T(-8.131242e-02), T(5.019608e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601FullRangeYUVToRGB8()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.000000e+00), T(0.000000e+00), T(1.402000e+00),
                        T(-7.037491e-01), T(1.000000e+00), T(-3.441363e-01),
                        T(-7.141362e-01), T(5.312113e-01), T(1.000000e+00),
                        T(1.772000e+00), T(0.000000e+00), T(-8.894745e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601FullRangeRGBToYUV10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.990000e-01), T(5.870001e-01), T(1.140000e-01),
                        T(0.000000e+00), T(-1.687359e-01), T(-3.312642e-01),
                        T(5.000000e-01), T(5.004888e-01), T(5.000000e-01),
                        T(-4.186876e-01), T(-8.131242e-02), T(5.004888e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601FullRangeYUVToRGB10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.000000e+00), T(0.000000e+00), T(1.402000e+00),
                        T(-7.016852e-01), T(1.000000e+00), T(-3.441363e-01),
                        T(-7.141362e-01), T(5.296535e-01), T(1.000000e+00),
                        T(1.772000e+00), T(0.000000e+00), T(-8.868660e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601FullRangeRGBToYUV16()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.990000e-01), T(5.870001e-01), T(1.140000e-01),
                        T(0.000000e+00), T(-1.687359e-01), T(-3.312642e-01),
                        T(5.000000e-01), T(5.000077e-01), T(5.000000e-01),
                        T(-4.186876e-01), T(-8.131242e-02), T(5.000077e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601FullRangeYUVToRGB16()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.000000e+00), T(0.000000e+00), T(1.402000e+00),
                        T(-7.010106e-01), T(1.000000e+00), T(-3.441363e-01),
                        T(-7.141362e-01), T(5.291443e-01), T(1.000000e+00),
                        T(1.772000e+00), T(0.000000e+00), T(-8.860135e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601ExtendedRangeYUVToRGB10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.966617e-01), T(5.824096e-01), T(1.131085e-01),
                        T(3.910062e-03), T(-1.674163e-01), T(-3.286736e-01),
                        T(4.960899e-01), T(5.004888e-01), T(4.960900e-01),
                        T(-4.154134e-01), T(-8.067654e-02), T(5.004888e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec601ExtendedRangeRGBToYUV10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.007882e+00), T(0.000000e+00), T(1.413050e+00),
                        T(-7.111566e-01), T(1.007882e+00), T(-3.468487e-01),
                        T(-7.197648e-01), T(5.298872e-01), T(1.007882e+00),
                        T(1.785966e+00), T(0.000000e+00), T(-8.977970e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709VideoRangeRGBToYUV8()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.825858e-01), T(6.142306e-01), T(6.200706e-02),
                        T(6.274501e-02), T(-1.006437e-01), T(-3.385720e-01),
                        T(4.392157e-01), T(5.019609e-01), T(4.392157e-01),
                        T(-3.989422e-01), T(-4.027352e-02), T(5.019609e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709VideoRangeYUVToRGB8()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.164384e+00), T(0.000000e+00), T(1.792741e+00),
                        T(-9.729452e-01), T(1.164384e+00), T(-2.132486e-01),
                        T(-5.329093e-01), T(3.014827e-01), T(1.164384e+00),
                        T(2.112402e+00), T(0.000000e+00), T(-1.133402e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709VideoRangeRGBToYUV10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.820504e-01), T(6.124294e-01), T(6.182522e-02),
                        T(6.256112e-02), T(-1.003486e-01), T(-3.375791e-01),
                        T(4.379277e-01), T(5.004888e-01), T(4.379277e-01),
                        T(-3.977723e-01), T(-4.015542e-02), T(5.004888e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709VideoRangeYUVToRGB10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.167808e+00), T(0.000000e+00), T(1.798014e+00),
                        T(-9.729451e-01), T(1.167808e+00), T(-2.138758e-01),
                        T(-5.344767e-01), T(3.014826e-01), T(1.167808e+00),
                        T(2.118615e+00), T(0.000000e+00), T(-1.133402e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709VideoRangeRGBToYUV16()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.818754e-01), T(6.118406e-01), T(6.176579e-02),
                        T(6.250086e-02), T(-1.002521e-01), T(-3.372546e-01),
                        T(4.375067e-01), T(5.000077e-01), T(4.375067e-01),
                        T(-3.973899e-01), T(-4.011682e-02), T(5.000077e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709VideoRangeYUVToRGB16()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.168932e+00), T(0.000000e+00), T(1.799744e+00),
                        T(-9.729451e-01), T(1.168932e+00), T(-2.140816e-01),
                        T(-5.349910e-01), T(3.014827e-01), T(1.168932e+00),
                        T(2.120653e+00), T(0.000000e+00), T(-1.133402e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709FullRangeRGBToYUV8()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.126001e-01), T(7.151999e-01), T(7.219999e-02),
                        T(0.000000e+00), T(-1.145722e-01), T(-3.854279e-01),
                        T(5.000000e-01), T(5.019608e-01), T(5.000000e-01),
                        T(-4.541529e-01), T(-4.584708e-02), T(5.019608e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709FullRangeYUVToRGB8()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.000000e+00), T(0.000000e+00), T(1.574800e+00),
                        T(-7.904879e-01), T(1.000000e+00), T(-1.873243e-01),
                        T(-4.681243e-01), T(3.290095e-01), T(1.000000e+00),
                        T(1.855600e+00), T(0.000000e+00), T(-9.314385e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709FullRangeRGBToYUV10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.126001e-01), T(7.151999e-01), T(7.219999e-02),
                        T(0.000000e+00), T(-1.145722e-01), T(-3.854279e-01),
                        T(5.000000e-01), T(5.004888e-01), T(5.000000e-01),
                        T(-4.541529e-01), T(-4.584708e-02), T(5.004887e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709FullRangeYUVToRGB10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.000000e+00), T(0.000000e+00), T(1.574800e+00),
                        T(-7.881697e-01), T(1.000000e+00), T(-1.873243e-01),
                        T(-4.681243e-01), T(3.280446e-01), T(1.000000e+00),
                        T(1.855600e+00), T(0.000000e+00), T(-9.287069e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709FullRangeRGBToYUV16()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.126001e-01), T(7.151999e-01), T(7.219999e-02),
                        T(0.000000e+00), T(-1.145722e-01), T(-3.854279e-01),
                        T(5.000000e-01), T(5.000076e-01), T(5.000000e-01),
                        T(-4.541529e-01), T(-4.584708e-02), T(5.000076e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709FullRangeYUVToRGB16()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.000000e+00), T(0.000000e+00), T(1.574800e+00),
                        T(-7.874120e-01), T(1.000000e+00), T(-1.873243e-01),
                        T(-4.681243e-01), T(3.277293e-01), T(1.000000e+00),
                        T(1.855600e+00), T(0.000000e+00), T(-9.278142e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709ExtendedRangeYUVToRGB10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(2.109376e-01), T(7.096070e-01), T(7.163538e-02),
                        T(3.910121e-03), T(-1.136762e-01), T(-3.824138e-01),
                        T(4.960899e-01), T(5.004888e-01), T(4.960899e-01),
                        T(-4.506013e-01), T(-4.548855e-02), T(5.004887e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709ExtendedRangeRGBToYUV10()
    {
        // ... AUTO GENERATED CODE (see above) ...
        return Mat44<T>(T(1.007882e+00), T(0.000000e+00), T(1.587212e+00),
                        T(-7.983227e-01), T(1.007882e+00), T(-1.888007e-01),
                        T(-4.718139e-01), T(3.266893e-01), T(1.007882e+00),
                        T(1.870225e+00), T(0.000000e+00), T(-9.399677e-01),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709ToACES()
    {
        return Mat44<T>(
            T(4.396330e-01), T(3.829891e-01), T(1.773786e-01), T(0.000000e+00),
            T(8.977623e-02), T(8.134389e-01), T(9.678369e-02), T(0.000000e+00),
            T(1.754106e-02), T(1.115466e-01), T(8.709126e-01), T(0.000000e+00),
            T(0.000000e+00), T(0.000000e+00), T(0.000000e+00), T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> ACESToRec709()
    {
        return Mat44<T>(T(2.521685e+00), T(-1.134132e+00), T(-3.875561e-01),
                        T(0.000000e+00), T(-2.764794e-01), T(1.372720e+00),
                        T(-9.623854e-02), T(0.000000e+00), T(-1.537779e-02),
                        T(-1.529755e-01), T(1.168353e+00), T(0.000000e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709ToXYZ()
    {
        return Mat44<T>(
            T(4.123909e-01), T(3.575847e-01), T(1.804811e-01), T(0.000000e+00),
            T(2.126389e-01), T(7.151684e-01), T(7.219206e-02), T(0.000000e+00),
            T(1.933011e-02), T(1.191912e-01), T(9.505037e-01), T(0.000000e+00),
            T(0.000000e+00), T(0.000000e+00), T(0.000000e+00), T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> XYZToRec709()
    {
        return Mat44<T>(T(3.240970e+00), T(-1.537385e+00), T(-4.986269e-01),
                        T(0.000000e+00), T(-9.692436e-01), T(1.875969e+00),
                        T(4.155701e-02), T(0.000000e+00), T(5.563050e-02),
                        T(-2.039773e-01), T(1.057003e+00), T(0.000000e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> ArriSceneReferredToXYZ()
    {
        return Mat44<T>(
            T(6.380077e-01), T(2.147042e-01), T(9.774475e-02), T(0.000000e+00),
            T(2.919536e-01), T(8.238404e-01), T(-1.157950e-01), T(0.000000e+00),
            T(2.797976e-03), T(-6.703217e-02), T(1.153259e+00), T(0.000000e+00),
            T(0.000000e+00), T(0.000000e+00), T(0.000000e+00), T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> XYZToArriSceneReferred()
    {
        return Mat44<T>(T(1.789066e+00), T(-4.825349e-01), T(-2.000824e-01),
                        T(0.000000e+00), T(-6.398488e-01), T(1.396401e+00),
                        T(1.944386e-01), T(0.000000e+00), T(-4.153119e-02),
                        T(8.233530e-02), T(8.788948e-01), T(0.000000e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> ArriWideGamutToXYZ()
    {
        return Mat44<T>(
            T(5.983964e-01), T(2.712610e-01), T(7.690950e-02), T(0.000000e+00),
            T(2.908683e-01), T(8.233573e-01), T(-1.157822e-01), T(0.000000e+00),
            T(1.444217e-02), T(2.917472e-02), T(1.024923e+00), T(0.000000e+00),
            T(0.000000e+00), T(0.000000e+00), T(0.000000e+00), T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> XYZToArriWideGamut()
    {
        return Mat44<T>(T(1.991614e+00), T(-6.482611e-01), T(-2.226812e-01),
                        T(0.000000e+00), T(-7.047051e-01), T(1.439075e+00),
                        T(2.154482e-01), T(0.000000e+00), T(-8.004165e-03),
                        T(-3.182906e-02), T(9.726881e-01), T(0.000000e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> ArriSceneReferredToRec709()
    {
        return Mat44<T>(T(1.617524e+00), T(-5.372848e-01), T(-8.023572e-02),
                        T(0.000000e+00), T(-7.057323e-02), T(1.334612e+00),
                        T(-2.640408e-01), T(0.000000e+00), T(-2.110192e-02),
                        T(-2.269537e-01), T(1.248056e+00), T(0.000000e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> Rec709ToArriSceneReferred()
    {
        return Mat44<T>(
            T(6.313209e-01), T(2.708000e-01), T(9.787762e-02), T(0.000000e+00),
            T(3.682023e-02), T(7.930380e-01), T(1.701436e-01), T(0.000000e+00),
            T(1.736987e-02), T(1.487892e-01), T(8.338408e-01), T(0.000000e+00),
            T(0.000000e+00), T(0.000000e+00), T(0.000000e+00), T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> ArriFilmStyleMatrix()
    {
        return Mat44<T>(T(1.271103e+00), T(-2.842790e-01), T(1.317600e-02),
                        T(0.000000e+00), T(-1.271650e-01), T(1.436429e+00),
                        T(-3.092640e-01), T(0.000000e+00), T(-1.299270e-01),
                        T(-5.102860e-01), T(1.640214e+00), T(0.000000e+00),
                        T(0.000000e+00), T(0.000000e+00), T(0.000000e+00),
                        T(1.000000e+00));
    }

    template <typename T> inline Mat44<T> ArriFilmStyleInverseMatrix()
    {
        return Mat44<T>(
            T(8.061650e-01), T(1.685340e-01), T(2.530100e-02), T(0.000000e+00),
            T(9.122800e-02), T(7.652210e-01), T(1.435500e-01), T(0.000000e+00),
            T(9.224100e-02), T(2.514180e-01), T(6.563410e-01), T(0.000000e+00),
            T(0.000000e+00), T(0.000000e+00), T(0.000000e+00), T(1.000000e+00));
    }

} // End namespace TwkMath

#endif // __TwkMath_MatrixColor_h__
