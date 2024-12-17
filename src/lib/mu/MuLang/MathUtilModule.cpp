//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/MathUtilModule.h>
#include <MuLang/Native.h>
#include <Mu/Alias.h>
#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/ReferenceType.h>
#include <Mu/Value.h>
#include <MuLang/Noise.h>
#include <iostream>
#include <limits.h>
#include <math.h>
#include <algorithm>

namespace Mu
{
    using namespace std;

    MathUtilModule::MathUtilModule(Context* c)
        : Module(c, "math_util")
    {
    }

    MathUtilModule::~MathUtilModule() {}

    inline float frand(double a)
    {
        /* AJG - more rand junk */
        // return float( double(::random()) / double(ULONG_MAX >> 1) * a );
        return float(double(rand()) / double(UINT_MAX >> 1) * a);
    }

    Vector2f dnoise2(const Vector2f& p)
    {
        Vector2f grad;
        Mu::noiseAndGrad2((const float*)&p, (float*)&grad);
        return grad;
    }

    Vector3f dnoise3(const Vector3f& p)
    {
        Vector3f grad;
        Mu::noiseAndGrad3((const float*)&p, (float*)&grad);
        return grad;
    }

    float gauss(float scale)
    {
        //
        // Pick two uniform random numbers that lie within the -1, 1
        // square and discard them if they do not lie within the unit
        // circle.

        float x;
        float y;
        float r2;

        do
        {
            x = frand(2.0) - 1.0f;
            y = frand(2.0) - 1.0f;
            r2 = x * x + y * y;
        } while (r2 > 1.0f || r2 == 0.0f);

        const float fac = sqrtf(-2.0f * logf(r2) / r2);

        //
        // This Box-Muller
        // (or Box-Mueller, depending on where you read)
        // generates two normal deviates, not just one.
        //
        //  This discards the other number in favor of being reentrant.
        //

        return y * fac * scale;
    }

    Vector3f sphrand()
    {
        Vector3f v;
        float d;

        do
        {
            v[0] = frand(2.0) - 1.0f;
            v[1] = frand(2.0) - 1.0f;
            v[2] = frand(2.0) - 1.0f;
            d = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
        } while (d > 1.0f);

        return v;
    }

#define T Thread&

    float __C_math_util_clamp_float_float_float_float(T, float a, float b,
                                                      float c)
    {
        return float(a < b ? b : (a > c ? c : a));
    }

    float __C_math_util_step_float_float_float(T, float a, float b)
    {
        return a < b ? 0.0f : 1.0f;
    }

    float __C_math_util_linstep_float_float_float_float(T, float a, float b,
                                                        float c)
    {
        return linstep(a, b, c);
    }

    float __C_math_util_smoothstep_float_float_float_float(T, float a, float b,
                                                           float c)
    {
        return smoothstep(a, b, c);
    }

    float __C_math_util_hermite_float_float_float_float_float_float(
        T, float a, float b, float c, float d, float e)
    {
        return hermite(a, b, c, d, e);
    }

    float __C_math_util_lerp_float_float_float_float(T, float a, float b,
                                                     float t)
    {
        return float(b * t + a * (1.0f - t));
    }

    Vector2f
    __C_math_util_lerp_vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_(
        T, Vector2f& a, Vector2f& b, float t)
    {
        return b * t + a * (1.0f - t);
    }

    Vector3f
    __C_math_util_lerp_vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_(
        T, Vector3f& a, Vector3f& b, float t)
    {
        return b * t + a * (1.0f - t);
    }

    Vector4f
    __C_math_util_lerp_vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_(
        T, Vector4f& a, Vector4f& b, float t)
    {
        return b * t + a * (1.0f - t);
    }

    Vector3f
    __C_math_util_rotate_vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_float(
        T, Vector3f p, Vector3f axis, float radians)
    {
        return rotate(p, axis, radians);
    }

    float __C_math_util_degrees_float_float(T, float a) { return a * 57.29578; }

    float __C_math_util_radians_float_float(T, float a)
    {
        return a * 0.017453293;
    }

