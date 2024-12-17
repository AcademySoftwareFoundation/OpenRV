#ifndef __Mu__Native__h__
#define __Mu__Native__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/BaseFunctions.h>
#include <Mu/FreeVariable.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionSpecializer.h>
#include <Mu/MachineRep.h>
#include <Mu/Module.h>
#include <Mu/Namespace.h>
#include <Mu/Node.h>
#include <Mu/Object.h>
#include <Mu/ParameterVariable.h>
#include <Mu/PartialApplicator.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <Mu/config.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/FixedArray.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/HalfType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/RegexType.h>
#include <MuLang/StringType.h>
#include <math.h>

//
//  Some the above headers are not referenced by this header, but are
//  necessary to compile the output cpp file.
//

#define MU_HEAP_ALLOC(T, C) (new T(C))
#define MU_NEW_STRING(S) \
    (((MuLangContext*)NODE_THREAD.context())->stringType()->allocate(S))
#define MU_NEW_REGEX(S, F)                                                     \
    (new Mu::RegexType::Regex(                                                 \
        ((MuLangContext*)NODE_THREAD.context())->stringType(), NODE_THREAD, S, \
        F))
#define MU_NEW_FUNCTIONOBJ(T) (new Mu::FunctionObject(T))

#define MU_ARENA_ALLOC_OPERATORS()                 \
    static void* operator new(size_t s)            \
    {                                              \
        return Object::arena().allocate(s);        \
    }                                              \
    static void operator delete(void* p, size_t s) \
    {                                              \
        Object::arena().deallocate(p, s);          \
    }

// #define MU_ALLOC_INSTANCE(T,N,D) (ClassInstance::allocate(T,N,D))

//
//  Some types in mangled form
//

#define string_RT Mu::StringType::String

//
//  USING DIRECT MACRO EXPANSION
//
//  The "T" argument is the thread. This is ignored by these macros.
//  In gcc 4.0 this makes a big performance difference compared to the
//  inline functions -- the compiler does not elide the thread
//  parameter in such a way as to generate identical code to the macro
//  expansion.
//
//  In addition, it looks like even at -03 the inline stubs tend to
//  cause a slow down. This may be the result of cache hits, but
//  that's just a huge guess at this point.
//
//  In cases where the macro would cause multiple evaluation, an
//  inline function is used instead. Correspondingly, the interpreter
//  versions have been made to no longer use lazy evaluation of
//  arguments so that the behavior is consistant.
//

//  ---- constructors ----

#define float_float_int(T, A) (float(A))
#define float_float_int64(T, A) (float(A))
#define float_float_short(T, A) (float(A))
#define float_float_char(T, A) (float(A))
#define float_float_byte(T, A) (float(A))
#define float_float_floatAmp_(T, A) (A)
#define float_float_half(T, a) (float(shortToHalf(a)))

#define half_half_int(T, a) (halfToShort(float(a)))
#define half_half_int64(T, a) (halfToShort(float(a)))
#define half_half_float(T, a) (halfToShort(half(a)))

#define int_int_float(T, A) (int(A))
#define int_int_double(T, A) (int(A))
#define int_int_int64(T, A) (int(A))
#define int_int_short(T, A) (int(A))
#define int_int_char(T, A) (int(A))
#define int_int_byte(T, A) (int(A))
#define int_int_intAmp_(T, A) (A)

#define int64_int64_int(T, A) (int64(A))
#define int64_int64_float(T, A) (int64(A))
#define int64_int64_short(T, A) (int64(A))
#define int64_int64_char(T, A) (int64(A))
#define int64_int64_byte(T, A) (int64(A))
#define int64_int64_int64Amp_(T, A) (A)

#define short_short_float(T, A) (short(A))
#define short_short_int64(T, A) (short(A))
#define short_short_int(T, A) (short(A))
#define short_short_char(T, A) (short(A))
#define short_short_byte(T, A) (short(A))
#define short_short_shortAmp_(T, A) (A)

#define byte_byte_float(T, A) (byte(A))
#define byte_byte_int64(T, A) (byte(A))
#define byte_byte_int(T, A) (byte(A))
#define byte_byte_char(T, A) (byte(A))
#define byte_byte_short(T, A) (byte(A))
#define byte_byte_byteAmp_(T, A) (A)

#define char_char_int(T, A) (A)
#define char_char_int64(T, A) (A)
#define char_char_short(T, A) (int(A))
#define char_char_byte(T, A) (int(A))
#define char_char_charAmp_(T, A) (A)

// ----- void type ------

// not sure why this shows up, but its a no-op
#define void_void_QMark_(T, A)

// ----- bool type ------

#define Bang__bool_bool(T, A) (!(A))
#define Amp_Amp__bool_bool_bool(T, A, B) ((A) && (B))
#define Pipe_Pipe__bool_bool_bool(T, A, B) ((A) || (B))
#define EQ_EQ__bool_bool_bool(T, A, B) ((A) == (B))
#define Bang_EQ__bool_bool_bool(T, A, B) ((A) != (B))
#define EQ__boolAmp__boolAmp__bool(T, A, B) ((A) = (B))
#define eq_QMark_non_primitive_or_nil_QMark_non_primitive_or_nil(T, A, B) \
    ((A) == (B))
