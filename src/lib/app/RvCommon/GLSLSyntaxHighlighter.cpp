//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/GLSLSyntaxHighlighter.h>

namespace
{

    const char* glslTypes[] = {"float",
                               "double",
                               "int",
                               "void",
                               "bool",
                               "true",
                               "false",
                               "mat2",
                               "mat3",
                               "mat4",
                               "dmat2",
                               "dmat3",
                               "dmat4",
                               "mat2x2",
                               "mat2x3",
                               "mat2x4",
                               "dmat2x2",
                               "dmat2x3",
                               "dmat2x4",
                               "mat3x2",
                               "mat3x3",
                               "mat3x4",
                               "dmat3x2",
                               "dmat3x3",
                               "dmat3x4",
                               "mat4x2",
                               "mat4x3",
                               "mat4x4",
                               "dmat4x2",
                               "dmat4x3",
                               "dmat4x4",
                               "vec2",
                               "vec3",
                               "vec4",
                               "ivec2",
                               "ivec3",
                               "ivec4",
                               "bvec2",
                               "bvec3",
                               "bvec4",
                               "dvec2",
                               "dvec3",
                               "dvec4",
                               "uint",
                               "uvec2",
                               "uvec3",
                               "uvec4",
                               "sampler1D",
                               "sampler2D",
                               "sampler3D",
                               "samplerCube",
                               "sampler1DShadow",
                               "sampler2DShadow",
                               "samplerCubeShadow",
                               "sampler1DArray",
                               "sampler2DArray",
                               "sampler1DArrayShadow",
                               "sampler2DArrayShadow",
                               "isampler1D",
                               "isampler2D",
                               "isampler3D",
                               "isamplerCube",
                               "isampler1DArray",
                               "isampler2DArray",
                               "usampler1D",
                               "usampler2D",
                               "usampler3D",
                               "usamplerCube",
                               "usampler1DArray",
                               "usampler2DArray",
                               "sampler2DRect",
                               "sampler2DRectShadow",
                               "isampler2DRect",
                               "usampler2DRect",
                               "samplerBuffer",
                               "isamplerBuffer",
                               "usamplerBuffer",
                               "sampler2DMS",
                               "isampler2DMS",
                               "usampler2DMS",
                               "sampler2DMSArray",
                               "isampler2DMSArray",
                               "usampler2DMSArray",
                               "samplerCubeArray",
                               "samplerCubeArrayShadow",
                               "isamplerCubeArray",
                               "usamplerCubeArray",
                               "image1D",
                               "iimage1D",
                               "uimage1D",
                               "image2D",
                               "iimage2D",
                               "uimage2D",
                               "image3D",
                               "iimage3D",
                               "uimage3D",
                               "image2DRect",
                               "iimage2DRect",
                               "uimage2DRect",
                               "imageCube",
                               "iimageCube",
                               "uimageCube",
                               "imageBuffer",
                               "iimageBuffer",
                               "uimageBuffer",
                               "image1DArray",
                               "iimage1DArray",
                               "uimage1DArray",
                               "image2DArray",
                               "iimage2DArray",
                               "uimage2DArray",
                               "imageCubeArray",
                               "iimageCubeArray",
                               "uimageCubeArray",
                               "image2DMS",
                               "iimage2DMS",
                               "uimage2DMS",
                               "image2DMSArray",
                               "iimage2DMSArray",
                               "uimage2DMSArray",
                               "long",
                               "short",
                               "half",
                               "fixed",
                               "unsigned",
                               "hvec2",
                               "hvec3",
                               "hvec4",
                               "fvec2",
                               "fvec3",
                               "fvec4",
                               "sampler3DRect",
                               0};

    const char* glslModifiers[] = {"attribute",
                                   "const",
                                   "uniform",
                                   "varying",
                                   "buffer",
                                   "shared",
                                   "coherent",
                                   "volatile",
                                   "restrict",
                                   "readonly",
                                   "writeonly",
                                   "atomic_uint",
                                   "layout",
                                   "centroid",
                                   "flat",
                                   "smooth",
                                   "noperspective",
                                   "patch",
                                   "sample",
                                   "break",
                                   "continue",
                                   "do",
                                   "for",
                                   "while",
                                   "switch",
                                   "case",
                                   "default",
                                   "if",
                                   "else",
                                   "subroutine",
                                   "in",
                                   "out",
                                   "inout",
                                   "invariant",
                                   "discard",
                                   "return",
                                   "lowp",
                                   "mediump",
                                   "highp",
                                   "precision",
                                   "struct",
                                   "common",
                                   "partition",
                                   "active",
                                   "asm",
                                   "class",
                                   "union",
                                   "enum",
                                   "typedef",
                                   "template",
                                   "this",
                                   "packed",
                                   "resource",
                                   "goto",
                                   "inline",
                                   "noinline",
                                   "public",
                                   "static",
                                   "extern",
                                   "external",
                                   "interface",
                                   "superp",
                                   "input",
                                   "output",
                                   "filter",
                                   "sizeof",
                                   "cast",
                                   "namespace",
                                   "using",
                                   "row_major",
                                   "early_fragment_tests",
                                   0};

