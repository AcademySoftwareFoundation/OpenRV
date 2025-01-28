//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/MathModule.h>
#include <Mu/Alias.h>
#include <Mu/Function.h>
#include <Mu/Node.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/NodeAssembler.h>
#include <Mu/ReferenceType.h>
#include <Mu/Value.h>
#include <Mu/MachineRep.h>
#include <iostream>
#include <math.h>

#ifdef _MSC_VER
inline double cbrt(double x) { return pow((double)x, (1.0 / 3.0)); }
#endif

namespace Mu
{

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#ifndef M_E
#define M_E (2.7182818284590452354)
#endif

    using namespace std;

    MathModule::MathModule(Context* c)
        : Module(c, "math")
    {
    }

    MathModule::~MathModule() {}

    //
    //  Because Mu always does floating point operations as double, we
    //  need to make adapter functions for the float versions of the
    //  standard math library. This is unfortunate since an additional
    //  function call is made...
    //

#define T Thread&

    int __C_math_max_int_int_int(T, int a, int b) { return std::max(a, b); }

    int __C_math_min_int_int_int(T, int a, int b) { return std::min(a, b); }

    int __C_math_abs_int_int(T, int a) { return std::abs(a); }

    float __C_math_max_float_float_float(T, float a, float b)
    {
        return std::max(a, b);
    }

    float __C_math_min_float_float_float(T, float a, float b)
    {
        return std::min(a, b);
    }

    float __C_math_abs_float_float(T, float a) { return a > 0 ? a : -a; }

    float __C_math_inversesqrt_float_float(T, float a)
    {
        return float(1.0 / ::sqrt(a));
    }

    float __C_math_sin_float_float(T, float a) { return float(sin(double(a))); }

    float __C_math_cos_float_float(T, float a) { return float(cos(double(a))); }

    float __C_math_tan_float_float(T, float a) { return float(tan(double(a))); }

    float __C_math_asin_float_float(T, float a)
    {
        return float(asin(double(a)));
    }

    float __C_math_acos_float_float(T, float a)
    {
        return float(acos(double(a)));
    }

    float __C_math_atan_float_float(T, float a)
    {
        return float(atan(double(a)));
    }

    float __C_math_atan2_float_float_float(T, float a, float b)
    {
        return float(atan2(double(a), double(b)));
    }

    float __C_math_exp_float_float(T, float a) { return float(exp(double(a))); }

    // float __C_math_expm1_float_float(T, float a) { return
    // float(expm1(double(a))); }
    float __C_math_log_float_float(T, float a) { return float(log(double(a))); }

    float __C_math_log10_float_float(T, float a)
    {
        return float(log10(double(a)));
    }

    // float __C_math_log1p_float_float(T, float a) { return
    // float(log1p(double(a))); }
    float __C_math_sqrt_float_float(T, float a)
    {
        return float(sqrt(double(a)));
    }

    float __C_math_cbrt_float_float(T, float a)
    {
        return float(cbrt(double(a)));
    }

    float __C_math_floor_float_float(T, float a)
    {
        return float(floor(double(a)));
    }

    float __C_math_ceil_float_float(T, float a)
    {
        return float(ceil(double(a)));
    }

    // float __C_math_rint_float_float(T, float a) { return
    // float(rint(double(a))); }
    float __C_math_pow_float_float_float(T, float a, float b)
    {
        return float(pow(double(a), double(b)));
    }

    float __C_math_hypot_float_float_float(T, float a, float b)
    {
        return float(hypot(double(a), double(b)));
    }

    double __C_math_max_double_double_double(T, double a, double b)
    {
        return std::max(a, b);
    }

    double __C_math_min_double_double_double(T, double a, double b)
    {
        return std::min(a, b);
    }

    double __C_math_abs_double_double(T, double a) { return a > 0 ? a : -a; }

    double __C_math_inversesqrt_double_double(T, double a)
    {
        return double(1.0 / ::sqrt(a));
    }