#define neq_QMark_non_primitive_or_nil_QMark_non_primitive_or_nil(T, A, B) \
    ((A) != (B))
#define assert_void_bool(T, A)              \
    {                                       \
        if (!(A))                           \
            throw_assertion_failure(T, #A); \
    }
#define EQ__QMark__QMark_non_primitive_reference_QMark_non_primitive_or_nil( \
    T, A, B)                                                                 \
    ((A) = (B))

namespace Mu
{
    void throw_assertion_failure(Mu::Thread& NODE_THREAD, const char*);
}

// ----- float Type ------

#define Plus__float_float_float(T, A, B) ((A) + (B))
#define Minus__float_float_float(T, A, B) ((A) - (B))
#define Star__float_float_float(T, A, B) ((A) * (B))
#define Slash__float_float_float(T, A, B) ((A) / (B))
#define PCent__float_float_float(T, A, B) (float(::mod(double(A), double(B))))
#define Caret__float_float_float(T, A, B) (float(::pow(double(A), double(B))))
#define GT__bool_float_float(T, A, B) ((A) > (B))
#define LT__bool_float_float(T, A, B) ((A) < (B))
#define GT_EQ__bool_float_float(T, A, B) ((A) >= (B))
#define LT_EQ__bool_float_float(T, A, B) ((A) <= (B))
#define Minus__float_float(T, A) (-(A))
#define EQ__floatAmp__floatAmp__float(T, A, B) ((A) = (B))
#define Plus_EQ__floatAmp__floatAmp__float(T, A, B) ((A) += (B))
#define Minus_EQ__floatAmp__floatAmp__float(T, A, B) ((A) -= (B))
#define Star_EQ__floatAmp__floatAmp__float(T, A, B) ((A) *= (B))
#define Slash_EQ__floatAmp__floatAmp__float(T, A, B) ((A) /= (B))
#define PCent_EQ__floatAmp__floatAmp__float(T, A, B) \
    ((A) = float(::mod(double(A), double(B))))
#define Caret_EQ__floatAmp__floatAmp__float(T, A, B) \
    ((A) = float(::pow(double(A), double(B))))
#define prePlus_Plus__float_floatAmp_(T, A) (++(A))
#define postPlus_Plus__float_floatAmp_(T, A) ((A)++)
#define preMinus_Minus__float_floatAmp_(T, A) (--(A))
#define postMinus_Minus__float_floatAmp_(T, A) ((A)--)
#define EQ_EQ__bool_float_float(T, A, B) ((A) == (B))
#define Bang_EQ__bool_float_float(T, A, B) ((A) != (B))

// ----- half Type ------

#define Plus__half_half_half(T, A, B) \
    halfToShort(shortToHalf(A) + shortToHalf(B))
#define Minus__half_half_half(T, A, B) \
    halfToShort(shortToHalf(A) - shortToHalf(B))
#define Star__half_half_half(T, A, B) \
    halfToShort(shortToHalf(A) * shortToHalf(B))
#define Slash__half_half_half(T, A, B) \
    halfToShort(shortToHalf(A) / shortToHalf(B))
#define PCent__half_half_half(T, A, B) \
    halfToShort(half(::mod(floatshortToHalf(A), floatshortToHalf(B))))
#define Caret__half_half_half(T, A, B) \
    halfToShort(half(::pow(floatshortToHalf(A), floatshortToHalf(B))))
#define GT__bool_half_half(T, A, B) (shortToHalf(A) > shortToHalf(B))
#define LT__bool_half_half(T, A, B) (shortToHalf(A) < shortToHalf(B))
#define GT_EQ__bool_half_half(T, A, B) (shortToHalf(A) >= shortToHalf(B))
#define LT_EQ__bool_half_half(T, A, B) (shortToHalf(A) <= shortToHalf(B))
#define Minus__half_half(T, A) halfToShort(-shortToHalf(A))
#define EQ__halfAmp__halfAmp__half(T, A, B) (shortToHalf(A) = shortToHalf(B))
#define Plus_EQ__halfAmp__halfAmp__half(T, A, B) \
    ((A) = shortToHalf(A) + shortToHalf(B))
#define Minus_EQ__halfAmp__halfAmp__half(T, A, B) \
    ((A) = shortToHalf(A) - shortToHalf(B))
#define Star_EQ__halfAmp__halfAmp__half(T, A, B) \
    ((A) = shortToHalf(A) * shortToHalf(B))
#define Slash_EQ__halfAmp__halfAmp__half(T, A, B) \
    ((A) = shortToHalf(A) / shortToHalf(B))
#define PCent_EQ__halfAmp__halfAmp__half(T, A, B) \
    ((A) = half(::mod(float(shortToHalf(A)), float(shortToHalf(B)))))
#define Caret_EQ__halfAmp__halfAmp__half(T, A, B) \
    (shortToHalf(A) = half(::pow(doubleshortToHalf(A), doubleshortToHalf(B))))