    const char* glslBuiltIns[] = {"abs",
                                  "acos",
                                  "acosh",
                                  "all",
                                  "any",
                                  "asin",
                                  "asinh",
                                  "atan",
                                  "atanh",
                                  "atomicCounter",
                                  "atomicCounterDecrement",
                                  "atomicCounterIncrement",
                                  "barrier",
                                  "bitCount",
                                  "bitfieldExtract",
                                  "bitfieldInsert",
                                  "bitfieldReverse",
                                  "ceil",
                                  "clamp",
                                  "cos",
                                  "cosh",
                                  "cross",
                                  "degrees",
                                  "determinant",
                                  "dFdx",
                                  "dFdy",
                                  "distance",
                                  "dot",
                                  "EmitStreamVertex",
                                  "EmitVertex",
                                  "EndPrimitive",
                                  "EndStreamPrimitive",
                                  "equal",
                                  "exp",
                                  "exp2",
                                  "faceforward",
                                  "findLSB",
                                  "findMSB",
                                  "floatBitsToInt",
                                  "floatBitsToUint",
                                  "floor",
                                  "fma",
                                  "fract",
                                  "frexp",
                                  "fwidth",
                                  "greaterThan",
                                  "greaterThanEqual",
                                  "imageAtomicAdd",
                                  "imageAtomicAnd",
                                  "imageAtomicCompSwap",
                                  "imageAtomicExchange",
                                  "imageAtomicMax",
                                  "imageAtomicMin",
                                  "imageAtomicOr",
                                  "imageAtomicXor",
                                  "imageLoad",
                                  "imageSize",
                                  "imageStore",
                                  "imulExtended",
                                  "intBitsToFloat",
                                  "interpolateAtCentroid",
                                  "interpolateAtOffset",
                                  "interpolateAtSample",
                                  "inverse",
                                  "inversesqrt",
                                  "isinf",
                                  "isnan",
                                  "ldexp",
                                  "length",
                                  "lessThan",
                                  "lessThanEqual",
                                  "log",
                                  "log2",
                                  "matrixCompMult",
                                  "max",
                                  "memoryBarrier",
                                  "min",
                                  "mix",
                                  "mod",
                                  "modf",
                                  "noise",
                                  "normalize",
                                  "not",
                                  "notEqual",
                                  "outerProduct",
                                  "packDouble2x32",
                                  "packHalf2x16",
                                  "packSnorm2x16",
                                  "packSnorm4x8",
                                  "packUnorm2x16",
                                  "packUnorm4x8",
                                  "pow",
                                  "radians",
                                  "reflect",
                                  "refract",
                                  "round",
                                  "roundEven",
                                  "sign",
                                  "sin",
                                  "sinh",
                                  "smoothstep",
                                  "sqrt",
                                  "step",
                                  "tan",
                                  "tanh",
                                  "texelFetch",
                                  "texelFetchOffset",
                                  "texture",
                                  "textureGather",
                                  "textureGatherOffset",
                                  "textureGatherOffsets",
                                  "textureGrad",
                                  "textureGradOffset",
                                  "textureLod",
                                  "textureLodOffset",
                                  "textureOffset",
                                  "textureProj",
                                  "textureProjGrad",
                                  "textureProjGradOffset",
                                  "textureProjLod",
                                  "textureProjLodOffset",
                                  "textureProjOffset",
                                  "textureQueryLevels",
                                  "textureQueryLod",
                                  "textureSize",
                                  "transpose",
                                  "trunc",
                                  "uaddCarry",
                                  "uintBitsToFloat",
                                  "umulExtended",
                                  "unpackDouble2x32",
                                  "unpackHalf2x16",
                                  "unpackSnorm2x16",
                                  "unpackSnorm4x8",
                                  "unpackUnorm2x16",
                                  "unpackUnorm4x8",
                                  "usubBorrow",
                                  0};

    const char* glslPreProcessor[] = {
        "define", "undef", "if",     "ifdef",     "ifndef",  "else", "elif",
        "endif",  "error", "pragma", "extension", "version", "line", 0};

    const char* glslPreProcessorBuiltIns[] = {"__LINE__", "__FILE__",
                                              "__VERSION__", 0};

} // namespace

namespace Rv
{
    using namespace std;