    float __C_math_util_random_float_float(T, float a) { return frand(a); }

    float __C_math_util_gauss_float_float(T, float a) { return gauss(a); }

    /* AJG - random crap */
    // void __C_math_util_seed_void_int(T, int a) { return ::srandom(a); }
    void __C_math_util_seed_void_int(T, int a) { return srand(a); }

    Vector3f __C_math_util_sphrand_vector_floatBSB_3ESB_(T)
    {
        return sphrand();
    }

    float __C_math_util_random_float_float_float(T, float a, float b)
    {
        return frand(b - a) + a;
    }

    int __C_math_util_random_int_int(T, int a)
    {
        /* AJG - random crap */
        // return a ? int(::random()) % a : 0;
        return a ? int(rand()) % a : 0;
    }

    Vector3f __C_math_util_dnoise3_vector_floatBSB_3ESB__vector_floatBSB_3ESB_(
        T, Vector3f a)
    {
        return Mu::dnoise3(a);
    }

    Vector2f __C_math_util_dnoise2_vector_floatBSB_2ESB__vector_floatBSB_2ESB_(
        T, Vector2f a)
    {
        return Mu::dnoise2(a);
    }

    float __C_math_util_dnoise1_float_float(T, float a)
    {
        return Mu::dnoise1(a);
    }

    float __C_math_util_noise3_float_vector_floatBSB_3ESB_(T, Vector3f a)
    {
        return Mu::noise3(a);
    }

    float __C_math_util_noise2_float_vector_floatBSB_2ESB_(T, Vector2f a)
    {
        return Mu::noise2(a);
    }

    float __C_math_util_noise1_float_float(T, float a) { return Mu::noise1(a); }

#undef T

    void MathUtilModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        //
        //  All of the functions are inlined in Native.h So here the
        //  NativeInlined attribute is set in for all function attributes.
        //

        Mapped |= NativeInlined;
        None |= NativeInlined;

        Context* c = context();