#define prePlus_Plus__half_halfAmp_(T, A) halfToShort(++shortToHalf(A))
#define postPlus_Plus__half_halfAmp_(T, A) halfToShort(shortToHalf(A)++)
#define preMinus_Minus__half_halfAmp_(T, A) halfToShort(--shortToHalf(A))
#define postMinus_Minus__half_halfAmp_(T, A) halfToShort(shortToHalf(A)--)
#define EQ_EQ__bool_half_half(T, A, B) (shortToHalf(A) == shortToHalf(B))
#define Bang_EQ__bool_half_half(T, A, B) (shortToHalf(A) != shortToHalf(B))

// ----- int Type ------

#define Plus__int_int_int(T, A, B) ((A) + (B))
#define Minus__int_int_int(T, A, B) ((A) - (B))
#define Star__int_int_int(T, A, B) ((A) * (B))
#define Slash__int_int_int(T, A, B) ((A) / (B))
#define PCent__int_int_int(T, A, B) ((A) % (B))
#define Caret__int_int_int(T, A, B) ((A) ^ (B))
#define Amp__int_int_int(T, A, B) ((A) & (B))
#define Pipe__int_int_int(T, A, B) ((A) | (B))
#define GT_GT___int_int_int(T, A, B) ((A) >> (B))
#define LT_LT___int_int_int(T, A, B) ((A) << (B))
#define EQ_EQ__bool_int_int(T, A, B) ((A) == (B))
#define Bang_EQ__bool_int_int(T, A, B) ((A) != (B))
#define GT__bool_int_int(T, A, B) ((A) > (B))
#define LT__bool_int_int(T, A, B) ((A) < (B))
#define GT_EQ__bool_int_int(T, A, B) ((A) >= (B))
#define LT_EQ__bool_int_int(T, A, B) ((A) <= (B))
#define Minus__int_int(T, A) (-(A))
#define Tilde__int_int(T, A) (~(A))
#define EQ__intAmp__intAmp__int(T, A, B) ((A) = (B))
#define Plus_EQ__intAmp__intAmp__int(T, A, B) ((A) += (B))
#define Minus_EQ__intAmp__intAmp__int(T, A, B) ((A) -= (B))
#define Star_EQ__intAmp__intAmp__int(T, A, B) ((A) *= (B))
#define Slash_EQ__intAmp__intAmp__int(T, A, B) ((A) /= (B))
#define GT_GT_EQ__intAmp__intAmp__int(T, A, B) ((A) >>= (B))
#define LT_LT_EQ__intAmp__intAmp__int(T, A, B) ((A) <<= (B))
#define Caret_EQ__intAmp__intAmp__int(T, A, B) ((A) ^= (B))
#define Pipe_EQ__intAmp__intAmp__int(T, A, B) ((A) |= (B))
#define Amp_EQ__intAmp__intAmp__int(T, A, B) ((A) &= (B))
#define prePlus_Plus__int_intAmp_(T, A) (++(A))
#define postPlus_Plus__int_intAmp_(T, A) ((A)++)
#define preMinus_Minus__int_intAmp_(T, A) (--(A))
#define postMinus_Minus__int_intAmp_(T, A) ((A)--)

// ----- int64 Type ------

#define Plus__int64_int64_int64(T, A, B) ((A) + (B))
#define Minus__int64_int64_int64(T, A, B) ((A) - (B))
#define Star__int64_int64_int64(T, A, B) ((A) * (B))
#define Slash__int64_int64_int64(T, A, B) ((A) / (B))
#define PCent__int64_int64_int64(T, A, B) ((A) % (B))
#define Caret__int64_int64_int64(T, A, B) ((A) ^ (B))
#define Amp__int64_int64_int64(T, A, B) ((A) & (B))
#define Pipe__int64_int64_int64(T, A, B) ((A) | (B))
#define GT_GT___int64_int64_int64(T, A, B) ((A) >> (B))
#define LT_LT___int64_int64_int64(T, A, B) ((A) << (B))
#define EQ_EQ__bool_int64_int64(T, A, B) ((A) == (B))
#define Bang_EQ__bool_int64_int64(T, A, B) ((A) != (B))
#define GT__bool_int64_int64(T, A, B) ((A) > (B))
#define LT__bool_int64_int64(T, A, B) ((A) < (B))
#define GT_EQ__bool_int64_int64(T, A, B) ((A) >= (B))
#define LT_EQ__bool_int64_int64(T, A, B) ((A) <= (B))
#define Minus__int64_int64(T, A) (-(A))
#define Tilde__int64_int64(T, A) (~(A))
#define EQ__int64Amp__int64Amp__int64(T, A, B) ((A) = (B))
#define Plus_EQ__int64Amp__int64Amp__int64(T, A, B) ((A) += (B))
#define Minus_EQ__int64Amp__int64Amp__int64(T, A, B) ((A) -= (B))
#define Star_EQ__int64Amp__int64Amp__int64(T, A, B) ((A) *= (B))
#define Slash_EQ__int64Amp__int64Amp__int64(T, A, B) ((A) /= (B))
#define GT_GT_EQ__int64Amp__int64Amp__int64(T, A, B) ((A) >>= (B))
#define LT_LT_EQ__int64Amp__int64Amp__int64(T, A, B) ((A) <<= (B))
#define Caret_EQ__int64Amp__int64Amp__int64(T, A, B) ((A) ^= (B))
#define Pipe_EQ__int64Amp__int64Amp__int64(T, A, B) ((A) |= (B))
#define Amp_EQ__int64Amp__int64Amp__int64(T, A, B) ((A) &= (B))
#define prePlus_Plus__int64_int64Amp_(T, A) (++(A))
#define postPlus_Plus__int64_int64Amp_(T, A) ((A)++)
#define preMinus_Minus__int64_int64Amp_(T, A) (--(A))
#define postMinus_Minus__int64_int64Amp_(T, A) ((A)--)