    GLSLSyntaxHighlighter::GLSLSyntaxHighlighter(QTextDocument* parent)
        : QSyntaxHighlighter(parent)
    {
        HighlightingRule rule;

        // m_functionFormat.setFontWeight(QFont::Bold);
        m_functionFormat.setForeground(QColor(230, 230, 230));

        m_preProcessorFormat.setForeground(QColor(205, 80, 205));
        m_preProcessorFormat.setBackground(QColor(35, 35, 35));

        // m_typeFormat.setForeground(QColor(85, 26, 139));
        m_typeFormat.setForeground(QColor(155, 110, 208));
        // m_typeFormat.setFontWeight(QFont::Bold);

        // m_keywordFormat.setForeground(QColor(24, 116, 205));
        // m_keywordFormat.setForeground(QColor(90, 170, 215));
        m_keywordFormat.setForeground(QColor(120, 180, 215));
        // m_keywordFormat.setFontWeight(QFont::Bold);

        m_builtInFormat.setForeground(QColor(200, 90, 90));
        // m_builtInFormat.setFontWeight(QFont::Bold);

        m_imageFormat.setForeground(QColor(155, 110, 208));
        // m_imageFormat.setFontWeight(QFont::Bold);
        // m_imageFormat.setBackground(QColor(50, 50, 100));

        m_commentFormat.setForeground(QColor(127, 127, 127));
        m_quotationFormat.setForeground(Qt::green);

        rule.pattern = QRegExp("\\b[A-Za-z0-9_]+[ \t]*(?=\\()");
        rule.format = m_functionFormat;
        m_highlightingRules.append(rule);

        for (const char** p = glslTypes; *p; p++)
        {
            QString pattern = "\\b";
            pattern += *p;
            pattern += "\\b";
            rule.pattern = QRegExp(pattern);
            rule.format = m_typeFormat;
            m_highlightingRules.append(rule);
        }

        for (const char** p = glslModifiers; *p; p++)
        {
            QString pattern = "\\b";
            pattern += *p;
            pattern += "\\b";
            rule.pattern = QRegExp(pattern);
            rule.format = m_keywordFormat;
            m_highlightingRules.append(rule);
        }

        for (const char** p = glslBuiltIns; *p; p++)
        {
            QString pattern = "\\b";
            pattern += *p;
            pattern += "\\b";
            rule.pattern = QRegExp(pattern);
            rule.format = m_builtInFormat;
            m_highlightingRules.append(rule);
        }

        for (const char** p = glslPreProcessorBuiltIns; *p; p++)
        {
            QString pattern = "\\b";
            pattern += *p;
            pattern += "\\b";
            rule.pattern = QRegExp(pattern);
            rule.format = m_builtInFormat;
            m_highlightingRules.append(rule);
        }

        rule.pattern = QRegExp("\\b(?:inputImage|outputImage)\\b");
        rule.format = m_imageFormat;
        m_highlightingRules.append(rule);

        for (const char** p = glslPreProcessor; *p; p++)
        {
            QString pattern = "#[ \t]*";
            pattern += *p;
            pattern += "\\b";
            rule.pattern = QRegExp(pattern);
            rule.format = m_preProcessorFormat;
            m_highlightingRules.append(rule);
        }

        rule.pattern = QRegExp("//[^\n]*");
        rule.format = m_commentFormat;
        m_highlightingRules.append(rule);

        // GLSL doesn't have string literals
        // rule.pattern = QRegExp("\".*\"");
        // rule.format = m_quotationFormat;
        // m_highlightingRules.append(rule);

        m_commentStartExpression = QRegExp("/\\*");
        m_commentEndExpression = QRegExp("\\*/");
    }

    void GLSLSyntaxHighlighter::highlightBlock(const QString& text)
    {
        foreach (const HighlightingRule& rule, m_highlightingRules)
        {
            QRegExp expression(rule.pattern);
            int index = expression.indexIn(text);
            while (index >= 0)
            {
                int length = expression.matchedLength();
                setFormat(index, length, rule.format);
                index = expression.indexIn(text, index + length);
            }
        }

        setCurrentBlockState(0);

        int startIndex = 0;

        if (previousBlockState() != 1)
        {
            startIndex = m_commentStartExpression.indexIn(text);
        }

        while (startIndex >= 0)
        {
            int endIndex = m_commentEndExpression.indexIn(text, startIndex);
            int commentLength;

            if (endIndex == -1)
            {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            }
            else
            {
                commentLength = endIndex - startIndex
                                + m_commentEndExpression.matchedLength();
            }

            setFormat(startIndex, commentLength, m_commentFormat);
            startIndex = m_commentStartExpression.indexIn(
                text, startIndex + commentLength);
        }
    }

} // namespace Rv