    double __C_math_sin_double_double(T, double a)
    {
        return double(sin(double(a)));
    }

    double __C_math_cos_double_double(T, double a)
    {
        return double(cos(double(a)));
    }

    double __C_math_tan_double_double(T, double a)
    {
        return double(tan(double(a)));
    }

    double __C_math_asin_double_double(T, double a)
    {
        return double(asin(double(a)));
    }

    double __C_math_acos_double_double(T, double a)
    {
        return double(acos(double(a)));
    }

    double __C_math_atan_double_double(T, double a)
    {
        return double(atan(double(a)));
    }

    double __C_math_atan2_double_double_double(T, double a, double b)
    {
        return double(atan2(double(a), double(b)));
    }

    double __C_math_exp_double_double(T, double a)
    {
        return double(exp(double(a)));
    }

    // double __C_math_expm1_double_double(T, double a) { return
    // double(expm1(double(a))); }
    double __C_math_log_double_double(T, double a)
    {
        return double(log(double(a)));
    }

    double __C_math_log10_double_double(T, double a)
    {
        return double(log10(double(a)));
    }

    // double __C_math_log1p_double_double(T, double a) { return
    // double(log1p(double(a))); }
    double __C_math_sqrt_double_double(T, double a)
    {
        return double(sqrt(double(a)));
    }

    double __C_math_cbrt_double_double(T, double a)
    {
        return double(cbrt(double(a)));
    }

    double __C_math_floor_double_double(T, double a)
    {
        return double(floor(double(a)));
    }

    double __C_math_ceil_double_double(T, double a)
    {
        return double(ceil(double(a)));
    }

    // double __C_math_rint_double_double(T, double a) { return
    // double(rint(double(a))); }
    double __C_math_pow_double_double_double(T, double a, double b)
    {
        return double(pow(double(a), double(b)));
    }

    double __C_math_hypot_double_double_double(T, double a, double b)
    {
        return double(hypot(double(a), double(b)));
    }

#undef T

    void MathModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        //
        //  All of the functions are inlined in Native.h So here the
        //  NativeInlined attribute is set in for all function attributes.
        //

        Mapped |= NativeInlined;

        Context* c = context();