// ----- short Type ------

#define Plus__short_short_short(T, A, B) ((A) + (B))
#define Minus__short_short_short(T, A, B) ((A) - (B))
#define Star__short_short_short(T, A, B) ((A) * (B))
#define Slash__short_short_short(T, A, B) ((A) / (B))
#define PCent__short_short_short(T, A, B) ((A) % (B))
#define Caret__short_short_short(T, A, B) ((A) ^ (B))
#define Amp__short_short_short(T, A, B) ((A) & (B))
#define Pipe__short_short_short(T, A, B) ((A) | (B))
#define GT_GT___short_short_short(T, A, B) ((A) >> (B))
#define LT_LT___short_short_short(T, A, B) ((A) << (B))
#define EQ_EQ__bool_short_short(T, A, B) ((A) == (B))
#define Bang_EQ__bool_short_short(T, A, B) ((A) != (B))
#define GT__bool_short_short(T, A, B) ((A) > (B))
#define LT__bool_short_short(T, A, B) ((A) < (B))
#define GT_EQ__bool_short_short(T, A, B) ((A) >= (B))
#define LT_EQ__bool_short_short(T, A, B) ((A) <= (B))
#define Minus__short_short(T, A) (-(A))
#define Tilde__short_short(T, A) (~(A))
#define EQ__shortAmp__shortAmp__short(T, A, B) ((A) = (B))
#define Plus_EQ__shortAmp__shortAmp__short(T, A, B) ((A) += (B))
#define Minus_EQ__shortAmp__shortAmp__short(T, A, B) ((A) -= (B))
#define Star_EQ__shortAmp__shortAmp__short(T, A, B) ((A) *= (B))
#define Slash_EQ__shortAmp__shortAmp__short(T, A, B) ((A) /= (B))
#define GT_GT_EQ__shortAmp__shortAmp__short(T, A, B) ((A) >>= (B))
#define LT_LT_EQ__shortAmp__shortAmp__short(T, A, B) ((A) <<= (B))
#define Caret_EQ__shortAmp__shortAmp__short(T, A, B) ((A) ^= (B))
#define Pipe_EQ__shortAmp__shortAmp__short(T, A, B) ((A) |= (B))
#define Amp_EQ__shortAmp__shortAmp__short(T, A, B) ((A) &= (B))
#define prePlus_Plus__short_shortAmp_(T, A) (++(A))
#define postPlus_Plus__short_shortAmp_(T, A) ((A)++)
#define preMinus_Minus__short_shortAmp_(T, A) (--(A))
#define postMinus_Minus__short_shortAmp_(T, A) ((A)--)

// ----- byte Type ------

#define Plus__byte_byte_byte(T, A, B) ((A) + (B))
#define Minus__byte_byte_byte(T, A, B) ((A) - (B))
#define Star__byte_byte_byte(T, A, B) ((A) * (B))
#define Slash__byte_byte_byte(T, A, B) ((A) / (B))
#define PCent__byte_byte_byte(T, A, B) ((A) % (B))
#define Caret__byte_byte_byte(T, A, B) ((A) ^ (B))
#define Amp__byte_byte_byte(T, A, B) ((A) & (B))
#define Pipe__byte_byte_byte(T, A, B) ((A) | (B))
#define GT_GT___byte_byte_byte(T, A, B) ((A) >> (B))
#define LT_LT___byte_byte_byte(T, A, B) ((A) << (B))
#define EQ_EQ__bool_byte_byte(T, A, B) ((A) == (B))
#define Bang_EQ__bool_byte_byte(T, A, B) ((A) != (B))
#define GT__bool_byte_byte(T, A, B) ((A) > (B))
#define LT__bool_byte_byte(T, A, B) ((A) < (B))
#define GT_EQ__bool_byte_byte(T, A, B) ((A) >= (B))
#define LT_EQ__bool_byte_byte(T, A, B) ((A) <= (B))
#define Minus__byte_byte(T, A) (-(A))
#define Tilde__byte_byte(T, A) (~(A))
#define EQ__byteAmp__byteAmp__byte(T, A, B) ((A) = (B))
#define Plus_EQ__byteAmp__byteAmp__byte(T, A, B) ((A) += (B))
#define Minus_EQ__byteAmp__byteAmp__byte(T, A, B) ((A) -= (B))
#define Star_EQ__byteAmp__byteAmp__byte(T, A, B) ((A) *= (B))
#define Slash_EQ__byteAmp__byteAmp__byte(T, A, B) ((A) /= (B))
#define GT_GT_EQ__byteAmp__byteAmp__byte(T, A, B) ((A) >>= (B))
#define LT_LT_EQ__byteAmp__byteAmp__byte(T, A, B) ((A) <<= (B))
#define Caret_EQ__byteAmp__byteAmp__byte(T, A, B) ((A) ^= (B))
#define Pipe_EQ__byteAmp__byteAmp__byte(T, A, B) ((A) |= (B))
#define Amp_EQ__byteAmp__byteAmp__byte(T, A, B) ((A) &= (B))
#define prePlus_Plus__byte_byteAmp_(T, A) (++(A))
#define postPlus_Plus__byte_byteAmp_(T, A) ((A)++)
#define preMinus_Minus__byte_byteAmp_(T, A) (--(A))
#define postMinus_Minus__byte_byteAmp_(T, A) ((A)--)

