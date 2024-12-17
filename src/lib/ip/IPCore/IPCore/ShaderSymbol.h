//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ShaderSymbol__h__
#define __IPCore__ShaderSymbol__h__
#include <IPCore/ShaderValues.h>
#include <TwkGLF/GL.h>
#include <stl_ext/replace_alloc.h>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <sstream>

namespace TwkFB
{
    class FrameBuffer;
}

namespace IPCore
{
    class IPImage;

    namespace Shader
    {

        //
        //  The Shader::Symbol, Shader::Function, Shader::Program, and
        //  Shader::Expression types together form a metalanguage which is used
        //  to assemble the final Shader::Program. The metalanguage is primarily
        //  being used to stich together known functions in the source
        //  languages.
        //
        //  Basic code transformations can be applied to the metalanguage. For
        //  example one local transformation is detecting a 1/y gamma followed
        //  by a y gamma (resulting in a no-op, see ShaderCommon.cpp). A
        //  example global transformation is merging mutiple render-pass
        //  shader expressions into one larger one (this could be thought of
        //  as inlining).
        //
        //  Finally, the metalanguage is converted into the target language
        //  (GLSL, OpenCL) and that is passed to the actual device.
        //

        struct FunctionGLState;

        //
        //  class Shader::Symbol
        //
        //  Holds information about a symbol associated with a shader or shader
        //  function. Currently, this is either a global uniform variable or a
        //  function paramter. The GL type, array size (or 0 if its not an
        //  array) and the name are stored with the Symbol.
        //
        //  Symbols are static and created once.These are not constructed on the
        //  fly during evaluation.
        //
        //  TODO: add a default value? This would require a template sub-class
        //
        //  Symbols use reference counting with a starting reference of 1
        //
        //  Some of these objects (BoundSymbol, Expression) are using
        //  the replacement_allocator in order to prevent serialization during
        //  allocation. These objects are created frequently by the
        //  evaluation/caching threads and on Windows that's the kiss of death
        //  for threading if you use the standard allocator.
        //

        class Symbol
        {
        public:
            //
            //  Types
            //

            enum Qualifier
            {
                //
                //  These correspond to type qualifiers in GLSL.
                //
                //  NOTE: "varying" and "attribute" are not really represented
                //  here. *Parameter* types are all "varying". Or in terms of
                //  GLSL > 1.2 these are "out"
                //
                //  GLSL doesn't seem to have uniform as a parameter
                //  qualifier. Not sure what that means exactly; for example
                //  does it expand uniform values to varying when passing to a
                //  function?
                //

                Uniform,          // varies per-image
                Constant,         // never changes
                ParameterIn,      // input only parameter
                ParameterConstIn, // input only parameter, not mutable
                ParameterOut,     // output only parameter
                ParameterInOut,   // output and input parameter

                Special = 0x1000,
                Primary = 0x2000,

                SpecialParameterConstIn = ParameterConstIn | Special
            };

            enum Type
            {
                //
                //  These are types representing built-in GLSL types and those
                //  declared by global.glsl. Not all built-in types are included
                //  (yet). Only types that we know how to pass to the shader are
                //  included. In order to add a type here, its also necessary to
                //  add the assembly mechanism for the program and the
                //  underlying code that passes the value to the program.
                //
                //  built-in types
                //  --------------
                //

                VoidType, // degenerate
                FloatType,
                Vec2fType,
                Vec3fType,
                Vec4fType,
                IntType,
                Vec2iType,
                Vec3iType,
                Vec4iType,
                BoolType,
                Vec2bType,
                Vec3bType,
                Vec4bType,
                Matrix4fType,
                Matrix3fType,
                Matrix2fType,
                FloatArrayType,
                IntArrayType,
                BoolArrayType,
                Sampler1DType,
                Sampler2DType,
                Sampler2DRectType,
                Sampler3DType,
                Coord2DRectType,
                InputImageType,
                OutputImageType,
            };