        addSymbols(
            new Function(c, "max", MathModule::max_f, Mapped, Compiled,
                         __C_math_max_float_float_float, Return, "float", Args,
                         "float", "float", End),

            new Function(c, "min", MathModule::min_f, Mapped, Compiled,
                         __C_math_min_float_float_float, Return, "float", Args,
                         "float", "float", End),

            new Function(c, "abs", MathModule::abs_f, Mapped, Compiled,
                         __C_math_abs_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "max", MathModule::max_i, Mapped, Compiled,
                         __C_math_max_int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "min", MathModule::min_i, Mapped, Compiled,
                         __C_math_min_int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "abs", MathModule::abs_i, Mapped, Compiled,
                         __C_math_abs_int_int, Return, "int", Args, "int", End),

            new Function(c, "sin", MathModule::sin, Mapped, Compiled,
                         __C_math_sin_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "cos", MathModule::cos, Mapped, Compiled,
                         __C_math_cos_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "tan", MathModule::tan, Mapped, Compiled,
                         __C_math_tan_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "asin", MathModule::asin, Mapped, Compiled,
                         __C_math_asin_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "acos", MathModule::acos, Mapped, Compiled,
                         __C_math_acos_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "atan", MathModule::atan, Mapped, Compiled,
                         __C_math_atan_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "atan2", MathModule::atan2, Mapped, Compiled,
                         __C_math_atan2_float_float_float, Return, "float",
                         Args, "float", "float", End),

            new Function(c, "exp", MathModule::exp, Mapped, Compiled,
                         __C_math_exp_float_float, Return, "float", Args,
                         "float", End),
            /*
                            new Function(c, "expm1", MathModule::expm1, Mapped,
                                         Compiled, __C_math_expm1_float_float,
                                         Return, "float",
                                         Args, "float", End),
            */
            new Function(c, "log", MathModule::log, Mapped, Compiled,
                         __C_math_log_float_float, Compiled, ::logf, Return,
                         "float", Args, "float", End),

            new Function(c, "log10", MathModule::log10, Mapped, Compiled,
                         __C_math_log10_float_float, Compiled, ::log10f, Return,
                         "float", Args, "float", End),
            /*
                            new Function(c, "log1p", MathModule::log1p, Mapped,
                                         Compiled, __C_math_log1p_float_float,
                                         Return, "float",
                                         Args, "float", End),
            */
            new Function(c, "sqrt", MathModule::sqrt, Mapped, Compiled,
                         __C_math_sqrt_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "inversesqrt", MathModule::inversesqrt, Mapped,
                         Compiled, __C_math_inversesqrt_float_float, Return,
                         "float", Args, "float", End),

            new Function(c, "cbrt", MathModule::cbrt, Mapped, Compiled,
                         __C_math_cbrt_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "floor", MathModule::floor, Mapped, Compiled,
                         __C_math_floor_float_float, Return, "float", Args,
                         "float", End),

            new Function(c, "ceil", MathModule::ceil, Mapped, Compiled,
                         __C_math_ceil_float_float, Return, "float", Args,
                         "float", End),
            /*
                            new Function(c, "rint", MathModule::rint, Mapped,
                                         Compiled, __C_math_rint_float_float,
                                         Return, "float",
                                         Args, "float", End),
            */
            new Function(c, "pow", MathModule::pow, Mapped, Compiled,
                         __C_math_pow_float_float_float, Return, "float", Args,
                         "float", "float", End),

            new Function(c, "hypot", MathModule::hypot, Mapped, Compiled,
                         __C_math_hypot_float_float_float, Return, "float",
                         Args, "float", "float", End),

            new Alias(c, "vec4f", "vector float[4]"),
            new Alias(c, "vec3f", "vector float[3]"),
            new Alias(c, "vec2f", "vector float[2]"),

            //
            //  These should become doubles when double is implemented
            //

            new SymbolicConstant(c, "pi", "double", Value(double(M_PI))),

#if 1
            new SymbolicConstant(c, "e", "double", Value(double(M_E))),
#endif

            EndArguments);