// ----- vector float[2] Type -----

#define Plus__vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_( \
    T, A, B)                                                                     \
    ((A) + (B))
#define Minus__vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_( \
    T, A, B)                                                                      \
    ((A) - (B))
#define Star__vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_( \
    T, A, B)                                                                     \
    ((A) * (B))
#define Slash__vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_( \
    T, A, B)                                                                      \
    ((A) / (B))
#define PCent__vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_( \
    T, A, B)                                                                      \
    ((A) % (B))
#define Caret__vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_( \
    T, A, B)                                                                      \
    ((A) ^ (B))
#define cross_vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_( \
    T, A, B)                                                                     \
    (cross((A), (B)))
#define normalize_vector_floatBSB_2ESB__vector_floatBSB_2ESB_(T, A) \
    (normalize(A))
#define dot_float_vector_floatBSB_2ESB__vector_floatBSB_2ESB_(T, A, B) \
    (dot((A), (B)))
#define mag_float_vector_floatBSB_2ESB_(T, A) (mag(A))
#define Minus__vector_floatBSB_2ESB__vector_floatBSB_2ESB_(T, A) (-(A))

#define EQ__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_( \
    T, A, B)                                                                           \
    ((A) = (B))
#define Plus_EQ__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_( \
    T, A, B)                                                                                \
    ((A) += (B))
#define Minus_EQ__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_( \
    T, A, B)                                                                                 \
    ((A) -= (B))
#define Star_EQ__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_( \
    T, A, B)                                                                                \
    ((A) *= (B))
#define Slash_EQ__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_( \
    T, A, B)                                                                                 \
    ((A) /= (B))
#define PCent_EQ__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_( \
    T, A, B)                                                                                 \
    ((A) %= (B))
#define Caret_EQ__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_Amp__vector_floatBSB_2ESB_( \
    T, A, B)                                                                                 \
    ((A) ^= (B))

// ----- vector float[3] Type -----

#define Plus__vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_( \
    T, A, B)                                                                     \
    ((A) + (B))
#define Minus__vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_( \
    T, A, B)                                                                      \
    ((A) - (B))
#define Star__vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_( \
    T, A, B)                                                                     \
    ((A) * (B))
#define Slash__vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_( \
    T, A, B)                                                                      \
    ((A) / (B))
#define PCent__vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_( \
    T, A, B)                                                                      \
    ((A) % (B))
#define Caret__vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_( \
    T, A, B)                                                                      \
    ((A) ^ (B))
#define cross_vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_( \
    T, A, B)                                                                     \
    (cross((A), (B)))
#define normalize_vector_floatBSB_3ESB__vector_floatBSB_3ESB_(T, A) \
    (normalize(A))
#define dot_float_vector_floatBSB_3ESB__vector_floatBSB_3ESB_(T, A, B) \
    (dot((A), (B)))
#define mag_float_vector_floatBSB_3ESB_(T, A) (mag(A))
#define Minus__vector_floatBSB_3ESB__vector_floatBSB_3ESB_(T, A) (-(A))

#define EQ__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_( \
    T, A, B)                                                                           \
    ((A) = (B))
#define Plus_EQ__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_( \
    T, A, B)                                                                                \
    ((A) += (B))
#define Minus_EQ__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_( \
    T, A, B)                                                                                 \
    ((A) -= (B))
#define Star_EQ__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_( \
    T, A, B)                                                                                \
    ((A) *= (B))
#define Slash_EQ__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_( \
    T, A, B)                                                                                 \
    ((A) /= (B))
#define PCent_EQ__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_( \
    T, A, B)                                                                                 \
    ((A) %= (B))
#define Caret_EQ__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_Amp__vector_floatBSB_3ESB_( \
    T, A, B)                                                                                 \
    ((A) ^= (B))

// ----- vector float[4] Type -----

#define Plus__vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_( \
    T, A, B)                                                                     \
    ((A) + (B))
#define Minus__vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_( \
    T, A, B)                                                                      \
    ((A) - (B))