            Symbol(Qualifier c, const std::string& name, Type t,
                   size_t width = 0)
                : m_qualifier(c)
                , m_name(name)
                , m_type(t)
                , m_width(width)
            {
            }

            ~Symbol() {}

            const std::string& name() const { return m_name; }

            Qualifier qualifier() const
            {
                return (Qualifier)(m_qualifier & ~Special);
            }

            Type type() const { return m_type; }

            size_t width() const { return m_width; }

            const char* glslTypeName() const;
            const char* glslQualifierName() const;

            bool isSpecial() const { return m_qualifier & Special; }

            bool isPrimary() const { return m_qualifier & Primary; }

            bool isSampler() const
            {
                return m_type == Sampler1DType || m_type == Sampler2DType
                       || m_type == Sampler3DType
                       || m_type == Sampler2DRectType;
            }

            bool isCoordinate() const { return m_type == Coord2DRectType; }

            bool isOutputImage() const { return m_type == OutputImageType; }

            bool isInputImage() const { return m_type == InputImageType; }

            bool operator==(const Symbol& s) const
            {
                return s.m_name == m_name && s.m_qualifier == m_qualifier
                       && s.m_type == m_type && s.m_width == m_width;
            }

        private:
            std::string m_name;
            Qualifier m_qualifier;
            Type m_type;
            size_t m_width;
        };

        typedef std::vector<const Symbol*> SymbolVector;

        class Expression;

        //
        //  class Shader::BoundSymbol
        //
        //  This class is the base class for a value of a symbol. It doesn't
        //  actually hold the value -- the template sub-class does.  SEE BELOW
        //

        class BoundSymbol
        {
        public:
            static void* operator new(size_t s);
            static void operator delete(void* p, size_t s);

            explicit BoundSymbol(const Symbol* p)
                : m_symbol(p)
                , m_isExpression(false)
            {
            }

            virtual ~BoundSymbol() {}

            const Symbol* symbol() const { return m_symbol; }

            virtual BoundSymbol* copy() const = 0;
            virtual void* valuePointer() = 0;
            virtual void output(std::ostream&) const = 0;
            virtual void outputHash(std::ostream&) const = 0;

            bool isExpression() const { return m_isExpression; }

        protected:
            bool m_isExpression;

        private:
            const Symbol* m_symbol;
        };

        //
        //  class Shader::ProxyBoundSymbol
        //
        //

        class ProxyBoundSymbol : public BoundSymbol
        {
        public:
            typedef ProxyBoundSymbol ThisType;

            explicit ProxyBoundSymbol(const Symbol* p)
                : BoundSymbol(p)
            {
            }

            virtual ~ProxyBoundSymbol() {}

            virtual BoundSymbol* copy() const { return new ThisType(symbol()); }

            virtual void* valuePointer() { return 0; }

            virtual void output(std::ostream& o) const
            {
                o << "[proxy " << symbol()->glslTypeName() << "]";
            }

            virtual void outputHash(std::ostream& o) const
            {
                o << "[proxy!" << symbol()->glslTypeName() << "]";
            }
        };

        //
        //  class Shader::TypedBoundSymbol<>
        //
        //  Holds an actual value for a given symbol. These are created during
        //  evaluation to pass values to the renderer.
        //
        //  NOTE: for bool type use an int for the type here
        //

        template <typename T> class TypedBoundSymbol : public BoundSymbol
        {
        public:
            typedef TypedBoundSymbol<T> ThisType;

            explicit TypedBoundSymbol(const Symbol* p, const T& value = T(0))
                : BoundSymbol(p)
                , m_value(value)
            {
            }

            virtual ~TypedBoundSymbol() {}

            virtual BoundSymbol* copy() const
            {
                return new ThisType(symbol(), m_value);
            }

            const T& value() const { return m_value; }

            void* valuePointer() { return (void*)&m_value; }

            virtual void output(std::ostream& o) const { o << m_value; }

            virtual void outputHash(std::ostream& o) const
            {
                ::outputHashValue(o, m_value);
            }

        private:
            T m_value;
        };