        addSymbols(
            new Function(c, "clamp", MathUtilModule::clamp, Mapped, Compiled,
                         __C_math_util_clamp_float_float_float_float, Return,
                         "float", Args, "float", "float", "float", End),

            new Function(c, "step", MathUtilModule::step, Mapped, Compiled,
                         __C_math_util_step_float_float_float, Return, "float",
                         Args, "float", "float", End),

            new Function(c, "linstep", MathUtilModule::linstep, Mapped,
                         Compiled,
                         __C_math_util_linstep_float_float_float_float, Return,
                         "float", Args, "float", "float", "float", End),

            new Function(c, "smoothstep", MathUtilModule::smoothstep, Mapped,
                         Compiled,
                         __C_math_util_smoothstep_float_float_float_float,
                         Return, "float", Args, "float", "float", "float", End),

            new Function(
                c, "hermite", MathUtilModule::hermite, Mapped, Compiled,
                __C_math_util_hermite_float_float_float_float_float_float,
                Return, "float", Args, "float", "float", "float", "float",
                "float", End),

            new Function(c, "lerp", MathUtilModule::lerp, Mapped, Compiled,
                         __C_math_util_lerp_float_float_float_float, Return,
                         "float", Args, "float", "float", "float", End),

            new Function(
                c, "lerp", MathUtilModule::lerp2f, None, Compiled,
                __C_math_util_lerp_vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_,
                Return, "vector float[2]", Args, "vector float[2]",
                "vector float[2]", "float", End),

            new Function(
                c, "lerp", MathUtilModule::lerp3f, None, Compiled,
                __C_math_util_lerp_vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_,
                Return, "vector float[3]", Args, "vector float[3]",
                "vector float[3]", "float", End),

            new Function(
                c, "lerp", MathUtilModule::lerp4f, None, Compiled,
                __C_math_util_lerp_vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_,
                Return, "vector float[4]", Args, "vector float[4]",
                "vector float[4]", "float", End),

            new Function(
                c, "rotate", MathUtilModule::rotate, Mapped, Compiled,
                __C_math_util_rotate_vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_float,
                Return, "vector float[3]", Args, "vector float[3]",
                "vector float[3]", "float", End),

            new Function(c, "degrees", MathUtilModule::degrees, Mapped,
                         Compiled, __C_math_util_degrees_float_float, Return,
                         "float", Args, "float", End),

            new Function(c, "radians", MathUtilModule::radians, Mapped,
                         Compiled, __C_math_util_radians_float_float, Return,
                         "float", Args, "float", End),

            new Function(c, "random", MathUtilModule::randomf2, None, Compiled,
                         __C_math_util_random_float_float_float, Return,
                         "float", Args, "float", "float", End),

            new Function(c, "random", MathUtilModule::randomf, None, Compiled,
                         __C_math_util_random_float_float, Return, "float",
                         Args, "float", End, End),

            new Function(c, "random", MathUtilModule::random, None, Compiled,
                         __C_math_util_random_int_int, Return, "int", Args,
                         "int", End),

            new Function(c, "gauss", MathUtilModule::gauss, None, Compiled,
                         __C_math_util_gauss_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "seed", MathUtilModule::seed, None, Compiled,
                         __C_math_util_seed_void_int, Return, "void", Args,
                         "int", End),

            new Function(c, "sphrand", MathUtilModule::sphrand, None, Compiled,
                         __C_math_util_sphrand_vector_floatBSB_3ESB_, Return,
                         "vector float[3]", End),

            new Function(c, "noise", MathUtilModule::noise1, Mapped, Compiled,
                         __C_math_util_noise1_float_float, Return, "float",
                         Args, "float", End),

            new Function(c, "noise", MathUtilModule::noise2, Mapped, Compiled,
                         __C_math_util_noise2_float_vector_floatBSB_2ESB_,
                         Return, "float", Args, "vector float[2]", End),

            new Function(c, "noise", MathUtilModule::noise3, Mapped, Compiled,
                         __C_math_util_noise3_float_vector_floatBSB_3ESB_,
                         Return, "float", Args, "vector float[3]", End),

            new Function(c, "dnoise", MathUtilModule::dnoise1, Mapped, Compiled,
                         __C_math_util_dnoise1_float_float, Return, "float",
                         Args, "float", End),

            new Function(
                c, "dnoise", MathUtilModule::dnoise2, Mapped, Compiled,
                __C_math_util_dnoise2_vector_floatBSB_2ESB__vector_floatBSB_2ESB_,
                Return, "vector float[2]", Args, "vector float[2]", End),

            new Function(
                c, "dnoise", MathUtilModule::dnoise3, Mapped, Compiled,
                __C_math_util_dnoise3_vector_floatBSB_3ESB__vector_floatBSB_3ESB_,
                Return, "vector float[3]", Args, "vector float[3]", End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(MathUtilModule::random, int)
    {
        int div = NODE_ARG(0, int);
        /* AJG - random crap */
        // NODE_RETURN( div ? int(::random()) % div : 0 );
        NODE_RETURN(div ? int(rand()) % div : 0);
    }

    float extrand(float a) { return frand(a); }

    NODE_IMPLEMENTATION(MathUtilModule::gauss, float)
    {
        NODE_RETURN(Mu::gauss(NODE_ARG(0, float)));
    }

    NODE_IMPLEMENTATION(MathUtilModule::randomf, float)
    {
        NODE_RETURN(frand(NODE_ARG(0, float)));
    }

    NODE_IMPLEMENTATION(MathUtilModule::randomf2, float)
    {
        const float min = NODE_ARG(0, float);
        const float max = NODE_ARG(1, float);
        NODE_RETURN(frand(max - min) + min);
    }

    NODE_IMPLEMENTATION(MathUtilModule::seed, void)
    {
        /* AJG - random crap */
        // ::srandom( (unsigned long) NODE_ARG(0, int) );
        srand((unsigned long)NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(MathUtilModule::clamp, float)
    {
        const float val = NODE_ARG(0, float);
        const float min = NODE_ARG(1, float);
        const float max = NODE_ARG(2, float);
        NODE_RETURN(float(val < min ? min : (val > max ? max : val)));
    }

    float hermite(float p0, float p1, float r0, float r1, float t)
    {
        const float t2 = t * t;
        const float t3 = t2 * t;
        const float t2_x3 = t2 * 3.0f;
        const float t3_x2 = t3 * 2.0f;
        return (float(p0 * (t3_x2 - t2_x3) + p1 * (-t3_x2 + t2_x3)
                      + r0 * (t3 - 2.0f * t2 + t) + r1 * (t3 - t2)));
    }

    NODE_IMPLEMENTATION(MathUtilModule::hermite, float)
    {
        const float p0 = NODE_ARG(0, float);
        const float p1 = NODE_ARG(1, float);
        const float r0 = NODE_ARG(2, float);
        const float r1 = NODE_ARG(3, float);
        const float t = NODE_ARG(4, float);
        NODE_RETURN(Mu::hermite(p0, p1, r0, r1, t));
    }

    float linstep(float a, float b, float t)
    {
        bool invert = false;

        if (a > b)
        {
            invert = true;
            swap(a, b);
        }

        if (t < a)
        {
            return (invert ? 1.0f : 0.0f);
        }
        else
        {
            if (t < b)
            {
                if (invert)
                {
                    return ((t - b) / (a - b));
                }
                else
                {
                    return ((t - a) / (b - a));
                }
            }
            else
            {
                return (invert ? 0.0f : 1.0f);
            }
        }
    }

    NODE_IMPLEMENTATION(MathUtilModule::linstep, float)
    {
        const float a = NODE_ARG(0, float);
        const float b = NODE_ARG(1, float);
        const float t = NODE_ARG(2, float);
        NODE_RETURN(Mu::linstep(a, b, t));
    }

    NODE_IMPLEMENTATION(MathUtilModule::lerp, float)
    {
        const float a = NODE_ARG(0, float);
        const float b = NODE_ARG(1, float);
        const float t = NODE_ARG(2, float);

        NODE_RETURN(float(b * t + a * (1.0f - t)));
    }

    //
    //  Native is inline for lerp
    //

    NODE_IMPLEMENTATION(MathUtilModule::lerp3f, Vector3f)
    {
        const Vector3f a = NODE_ARG(0, Vector3f);
        const Vector3f b = NODE_ARG(1, Vector3f);
        const float t = NODE_ARG(2, float);

        Vector3f v = b * t + a * (1.0f - t);

        NODE_RETURN(v);
    }

    NODE_IMPLEMENTATION(MathUtilModule::lerp2f, Vector2f)
    {
        const Vector2f a = NODE_ARG(0, Vector2f);
        const Vector2f b = NODE_ARG(1, Vector2f);
        const float t = NODE_ARG(2, float);

        Vector2f v = b * t + a * (1.0f - t);

        NODE_RETURN(v);
    }

    NODE_IMPLEMENTATION(MathUtilModule::lerp4f, Vector4f)
    {
        const Vector4f a = NODE_ARG(0, Vector4f);
        const Vector4f b = NODE_ARG(1, Vector4f);
        const float t = NODE_ARG(2, float);

        Vector4f v = b * t + a * (1.0f - t);

        NODE_RETURN(v);
    }

    //
    //  mative is macro for step
    //

    NODE_IMPLEMENTATION(MathUtilModule::step, float)
    {
        const float a = NODE_ARG(0, float);
        const float t = NODE_ARG(1, float);

        NODE_RETURN(t < a ? 0.0f : 1.0f);
    }

    //
    //  smoothstep
    //

    float smoothstep(float a, float b, float u)
    {
        bool invert = false;

        if (a > b)
        {
            invert = true;
            swap(a, b);
        }

        const float t = (u - a) / (b - a);

        if (t < 0.0)
        {
            return (invert ? 1.0f : 0.0f);
        }
        else
        {
            if (t < 1.0)
            {
                const float t2 = t * t;
                const float t3 = t2 * t;
                const float r = float(-2.f * t3 + 3.f * t2);
                return (invert ? 1.0f - r : r);
            }
            else
            {
                return (invert ? 0.0f : 1.0f);
            }
        }
    }

    NODE_IMPLEMENTATION(MathUtilModule::smoothstep, float)
    {
        float a = NODE_ARG(0, float);
        float b = NODE_ARG(1, float);
        float u = NODE_ARG(2, float);
        NODE_RETURN(Mu::smoothstep(a, b, u));
    }

    //
    //  sphrand
    //

    NODE_IMPLEMENTATION(MathUtilModule::sphrand, Vector3f)
    {
        NODE_RETURN(Mu::sphrand());
    }

    //
    //  These are defined as macros for the native versions
    //

    NODE_IMPLEMENTATION(MathUtilModule::degrees, float)
    {
        NODE_RETURN(float(NODE_ARG(0, float) * 57.29578));
    }

    NODE_IMPLEMENTATION(MathUtilModule::radians, float)
    {
        NODE_RETURN(float(NODE_ARG(0, float) * 0.017453293));
    }

    //
    //  noise1
    //

    NODE_IMPLEMENTATION(MathUtilModule::noise1, float)
    {
        NODE_RETURN(Mu::noise1(NODE_ARG(0, float)));
    }

    //
    //  noise2
    //

    float noise2(const Vector2f& v) { return Mu::noise2((const float*)&v); }

    NODE_IMPLEMENTATION(MathUtilModule::noise2, float)
    {
        NODE_RETURN(Mu::noise2(NODE_ARG(0, Vector2f)));
    }

    //
    //  noise3
    //

    float noise3(const Vector3f& v) { return Mu::noise3((const float*)&v); }

    NODE_IMPLEMENTATION(MathUtilModule::noise3, float)
    {
        NODE_RETURN(Mu::noise3(NODE_ARG(0, Vector3f)));
    }

    //
    //  dnoise1
    //

    float dnoise1(float p)
    {
        float grad;
        Mu::noiseAndGrad1(p, grad);
        return grad;
    }

    NODE_IMPLEMENTATION(MathUtilModule::dnoise1, float)
    {
        NODE_RETURN(Mu::dnoise1(NODE_ARG(0, float)));
    }

    //
    //  dnoise 2
    //

    NODE_IMPLEMENTATION(MathUtilModule::dnoise2, Vector2f)
    {
        NODE_RETURN(Mu::dnoise2(NODE_ARG(0, Vector2f)));
    }

    //
    //  dnoise3
    //

    NODE_IMPLEMENTATION(MathUtilModule::dnoise3, Vector3f)
    {
        NODE_RETURN(Mu::dnoise3(NODE_ARG(0, Vector3f)));
    }

    //
    //  rotate
    //

    Vector3f rotate(Vector3f p, Vector3f axis, float radians)
    {
        const float c = ::cos(radians);
        const float s = ::sin(radians);
        const float t = 1.0f - c;
        const float x = axis[0];
        const float y = axis[1];
        const float z = axis[2];

        const float tx = t * x;
        const float ty = t * y;
        const float txy = tx * y;
        const float txz = tx * z;

        const float sx = s * x;
        const float sy = s * y;
        const float sz = s * z;

        const float m00 = tx * x + c;
        const float m01 = txy + sz;
        const float m02 = txz - sy;

        const float m10 = txy - sz;
        const float m11 = ty * y + c;
        const float m12 = ty * z + sx;

        const float m20 = txz + sy;
        const float m21 = ty * z - sx;
        const float m22 = t * z * z + c;

        Vector3f v;
        v[0] = p[0] * m00 + p[1] * m10 + p[2] * m20;
        v[1] = p[0] * m01 + p[1] * m11 + p[2] * m21;
        v[2] = p[0] * m02 + p[1] * m12 + p[2] * m22;

        return v;
    }

    NODE_IMPLEMENTATION(MathUtilModule::rotate, Vector3f)
    {
        const Vector3f p = NODE_ARG(0, Vector3f);
        const Vector3f axis = NODE_ARG(1, Vector3f);
        const float radians = NODE_ARG(2, float);

        NODE_RETURN(Mu::rotate(p, axis, radians));
    }

} // namespace Mu