#define Star__vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_( \
    T, A, B)                                                                     \
    ((A) * (B))
#define Slash__vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_( \
    T, A, B)                                                                      \
    ((A) / (B))
#define PCent__vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_( \
    T, A, B)                                                                      \
    ((A) % (B))
#define Caret__vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_( \
    T, A, B)                                                                      \
    ((A) ^ (B))
#define cross_vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_( \
    T, A, B)                                                                     \
    (cross((A), (B)))
#define normalize_vector_floatBSB_4ESB__vector_floatBSB_4ESB_(T, A) \
    (normalize((A)))
#define dot_float_vector_floatBSB_4ESB__vector_floatBSB_4ESB_(T, A, B) \
    (dot((A), (B)))
#define mag_float_vector_floatBSB_4ESB_(T, A) (mag((A)))
#define Minus__vector_floatBSB_4ESB__vector_floatBSB_4ESB_(T, A) (-(A))

#define EQ__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_( \
    T, A, B)                                                                           \
    ((A) = (B))
#define Plus_EQ__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_( \
    T, A, B)                                                                                \
    ((A) += (B))
#define Minus_EQ__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_( \
    T, A, B)                                                                                 \
    ((A) -= (B))
#define Star_EQ__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_( \
    T, A, B)                                                                                \
    ((A) *= (B))
#define Slash_EQ__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_( \
    T, A, B)                                                                                 \
    ((A) /= (B))
#define PCent_EQ__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_( \
    T, A, B)                                                                                 \
    ((A) %= (B))
#define Caret_EQ__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_Amp__vector_floatBSB_4ESB_( \
    T, A, B)                                                                                 \
    ((A) ^= (B))

// ----- math module -----

#define math_max_int_int_int(T, A, B) (std::max((A), (B)))
#define math_min_int_int_int(T, A, B) (std::min((A), (B)))
#define math_abs_int_int(T, A) (std::abs((A)))

#define math_max_float_float_float(T, A, B) (std::max((A), (B)))
#define math_min_float_float_float(T, A, B) (std::min((A), (B)))
#define math_abs_float_float(T, A) (std::abs((A)))
#define math_sin_float_float(T, A) (::sin((A)))
#define math_cos_float_float(T, A) (::cos((A)))
#define math_tan_float_float(T, A) (::tan((A)))
#define math_asin_float_float(T, A) (::asin((A)))
#define math_acos_float_float(T, A) (::acos((A)))
#define math_atan_float_float(T, A) (::atan((A)))
#define math_atan2_float_float_float(T, A, B) (::atan2((A), (B)))
#define math_exp_float_float(T, A) (::exp((A)))
#define math_expm1_float_float(T, A) (::expm1(A))
#define math_log_float_float(T, A) (::log((A)))
#define math_log10_float_float(T, A) (::log10((A)))
#define math_log1p_float_float(T, A) (::log1p((A)))
#define math_sqrt_float_float(T, A) (::sqrt((A)))
#define math_inversesqrt_float_float(T, A) (1.0 / ::sqrt((A)))
#define math_cbrt_float_float(T, A) (::cbrt((A)))
#define math_floor_float_float(T, A) (::floor((A)))
#define math_ceil_float_float(T, A) (::ceil((A)))
#define math_rint_float_float(T, A) (::rint((A)))
#define math_pow_float_float_float(T, A, B) (::pow((A), (B)))
#define math_hypot_float_float_float(T, A, B) (::hypot((A), (B)))

#define math_max_double_double_double(T, A, B) (std::max((A), (B)))
#define math_min_double_double_double(T, A, B) (std::min((A), (B)))
#define math_abs_double_double(T, A) (std::abs((A)))
#define math_sin_double_double(T, A) (::sin((A)))
#define math_cos_double_double(T, A) (::cos((A)))
#define math_tan_double_double(T, A) (::tan((A)))
#define math_asin_double_double(T, A) (::asin((A)))
#define math_acos_double_double(T, A) (::acos((A)))
#define math_atan_double_double(T, A) (::atan((A)))
#define math_atan2_double_double_double(T, A, B) (::atan2((A), (B)))
#define math_exp_double_double(T, A) (::exp((A)))
#define math_expm1_double_double(T, A) (::expm1(A))
#define math_log_double_double(T, A) (::log((A)))
#define math_log10_double_double(T, A) (::log10((A)))
#define math_log1p_double_double(T, A) (::log1p((A)))
#define math_sqrt_double_double(T, A) (::sqrt((A)))
#define math_inversesqrt_double_double(T, A) (1.0 / ::sqrt((A)))
#define math_cbrt_double_double(T, A) (::cbrt((A)))
#define math_floor_double_double(T, A) (::floor((A)))
#define math_ceil_double_double(T, A) (::ceil((A)))
#define math_rint_double_double(T, A) (::rint((A)))
#define math_pow_double_double_double(T, A, B) (::pow((A), (B)))
#define math_hypot_double_double_double(T, A, B) (::hypot((A), (B)))

// ------ math_util module -----
//
//  These functions are defined in MathUtilModulel.cpp
//