        struct ImageOrFB
        {
            ImageOrFB()
                : fb(0)
                , image(0)
                , plane(0)
            {
            }

            ImageOrFB(const TwkFB::FrameBuffer* f, size_t p = 0)
                : fb(f)
                , image(0)
                , plane(p)
            {
            }

            explicit ImageOrFB(const IPImage* i)
                : fb(0)
                , image(i)
                , plane(0)
            {
            }

            explicit ImageOrFB(const IPImage* img, const TwkFB::FrameBuffer* f,
                               size_t p = 0)
                : image(img)
                , fb(f)
                , plane(p)
            {
            }

            const IPImage* image;
            const TwkFB::FrameBuffer* fb;
            size_t plane;
        };

        struct ImageCoord
        {
            ImageCoord()
                : image(0)
            {
            }

            explicit ImageCoord(const IPImage* i)
                : image(i)
            {
            }

            const IPImage* image;
        };

        struct ImageCoordName
        {
            explicit ImageCoordName()
                : name("UNINITIALIZED")
            {
            }

            explicit ImageCoordName(const std::string& s)
                : name(s)
            {
            }

            std::string name;
        };

        std::ostream& operator<<(std::ostream& o, const ImageOrFB& r);
        std::ostream& operator<<(std::ostream& o, const ImageCoord& r);
        std::ostream& operator<<(std::ostream& o, const ImageCoordName& r);

        typedef TypedBoundSymbol<ImageOrFB> BoundSampler;
        typedef TypedBoundSymbol<ImageCoord> BoundImageCoord;
        typedef TypedBoundSymbol<ImageCoordName> BoundImageCoordName;
        typedef TypedBoundSymbol<int> BoundSpecial;
        typedef TypedBoundSymbol<float> BoundFloat;
        typedef TypedBoundSymbol<int> BoundInt;
        typedef TypedBoundSymbol<int> BoundBool;
        typedef TypedBoundSymbol<TwkMath::Vec2f> BoundVec2f;
        typedef TypedBoundSymbol<TwkMath::Vec3f> BoundVec3f;
        typedef TypedBoundSymbol<TwkMath::Vec4f> BoundVec4f;
        typedef TypedBoundSymbol<TwkMath::Vec2i> BoundVec2i;
        typedef TypedBoundSymbol<TwkMath::Vec3i> BoundVec3i;
        typedef TypedBoundSymbol<TwkMath::Vec4i> BoundVec4i;
        typedef TypedBoundSymbol<TwkMath::Mat44f> BoundMat44f;
        typedef TypedBoundSymbol<TwkMath::Mat33f> BoundMat33f;
        typedef TypedBoundSymbol<TwkMath::Mat22f> BoundMat22f;

        //
        //  class BoundExpression
        //
        //  This type of BoundSymbol makes it possible to bind a function to a
        //  parameter. This results in an expression tree. Expression
        //  objects (other than "main") all return vec4f values so a
        //  BoundExpression can appear anywhere a Symbol::Vec4fType appears.
        //
        //  In the future this could be expanded if a Expression can
        //  return types other than vec4f values.
        //

        class BoundExpression : public BoundSymbol
        {
        public:
            BoundExpression(const Symbol* p, Expression* F);
            ~BoundExpression();
            virtual BoundSymbol* copy() const;

            Expression* value() const { return m_value; }

            void setValue(Expression* expr) { m_value = expr; }

            void* valuePointer() { return (void*)m_value; }

            virtual void output(std::ostream& o) const;
            virtual void outputHash(std::ostream& o) const;

        private:
            Expression* m_value;
        };

        enum DebuggingType
        {
            NoDebugInfo,
            ShaderCodeDebugInfo,
            AllDebugInfo
        };

        void setDebugging(DebuggingType);
        DebuggingType debuggingType();

        const char* glslNameOfQualifierType(Symbol::Qualifier);
        const char* glslNameOfSymbolType(Symbol::Type);

    } // namespace Shader
} // namespace IPCore

#endif // __IPCore__ShaderSymbol__h__
