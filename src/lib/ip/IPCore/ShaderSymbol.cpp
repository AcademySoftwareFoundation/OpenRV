//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/ShaderSymbol.h>
#include <IPCore/ShaderExpression.h>
#include <IPCore/ShaderState.h>
#include <IPCore/IPNode.h>
#include <IPCore/IPImage.h>
#include <TwkGLF/GL.h>
#include <TwkUtil/Timer.h>
#include <TwkFB/FrameBuffer.h>
#include <assert.h>
#include <set>
#include <sstream>
#include <iomanip>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/functional/hash.hpp>

namespace IPCore
{
    namespace Shader
    {
        using namespace std;
        using namespace TwkFB;
        using namespace boost;
        using namespace TwkUtil;

        volatile static DebuggingType debugging = NoDebugInfo;

        TWK_CLASS_NEW_DELETE(BoundSymbol)

        void setDebugging(DebuggingType t) { debugging = t; }

        DebuggingType debuggingType() { return debugging; }

        //
        //  template specializations
        //

        const char* glslNameOfQualifierType(Symbol::Qualifier q)
        {
            switch (q)
            {
            case Symbol::Uniform:
                return "uniform";
            case Symbol::Constant:
                return "const";
            case Symbol::ParameterIn:
                return "in";
            case Symbol::ParameterConstIn:
                return "const in";
            case Symbol::ParameterInOut:
                return "inout";
            default:
                return "MISSING QUALIFIER NAME";
            }
        }

        const char* Symbol::glslQualifierName() const
        {
            return glslNameOfQualifierType(qualifier());
        }

        const char* glslNameOfSymbolType(Symbol::Type t)
        {
            switch (t)
            {
            case Symbol::FloatType:
                return "float";
            case Symbol::Coord2DRectType:
            case Symbol::Vec2fType:
                return "vec2";
            case Symbol::Vec3fType:
                return "vec3";
            case Symbol::Vec4fType:
                return "vec4";
            case Symbol::IntType:
                return "int";
            case Symbol::Vec2iType:
                return "ivec2";
            case Symbol::Vec3iType:
                return "ivec3";
            case Symbol::Vec4iType:
                return "ivec4";
            case Symbol::BoolType:
                return "bool";
            case Symbol::Vec2bType:
                return "bvec2";
            case Symbol::Vec3bType:
                return "bvec3";
            case Symbol::Vec4bType:
                return "bvec4";
            case Symbol::Matrix4fType:
                return "mat4";
            case Symbol::Matrix3fType:
                return "mat3";
            case Symbol::Matrix2fType:
                return "mat2";
            case Symbol::FloatArrayType:
                return "float";
            case Symbol::IntArrayType:
                return "int";
            case Symbol::BoolArrayType:
                return "bool";
            case Symbol::Sampler1DType:
                return "sampler1D";
            case Symbol::Sampler2DType:
                return "sampler2D";
            case Symbol::Sampler2DRectType:
                return "sampler2DRect";
            case Symbol::Sampler3DType:
                return "sampler3D";
            case Symbol::InputImageType:
                return "*inputImage*";
            case Symbol::OutputImageType:
                return "*outputImage*";
            default:
                return "MISSING TYPE NAME";
            }
        }

        const char* Symbol::glslTypeName() const
        {
            return glslNameOfSymbolType(type());
        }

        //----------------------------------------------------------------------

        BoundExpression::BoundExpression(const Symbol* p, Expression* F)
            : BoundSymbol(p)
            , m_value(F)
        {
            m_isExpression = true;
            assert(F);
            assert(p->type() == Symbol::Vec4fType
                   || p->type() == Symbol::InputImageType);
        }

        BoundExpression::~BoundExpression() { delete m_value; }

        BoundSymbol* BoundExpression::copy() const
        {
            return new BoundExpression(symbol(), m_value->copy());
        }

        void BoundExpression::output(std::ostream& o) const
        {
            m_value->output(o);
        }

        void BoundExpression::outputHash(std::ostream& o) const
        {
            o << "{";
            m_value->outputHash(o);
            o << "}";
        }

        std::ostream& operator<<(std::ostream& o, const Shader::ImageOrFB& r)
        {
            if (r.image)
            {
                if (r.image->graphID() == "")
                {
                    o << "ImageOrFB[i](" << r.image->node->name() << ")";
                }
                else
                {
                    o << "ImageOrFB[i](" << r.image->graphID() << ")";
                }
            }
            else if (r.fb)
            {
                o << "ImageOrFB[fb](" << r.fb->identifier() << "," << r.plane
                  << ")";
            }

            return o;
        }

        std::ostream& operator<<(std::ostream& o, const Shader::ImageCoord& r)
        {
            o << "ImageCoord(" << r.image->node->name() << ")";
            return o;
        }

        std::ostream& operator<<(std::ostream& o,
                                 const Shader::ImageCoordName& r)
        {
            o << "ImageCoordName(" << r.name << ")";
            return o;
        }

    } // namespace Shader
} // namespace IPCore