namespace Mu
{
    Mu::Vector3f rotate(Mu::Vector3f, Mu::Vector3f, float);
    Mu::Vector3f dnoise3(const Mu::Vector3f&);
    Mu::Vector2f dnoise2(const Mu::Vector2f&);
    float dnoise1(float);
    float noise3(const Mu::Vector3f&);
    float noise2(const Mu::Vector2f&);
    float noise1(float);
    Mu::Vector3f sphrand();
    float smoothstep(float, float, float);
    float linstep(float, float, float);
    float hermite(float, float, float, float, float);
    float extrand(float);
    float gauss(float);
}; // namespace Mu

#define math_util_rotate_vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_float( \
    T, A, B, C)                                                                                  \
    (Mu::rotate((A), (B), (C)))
#define math_util_dnoise3_vector_floatBSB_3ESB__vector_floatBSB_3ESB_(T, A) \
    (Mu::dnoise3((A)))
#define math_util_dnoise2_vector_floatBSB_2ESB__vector_floatBSB_2ESB_(T, A) \
    (Mu::dnoise2((A)))
#define math_util_dnoise1_float_float(T, A) (Mu::dnoise1((A)))
#define math_util_noise3_float_vector_floatBSB_3ESB_(T, A) (Mu::noise3((A)))
#define math_util_noise2_float_vector_floatBSB_2ESB_(T, A) (Mu::noise2((A)))
#define math_util_noise1_float_float(T, A) (Mu::noise1((A)))
#define math_util_sphrand_vector_floatBSB_3ESB_(T) (Mu::sphrand())
#define math_util_smoothstep_float_float_float_float(T, A, B, C) \
    (Mu::smoothstep((A), (B), (C)))
#define math_util_linstep_float_float_float_float(T, A, B, C) \
    (Mu::linstep((A), (B), (C)))
#define math_util_degrees_float_float(T, A) ((A) * 57.29578)
#define math_util_radians_float_float(T, A) ((A) * 0.017453293)
#define math_util_step_float_float_float(T, A, U) ((U) < (A) ? 0.0f : 1.0f)
#define math_util_hermite_float_float_float_float_float_float(T, A, B, C, D, \
                                                              E)             \
    (Mu::hermite((A), (B), (C), (D), (E)))
#define math_util_seed_void_int(T, A) (::srandom((A)))
#define math_util_gauss_float_float(T, A) (Mu::gauss((A)))
#define math_util_random_float_float(T, A) (Mu::extrand((A)))

//
//  The following are inlined instead of macros to prevent
//  multiple evaluation of the arguments
//

inline Mu::Vector4f
math_util_lerp_vector_floatBSB_4ESB__vector_floatBSB_4ESB__vector_floatBSB_4ESB_(
    Mu::Thread&, const Mu::Vector4f& a, const Mu::Vector4f& b, float t)
{
    return b * t + a * (1.0f - t);
}

inline Mu::Vector2f
math_util_lerp_vector_floatBSB_2ESB__vector_floatBSB_2ESB__vector_floatBSB_2ESB_(
    Mu::Thread&, const Mu::Vector2f& a, const Mu::Vector2f& b, float t)
{
    return b * t + a * (1.0f - t);
}

inline Mu::Vector3f
math_util_lerp_vector_floatBSB_3ESB__vector_floatBSB_3ESB__vector_floatBSB_3ESB_(
    Mu::Thread&, const Mu::Vector3f& a, const Mu::Vector3f& b, float t)
{
    return b * t + a * (1.0f - t);
}

inline float math_util_lerp_float_float_float_float(Mu::Thread&, float a,
                                                    float b, float t)
{
    return b * t + a * (1.0f - t);
}

inline float math_util_clamp_float_float_float_float(Mu::Thread&, float val,
                                                     float mn, float mx)
{
    return val < mn ? mn : (val > mx ? mx : val);
}

inline int math_util_random_int_int(Mu::Thread&, int div)
{
    /* AJG - this is very strange - rand is supposed to be good on win32? */
    /* How about rand_s? */
    // return (div ? int(::random()) % div : 0);
    return (div ? int(rand()) % div : 0);
}

// ----- char Type ------

#define Plus__char_char_char(T, A, B) ((A) + (B))
#define Minus__char_char_char(T, A, B) ((A) - (B))
#define EQ_EQ__bool_char_char(T, A, B) ((A) == (B))
#define GT__bool_char_char(T, A, B) ((A) > (B))
#define LT__bool_char_char(T, A, B) ((A) < (B))
#define GT_EQ__bool_char_char(T, A, B) ((A) >= (B))
#define LT_EQ__bool_char_char(T, A, B) ((A) <= (B))
#define EQ__charAmp__charAmp__char(T, A, B) ((A) = (B))
#define Plus_EQ__charAmp__charAmp__char(T, A, B) ((A) += (B))
#define Minus_EQ__charAmp__charAmp__char(T, A, B) ((A) -= (B))
#define prePlus_Plus__char_charAmp_(T, A) (++(A))
#define postPlus_Plus__char_charAmp_(T, A) ((A)++)
#define preMinus_Minus__char_charAmp_(T, A) (--(A))
#define postMinus_Minus__char_charAmp_(T, A) ((A)--)