        addSymbols(
            new Function(c, "max", MathModule::max_d, Mapped, Compiled,
                         __C_math_max_double_double_double, Return, "double",
                         Args, "double", "double", End),

            new Function(c, "min", MathModule::min_d, Mapped, Compiled,
                         __C_math_min_double_double_double, Return, "double",
                         Args, "double", "double", End),

            new Function(c, "abs", MathModule::abs_d, Mapped, Compiled,
                         __C_math_abs_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "sin", MathModule::sin_d, Mapped, Compiled,
                         __C_math_sin_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "cos", MathModule::cos_d, Mapped, Compiled,
                         __C_math_cos_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "tan", MathModule::tan_d, Mapped, Compiled,
                         __C_math_tan_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "asin", MathModule::asin_d, Mapped, Compiled,
                         __C_math_asin_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "acos", MathModule::acos_d, Mapped, Compiled,
                         __C_math_acos_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "atan", MathModule::atan_d, Mapped, Compiled,
                         __C_math_atan_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "atan2", MathModule::atan2_d, Mapped, Compiled,
                         __C_math_atan2_double_double_double, Return, "double",
                         Args, "double", "double", End),

            new Function(c, "exp", MathModule::exp_d, Mapped, Compiled,
                         __C_math_exp_double_double, Return, "double", Args,
                         "double", End),
            /*
                            new Function(c, "expm1", MathModule::expm1, Mapped,
                                         Compiled, __C_math_expm1_double_double,
                                         Return, "double",
                                         Args, "double", End),
            */
            new Function(c, "log", MathModule::log_d, Mapped, Compiled,
                         __C_math_log_double_double, Compiled, ::logf, Return,
                         "double", Args, "double", End),

            new Function(c, "log10", MathModule::log10_d, Mapped, Compiled,
                         __C_math_log10_double_double, Compiled, ::log10f,
                         Return, "double", Args, "double", End),
            /*
                            new Function(c, "log1p", MathModule::log1p, Mapped,
                                         Compiled, __C_math_log1p_double_double,
                                         Return, "double",
                                         Args, "double", End),
            */
            new Function(c, "sqrt", MathModule::sqrt_d, Mapped, Compiled,
                         __C_math_sqrt_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "inversesqrt", MathModule::inversesqrt_d, Mapped,
                         Compiled, __C_math_inversesqrt_double_double, Return,
                         "double", Args, "double", End),

            new Function(c, "cbrt", MathModule::cbrt_d, Mapped, Compiled,
                         __C_math_cbrt_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "floor", MathModule::floor_d, Mapped, Compiled,
                         __C_math_floor_double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "ceil", MathModule::ceil_d, Mapped, Compiled,
                         __C_math_ceil_double_double, Return, "double", Args,
                         "double", End),
            /*
                            new Function(c, "rint", MathModule::rint, Mapped,
                                         Compiled, __C_math_rint_double_double,
                                         Return, "double",
                                         Args, "double", End),
            */
            new Function(c, "pow", MathModule::pow_d, Mapped, Compiled,
                         __C_math_pow_double_double_double, Return, "double",
                         Args, "double", "double", End),

            new Function(c, "hypot", MathModule::hypot_d, Mapped, Compiled,
                         __C_math_hypot_double_double_double, Return, "double",
                         Args, "double", "double", End),

            EndArguments

        );
    }

    NODE_IMPLEMENTATION(MathModule::inversesqrt, float)
    {
        NODE_RETURN(float(1.0 / ::sqrt(NODE_ARG(0, float))));
    }

    NODE_IMPLEMENTATION(MathModule::inversesqrt_d, double)
    {
        NODE_RETURN(1.0 / ::sqrt(NODE_ARG(0, double)));
    }

    NODE_IMPLEMENTATION(MathModule::abs_i, int)
    {
        const int a = NODE_ARG(0, int);
        NODE_RETURN(int(a > 0 ? a : -a));
    }

    NODE_IMPLEMENTATION(MathModule::max_i, int)
    {
        const int a = NODE_ARG(0, int);
        const int b = NODE_ARG(1, int);
        NODE_RETURN(int(a > b ? a : b));
    }

    NODE_IMPLEMENTATION(MathModule::min_i, int)
    {
        const int a = NODE_ARG(0, int);
        const int b = NODE_ARG(1, int);
        NODE_RETURN(int(a < b ? a : b));
    }

    NODE_IMPLEMENTATION(MathModule::abs_f, float)
    {
        const float a = NODE_ARG(0, float);
        NODE_RETURN(float(a > 0 ? a : -a));
    }

    NODE_IMPLEMENTATION(MathModule::max_f, float)
    {
        const float a = NODE_ARG(0, float);
        const float b = NODE_ARG(1, float);
        NODE_RETURN(float(a > b ? a : b));
    }

    NODE_IMPLEMENTATION(MathModule::min_f, float)
    {
        const float a = NODE_ARG(0, float);
        const float b = NODE_ARG(1, float);
        NODE_RETURN(float(a < b ? a : b));
    }

    NODE_IMPLEMENTATION(MathModule::abs_d, double)
    {
        const double a = NODE_ARG(0, double);
        NODE_RETURN(double(a > 0 ? a : -a));
    }

    NODE_IMPLEMENTATION(MathModule::max_d, double)
    {
        const double a = NODE_ARG(0, double);
        const double b = NODE_ARG(1, double);
        NODE_RETURN(double(a > b ? a : b));
    }

    NODE_IMPLEMENTATION(MathModule::min_d, double)
    {
        const double a = NODE_ARG(0, double);
        const double b = NODE_ARG(1, double);
        NODE_RETURN(double(a < b ? a : b));
    }