// ------ string type ------
//
//  The string type is garbage collected (although immutable) so it
//  requires some special constructor signatures. These are defined in
//  the StringType.cpp file.
//

#define EQ__stringAmp__stringAmp__string(T, A, B) (*((Pointer*)(A)) = (B))

Mu::Pointer string_string(Mu::Thread&);
Mu::Pointer string_string_int(Mu::Thread&, int);
Mu::Pointer string_string_float(Mu::Thread&, float);
Mu::Pointer string_string_byte(Mu::Thread&, char);
Mu::Pointer string_string_bool(Mu::Thread&, bool);
Mu::Pointer string_string_string(Mu::Thread&, Mu::Pointer);
Mu::Pointer string_string_QMark_class_or_interface(Mu::Thread&, Mu::Pointer);
Mu::Pointer string_string_QMark_variant(Mu::Thread&, Mu::Pointer);
Mu::Pointer string_string_vector_floatBSB_4ESB_(Mu::Thread&, Mu::Vector4f);
Mu::Pointer string_string_vector_floatBSB_3ESB_(Mu::Thread&, Mu::Vector3f);
Mu::Pointer string_string_vector_floatBSB_2ESB_(Mu::Thread&, Mu::Vector2f);
Mu::Pointer Plus__string_string_string(Mu::Thread&, Mu::Pointer, Mu::Pointer);
Mu::Pointer Plus_EQ__stringAmp__stringAmp__string(Mu::Thread&, Mu::Pointer,
                                                  Mu::Pointer);

int int_int_string(Mu::Thread&, Mu::Pointer);
float float_float_string(Mu::Thread&, Mu::Pointer);
bool bool_bool_string(Mu::Thread&, Mu::Pointer);
bool EQ_EQ__bool_string_string(Mu::Thread&, Mu::Pointer, Mu::Pointer);
bool Bang_EQ__bool_string_string(Mu::Thread&, Mu::Pointer, Mu::Pointer);

// these are the format operator (%)
Mu::Pointer PCent__string_string_QMark_tuple(Mu::Thread&, Mu::Pointer,
                                             Mu::Pointer);
Mu::Pointer PCent__string_string_int(Mu::Thread&, Mu::Pointer, int);
Mu::Pointer PCent__string_string_float(Mu::Thread&, Mu::Pointer, float);
Mu::Pointer PCent__string_string_short(Mu::Thread&, Mu::Pointer, short);
Mu::Pointer PCent__string_string_char(Mu::Thread&, Mu::Pointer, int);
Mu::Pointer PCent__string_string_byte(Mu::Thread&, Mu::Pointer, char);
Mu::Pointer PCent__string_string_bool(Mu::Thread&, Mu::Pointer, bool);
Mu::Pointer PCent__string_string_vector_floatBSB_4ESB_(Mu::Thread&, Mu::Pointer,
                                                       Mu::Vector4f);
Mu::Pointer PCent__string_string_vector_floatBSB_3ESB_(Mu::Thread&, Mu::Pointer,
                                                       Mu::Vector3f);
Mu::Pointer PCent__string_string_vector_floatBSB_2ESB_(Mu::Thread&, Mu::Pointer,
                                                       Mu::Vector2f);
Mu::Pointer PCent__string_string_QMark_class_not_tuple(Mu::Thread& NODE_THREAD,
                                                       Mu::Pointer,
                                                       Mu::Pointer);

void print_void_string(Mu::Thread&, Mu::Pointer);

int string_size_int_string(Mu::Thread&, Mu::Pointer);
int string_hash_int_string(Mu::Thread&, Mu::Pointer);
int compare_int_string_string(Mu::Thread&, Mu::Pointer, Mu::Pointer);
Mu::Pointer string_substr_string_string_int_int(Mu::Thread&, Mu::Pointer, int,
                                                int);
int string_BSB_ESB__char_string_int(Mu::Thread&, Mu::Pointer, int);
Mu::Pointer string_split_stringBSB_ESB__string_string(Mu::Thread&, Mu::Pointer,
                                                      Mu::Pointer);

// ---- regex type ----
//
//

#define EQ__regexAmp__regexAmp__regex(T, A, B) (*((Pointer*)(A)) = (B))
Mu::Pointer regex_regex_string(Mu::Thread&, Mu::Pointer);
Mu::Pointer regex_regex_string_int(Mu::Thread&, Mu::Pointer, int);
bool regex_match_bool_regex_string(Mu::Thread&, Mu::Pointer, Mu::Pointer);
Mu::Pointer regex_smatch_stringBSB_ESB__regex_string(Mu::Thread&, Mu::Pointer,
                                                     Mu::Pointer);
void print_void_string(Mu::Thread&, Mu::Pointer);
Mu::Pointer regex_replace_string_regex_string_string(Mu::Thread&, Mu::Pointer,
                                                     Mu::Pointer, Mu::Pointer);

#endif // __Mu__CPPIntType__h__