#define MATHFUNC2(NAME, FUNC)                \
    NODE_IMPLEMENTATION(NAME, float)         \
    {                                        \
        const double a = NODE_ARG(0, float); \
        const double b = NODE_ARG(1, float); \
        NODE_RETURN(float(FUNC(a, b)));      \
    }

    MATHFUNC2(MathModule::atan2, ::atan2)
    MATHFUNC2(MathModule::pow, ::pow)
    MATHFUNC2(MathModule::hypot, ::hypot)

#define MATHFUNC(name, mathfunc)                                  \
    NODE_IMPLEMENTATION(name, float)                              \
    {                                                             \
        NODE_RETURN(float(mathfunc(double(NODE_ARG(0, float))))); \
    }

    MATHFUNC(MathModule::sin, ::sin)
    MATHFUNC(MathModule::cos, ::cos)
    MATHFUNC(MathModule::tan, ::tan)
    MATHFUNC(MathModule::asin, ::asin)
    MATHFUNC(MathModule::acos, ::acos)
    MATHFUNC(MathModule::atan, ::atan)
    MATHFUNC(MathModule::exp, ::exp)
    // MATHFUNC(MathModule::expm1,::expm1)
    MATHFUNC(MathModule::log, ::log)
    MATHFUNC(MathModule::log10, ::log10)
    // MATHFUNC(MathModule::log1p,::log1p)
    MATHFUNC(MathModule::sqrt, ::sqrt)
    MATHFUNC(MathModule::cbrt, ::cbrt)
    MATHFUNC(MathModule::floor, ::floor)
    MATHFUNC(MathModule::ceil, ::ceil)
    // MATHFUNC(MathModule::rint,::rint)

#define MATHFUNC2_D(NAME, FUNC)               \
    NODE_IMPLEMENTATION(NAME, double)         \
    {                                         \
        const double a = NODE_ARG(0, double); \
        const double b = NODE_ARG(1, double); \
        NODE_RETURN(FUNC(a, b));              \
    }

    MATHFUNC2_D(MathModule::atan2_d, ::atan2)
    MATHFUNC2_D(MathModule::pow_d, ::pow)
    MATHFUNC2_D(MathModule::hypot_d, ::hypot)

#define MATHFUNC_D(name, mathfunc)                  \
    NODE_IMPLEMENTATION(name, double)               \
    {                                               \
        NODE_RETURN(mathfunc(NODE_ARG(0, double))); \
    }

    MATHFUNC_D(MathModule::sin_d, ::sin)
    MATHFUNC_D(MathModule::cos_d, ::cos)
    MATHFUNC_D(MathModule::tan_d, ::tan)
    MATHFUNC_D(MathModule::asin_d, ::asin)
    MATHFUNC_D(MathModule::acos_d, ::acos)
    MATHFUNC_D(MathModule::atan_d, ::atan)
    MATHFUNC_D(MathModule::exp_d, ::exp)
    // MATHFUNC(MathModule::expm1,::expm1)
    MATHFUNC_D(MathModule::log_d, ::log)
    MATHFUNC_D(MathModule::log10_d, ::log10)
    // MATHFUNC(MathModule::log1p,::log1p)
    MATHFUNC_D(MathModule::sqrt_d, ::sqrt)
    MATHFUNC_D(MathModule::cbrt_d, ::cbrt)
    MATHFUNC_D(MathModule::floor_d, ::floor)
    MATHFUNC_D(MathModule::ceil_d, ::ceil)
    // MATHFUNC(MathModule::rint,::rint)

#undef MATHFUNC

} // namespace Mu
