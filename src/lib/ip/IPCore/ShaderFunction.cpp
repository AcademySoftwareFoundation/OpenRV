//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/ShaderFunction.h>
#include <IPCore/ShaderState.h>
#include <IPCore/ShaderSymbol.h>
#include <IPCore/IPImage.h>
#include <TwkUtil/Timer.h>
#include <assert.h>
#include <set>
#include <sstream>
#include <iomanip>
#include <boost/algorithm/string.hpp>
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

        namespace
        {
            const char* global_glsl =
                "#version 150\n"
                "#extension GL_ARB_texture_rectangle : require\n";
            const char* global_glsl_gl2 =
                "#extension GL_ARB_texture_rectangle : require\n";
            const char* global_glsl_lt_150 = "#version 120\n";
        } // namespace

        struct SymbolTypeAssociation
        {
            const char* glslName;
            Symbol::Type type;
        };

        const SymbolTypeAssociation symbolTypeAssociations[] = {
            {"float", Symbol::FloatType},
            {"vec2", Symbol::Vec2fType},
            {"vec3", Symbol::Vec3fType},
            {"vec4", Symbol::Vec4fType},
            {"int", Symbol::IntType},
            {"ivec2", Symbol::Vec2iType},
            {"ivec3", Symbol::Vec3iType},
            {"ivec4", Symbol::Vec4iType},
            {"bool", Symbol::IntType},
            {"bvec2", Symbol::Vec2bType},
            {"bvec3", Symbol::Vec3bType},
            {"bvec4", Symbol::Vec4bType},
            {"mat4", Symbol::Matrix4fType},
            {"mat3", Symbol::Matrix3fType},
            {"mat2", Symbol::Matrix2fType},
            {"sampler1D", Symbol::Sampler1DType},
            {"sampler2D", Symbol::Sampler2DType},
            {"sampler2DRect", Symbol::Sampler2DRectType},
            {"sampler3D", Symbol::Sampler3DType},
            {"inputImage", Symbol::InputImageType},
            {"outputImage", Symbol::OutputImageType},
            {0, Symbol::VoidType}};

        namespace
        {
            unsigned int glslMajor = 0;
            unsigned int glslMinor = 0;

            void initGLSLVersion()
            {
                if (glslMajor == 0)
                {
                    //
                    //  NOTE: its much better to have assigned
                    if (const char* glVersion = (const char*)glGetString(
                            GL_SHADING_LANGUAGE_VERSION))
                    {
                        Shader::Function::useShadingLanguageVersion(glVersion);
                    }
                    else
                    {
                        cerr
                            << "ERROR: GL_SHADING_LANGUAGE_VERSION query failed"
                            << endl;
                        abort();
                    }
                }
            }

        } // namespace

        Function::FunctionVector Function::m_retiredFunctions;

        void Function::useShadingLanguageVersion(const char* glVersion)
        {
            vector<string> buffer;
            algorithm::split(buffer, glVersion, is_any_of(string(". ")),
                             token_compress_on);

            if (buffer.size() >= 2)
            {
                glslMajor = std::atoi(buffer[0].c_str());
                glslMinor = std::atoi(buffer[1].c_str());
            }
            else
            {
                cout << "ERROR: Shader::Function::useShadingLanguageVersion "
                        "failed"
                     << ", glVersion = \"" << glVersion << "\"" << endl;

                glslMajor = 0;
                glslMinor = 0;
            }
        }

        bool Function::isGLSLVersionLessThan150()
        {
            if (glslMajor > 1)
            {
                return false;
            }
            else
            {
                // Two edge case checks: check for 0 and check that minor
                // versions are multiples of 10. Kronos spec is that minor is
                // always a mult of 10. If not a multiple of 10; we abort and
                // keep the number as-is.
                unsigned int minor = glslMinor;
                if (glslMajor < 1)
                {
                    return true;
                }
                else if (glslMinor == 0 || glslMinor < 10)
                {
                    return glslMinor < 5;
                }
                else if (glslMinor % 10 != 0)
                {
                    // Intentional: always larger than 5.
                    return glslMinor < 5;
                }
                else
                {
                    // Here: exact multiple of 10
                    minor = glslMinor / 10;
                    return minor < 5;
                }
            }
        }

        Function::Function(const string& name, const string& glsl, Type type,
                           const SymbolVector& params,
                           const SymbolVector& globals, size_t numFetchesApprox,
                           const string& doc)
            : m_name(name)
            , m_type(type)
            , m_parameters(params)
            , m_globals(globals)
            , m_state(0)
            , m_doc(doc)
            , m_hash(0)
            , m_usesOutputSize(false)
            , m_usesOutputST(false)
            , m_inline(type == Filter || type == MorphologicalFilter)
        {
            initGLSLVersion();
            m_resourceUsage.fetches = numFetchesApprox;
            m_originalGLSL = glsl;
            m_sourceCode = removeComments(glsl);
            initHash();
            hashAuxNames();
            parseDeclaration(false);
            if (type == UndecidedType)
                inferType();
            replaceTextureCalls();
            initResourceUsage();
        }

        Function::Function(const string& name, const string& glsl, Type type,
                           size_t numFetchesApprox, const string& doc)
            : m_name(name)
            , m_type(type)
            , m_state(0)
            , m_doc(doc)
            , m_hash(0)
            , m_usesOutputSize(false)
            , m_usesOutputST(false)
            , m_inline(type == Filter || type == MorphologicalFilter)
        {
            initGLSLVersion();
            m_resourceUsage.fetches = numFetchesApprox;
            m_originalGLSL = glsl;
            m_sourceCode = removeComments(glsl);
            initHash();
            hashAuxNames();
            parseDeclaration(true);
            if (type == UndecidedType)
                inferType();
            replaceTextureCalls();
            initResourceUsage();
        }

        Function::~Function()
        {
            releaseCompiledState();
            for (size_t i = 0; i < m_parameters.size(); i++)
                delete m_parameters[i];
            for (size_t i = 0; i < m_globals.size(); i++)
                delete m_globals[i];
        }

        void Function::inferType()
        {
            //
            //  This function will infer the function type based on the
            //  results of parseDeclaration(). It looks at the number and use
            //  of the image inputs.
            //
            //  This function cannot distinguish between Color and LinearColor
            //  (which is fine since there's nothing to be done based on that
            //  distinction curently). Nor can it distinguish between Filter
            //  and MorphologicalFilter types.
            //
            //  A MorphologicalFilter in this case means that the function is
            //  using the derivative of the incoming ST to determine the
            //  number and pattern of sampling (its not fixed).
            //

            size_t nImages = m_inImageParameters.size();

            if (nImages == 0)
            {
                //
                //  No input images is a Source by definition
                //

                m_type = Source;
            }
            else
            {
                //
                //  If any input is being used as a sampler its a Filter
                //

                for (size_t i = 0; i < nImages; i++)
                {
                    if (m_inImageParameters[i].usesSampling)
                        m_type = Filter;
                }

                //
                //  If its not a filter it must be a Merge or Color type
                //

                if (m_type == UndecidedType)
                {
                    m_type = nImages > 1 ? Merge : Color;
                }
            }
        }

        void Function::initResourceUsage()
        {
            //
            //  Figure out how many buffers and coord this function is using
            //

            for (size_t i = 0; i < m_parameters.size(); i++)
            {
                const Symbol* s = m_parameters[i];
                if (s->isSampler())
                    m_resourceUsage.buffers++;
                if (s->isCoordinate())
                    m_resourceUsage.coords++;
            }
        }

        void Function::initHash()
        {
            //
            //  NOTE: m_hash should be the combined hash of source code from
            //  *all* languages. I.e. if this function has both a GLSL and
            //  OpenCL implementation than the hash should reflect the combined
            //  source code. This can be as simple as concatenating the source
            //  text together or using boost::hash_combine() (or TR1 if it
            //  exists) to do it.
            //
            //  Why use all languages? If we end up in a situation where there
            //  are hybrid or user selectable pipelines, or if a bug existed
            //  in only one of the language implementations it should be
            //  reflected everywhere.
            //
            //  The original source code is used for the hash so that any
            //  distinguishing comments (put there on purpose) will be
            //  included in the hash
            //

            boost::hash<string> string_hash;
            m_hash = string_hash(m_originalGLSL);

            if (m_type == Main)
            {
                m_callName = m_name;
            }
            else
            {
                ostringstream str;
                size_t nbytes = sizeof(m_hash) / 2;
                str << hex
                    << (((m_hash >> (nbytes * 8)) ^ m_hash) >> (nbytes * 8));
                m_hashString = str.str();
                m_callName = m_name + m_hashString;
            }
        }

        const Symbol::Type Function::returnType() const
        {
            switch (m_type)
            {
            case Main:
                return Symbol::VoidType;
            default:
                return Symbol::Vec4fType;
            }
        }

        bool Function::isCompiled() const { return m_state && m_state->shader; }

        void Function::releaseCompiledState() const
        {
            if (m_state)
            {
                if (m_state->shader)
                    glDeleteShader(m_state->shader);
                delete m_state;
                m_state = nullptr;
            }
        }

        void outputAnnotatedCode(ostream& o, const string& code)
        {
            o << endl;

            size_t count = 1;

            for (size_t i = 0, s = code.size(); i < s; i++)
            {
                if (code[i] == '\n' || i == 0)
                {
                    if (i)
                        o << endl;
                    o << setw(4) << count << ":  ";
                    count++;
                    if (i == 0)
                        o << code[i];
                }
                else
                {
                    o << code[i];
                }
            }

            o << endl << endl;
        }

        //
        //  Anonymous namespace with a bunch of parsing helper functions /
        //  datastructures.
        //

        namespace
        {

            struct ParsedParameter
            {
                ParsedParameter()
                    : q_in(false)
                    , q_out(false)
                    , q_const(false)
                    , special(false)
                    , requiresInline(false)
                {
                }

                bool q_in;
                bool q_out;
                bool q_const;
                bool special;
                bool usesST;
                bool usesSize;
                bool usesSampling;
                bool usesLookup;
                bool requiresInline;
                string name;
                string type;
            };

            typedef std::vector<ParsedParameter> ParsedParameterVector;

        } // anonymous namespace

        void Function::parseDeclaration(bool createSymbols)
        {
            ParsedParameterVector parsedParameters;
            m_glslParameters.clear();

            //
            //  Search for parameter list (e.g. "FUNCTION (...)").
            //  \\s == white space
            //

            regex re("\\b" + m_name + "\\b\\s*\\(([^)]*)\\)");
            smatch m;

            StringVector sourceParameters;

            bool usesSampling = false;

            if (regex_search(m_sourceCode, m, re))
            {
                string mstring = m[1];
                algorithm::split(sourceParameters, mstring,
                                 is_any_of(string(",")), token_compress_on);

                for (size_t i = 0; i < sourceParameters.size(); i++)
                {
                    trim(sourceParameters[i]);

                    //
                    //  Ignore it if its empty (this is probably a syntax error)
                    //

                    if (sourceParameters[i] == "")
                    {
                        sourceParameters.erase(sourceParameters.begin() + i);
                        i--;
                        continue;
                    }

                    //
                    //  Split by spaces
                    //

                    StringVector parts;
                    algorithm::split(parts, sourceParameters[i],
                                     algorithm::is_space(), token_compress_on);

                    //
                    //  Found qualifiers and any special cookies we need to
                    //  know about
                    //

                    ParsedParameter pp;

                    pp.q_in = true; // defaults to "in" parameter if no
                                    // qualifiers are present

                    for (int q = 0; q < parts.size(); q++)
                    {
                        if (parts[q] == "in")
                        {
                            pp.q_in = true;
                            pp.q_out = false;
                        }
                        else if (parts[q] == "out")
                        {
                            pp.q_out = true;
                            pp.q_in = false;
                        }
                        else if (parts[q] == "inout")
                        {
                            pp.q_in = true;
                            pp.q_out = true;
                        }
                        else if (parts[q] == "const")
                            pp.q_const = true;
                        else if (parts[q] == "")
                        {
                            parts.erase(parts.begin() + q);
                            q--;
                        }
                    }

                    if (parts.size() == 0)
                    {
                        TWK_THROW_EXC_STREAM(
                            "empty parameter declaration for parameter "
                            << (i + 1));
                    }

                    if (parts.size() == 1)
                    {
                        TWK_THROW_EXC_STREAM("invalid parameter declaration \'"
                                             << parts.front()
                                             << "\' for parameter " << (i + 1));
                    }

                    string name = parts.back();
                    string ptype = parts[parts.size() - 2];

                    //
                    //  These are currently hard-coded parameters that are
                    //  "special". The meaning of "special" is determined by
                    //  the shader generator.
                    //

                    pp.special = (name == "time" && ptype == "float")
                                 || (name == "_offset" && ptype == "vec2")
                                 || ptype == "outputImage";
                    pp.name = name;
                    pp.type = ptype;
                    pp.requiresInline = false;

                    if (ptype == "inputImage")
                    {
                        //
                        //  useSTRE is a bit funky -- its matching "NAME( _"
                        //  where _ is anything other than an end paren (or
                        //  whitespace).
                        //
                        //  Use of any of size, ST, or sampling offset from
                        //  the current pixel indicates that the shader will
                        //  need to be inlined whenever used since these
                        //  references will potentially be rewritten for each
                        //  instance of the function.
                        //
                        //  For example, a color shader that does nothing but
                        //  does a per-pixel operation like gamma will not
                        //  need to be inlined. However a color shader that
                        //  uses size and ST to do vingetting would need to be
                        //  inlined.
                        //
                        //  Most merge shaders like an over do not need to be
                        //  inlined. However, a merge shader that does a
                        //  transition like a wipe does need to be inlined
                        //  since it probably makes use of ST and the incoming
                        //  image sizes.
                        //
                        //  Filters (that sample the inputImage) always
                        //  need to be inlined since the "sampling" is
                        //  converted into a call to automatically generated
                        //  "samplerExpr" function.
                        //

                        smatch sm;
                        regex usesSTRE("\\b" + name
                                       + "\\s*\\.\\s*([stpq]{1,4}|[xyzw]{1,4}|["
                                         "rgba]{1,4})\\b");
                        regex usesSamplingRE("\\b" + name
                                             + "\\s*\\(\\s*[^\\)]");
                        regex usesLookupRE("\\b" + name + "\\s*\\(\\s*\\)");
                        regex usesSizeRE("\\b" + name
                                         + "\\s*\\.\\s*size\\s*\\(\\s*\\)");

                        pp.usesST = regex_search(m_sourceCode, sm, usesSTRE);
                        pp.usesSampling =
                            regex_search(m_sourceCode, sm, usesSamplingRE);
                        pp.usesLookup =
                            regex_search(m_sourceCode, sm, usesLookupRE);
                        pp.usesSize =
                            regex_search(m_sourceCode, sm, usesSizeRE);
                        pp.requiresInline =
                            pp.usesSampling || pp.usesST || pp.usesSize;

                        if (pp.usesSampling)
                            usesSampling = true;
                    }

                    if (pp.requiresInline)
                        m_inline = true;

                    parsedParameters.push_back(pp);
                }

                //
                //  Build the m_glslParameters vector from the parsed
                //  parameters. Not all parsed parameters become glsl
                //  parameters.
                //

                for (size_t i = 0; i < parsedParameters.size(); i++)
                {
                    ParsedParameter& p = parsedParameters[i];

                    //
                    //  Automatically determine the parameter symbol types if
                    //  needed
                    //

                    if (p.type == "inputImage")
                    {
                        if (p.q_in != true || p.q_const != true)
                        {
                            //
                            //  do not allow inputImage type to be an
                            //  out or non-constant. force it to be and then
                            //  notify user (shader writer)
                            //

                            TWK_THROW_EXC_STREAM(
                                "inputImage argument \'"
                                << p.name
                                << "\' must have qualifier \'const in\'");
                        }

                        if (!p.usesSampling)
                        {
                            //
                            //  Change the parameter type to vec4 and get rid of
                            //  the "()" used to call the inputImage (since
                            //  its just a color now).
                            //

                            p.type = "vec4";
                            ostringstream str;
                            str << (p.q_const ? "const" : "") << " "
                                << (p.q_in ? "in" : "")
                                << (p.q_out ? "out" : "") << " " << p.type
                                << " " << p.name;

                            m_glslParameters.push_back(str.str());

                            //
                            //  In the case of non-inlined, rewrite the glsl
                            //  code to remove () around inputImage
                            //  "calls" since these are now just a vanilla
                            //  vec4 parameter.
                            //
                            //  In the case of inlined, do the same thing but
                            //  prepend '_x_x' in front of the symbol so we
                            //  can differentiate between this and a texture
                            //  coordinate during rewriting.
                            //
                            //  NOTE: GL reserves symbols starting with '__'
                            //  so we need to make sure we don't use those.
                            //

                            string replacePat =
                                p.requiresInline ? "_x_x\\1" : "\\1";
                            regex re("\\b(" + p.name + ")\\s*\\(\\s*\\)");
                            m_sourceCode =
                                regex_replace(m_sourceCode, re, replacePat);
                        }

                        m_inImageParameters.push_back(
                            ImageParameterInfo(p.name, p.usesST, p.usesSize,
                                               p.usesSampling, p.usesLookup));
                    }
                    else if (p.type == "outputImage")
                    {
                        if (p.q_in != true || p.q_const != true)
                        {
                            //
                            //  do not allow outputImage type to be an out or
                            //  non-constant. force it to be and then notify
                            //  user (shader writer)
                            //
                            //  NOTE: its ok to throw here. Throwing here will
                            //  unwind past the Function constructor and back
                            //  to whoever tried to make then function. This
                            //  is expected behavior.
                            //

                            TWK_THROW_EXC_STREAM(
                                "outputImage argument \'"
                                << p.name
                                << "\' must have qualifier \'const in\'");
                        }

                        //
                        //  note "outputImage" is a special type that shader
                        //  writers need to be aware of remove "outputImage
                        //  foo" from parameter list cause we don't need it
                        //  replace foo.size with _windowSize, foo.AB (where
                        //  AB is any legal swizzle op for a vec2) with
                        //  _fragCoord
                        //

                        smatch sm;
                        regex regwh("\\b(" + p.name
                                    + ")\\s*\\.\\s*size\\s*\\(\\s*\\)");
                        regex regf(
                            "\\b(" + p.name
                            + ")\\s*\\.\\s*([st]{1,2}|[xy]{1,2}|[rg]{1,2})\\b");

                        //
                        //  look for occurence of foo.size or swizzle and
                        //  replace with corresponding symbols.
                        //

                        if (regex_search(m_sourceCode, sm, regwh))
                        {
                            m_usesOutputSize = true;
                            m_sourceCode = regex_replace(m_sourceCode, regwh,
                                                         "_windowSize");
                            m_inline = true;
                        }

                        if (regex_search(m_sourceCode, sm, regf))
                        {
                            m_usesOutputSize = true;
                            m_usesOutputST = true;

                            //
                            //  Swap in whatever swizzle the user might have
                            //  done on the outputImage coords when
                            //  substituting in fragCoord.
                            //

                            m_sourceCode = regex_replace(m_sourceCode, regf,
                                                         "_fragCoord.\\2");
                            m_inline = true;
                        }

                        //
                        //  remove this parameter
                        //

                        regex re;

                        if (i == 0)
                        {
                            re = regex("\\s*const\\s+in\\s+outputImage\\s+"
                                       + p.name + "\\s*,");
                        }
                        else
                        {
                            re = regex(",\\s*const\\s+in\\s+outputImage\\s+"
                                       + p.name + "\\b");
                        }

                        m_sourceCode = regex_replace(m_sourceCode, re, "");
                    }
                    else
                    {
                        ostringstream str;
                        str << (p.q_const ? "const" : "") << " "
                            << (p.q_in ? "in" : "") << (p.q_out ? "out" : "")
                            << " " << p.type << " " << p.name;

                        m_glslParameters.push_back(str.str());
                    }

                    //
                    //  If creating symbols, the symbolic (actual) function
                    //  parameters start out empty and this function will be
                    //  making them. The actual parameters are in m_parameters
                    //  whereas the GLSL parameters that are part of the final
                    //  shader text are in m_glslParameters.
                    //
                    //  For clarity: the m_parameters hold Shader::Symbol
                    //  objects which describe the interface on the CPU in the
                    //  Shader::Expression tree. When converted to
                    //  actual shaders on the GPU, the signature of a function
                    //  can change depending on how its used. The GPU function
                    //  signature is stored in m_glslParameters
                    //

                    if (createSymbols)
                    {
                        Symbol::Type stype = Symbol::VoidType;
                        unsigned int qualifier = Symbol::Uniform;

                        for (const SymbolTypeAssociation* a =
                                 symbolTypeAssociations;
                             a->glslName; a++)
                        {
                            if (p.type == a->glslName)
                            {
                                stype = a->type;

                                if (p.q_out && p.q_in)
                                    qualifier = Symbol::ParameterInOut;
                                else if (p.q_in && p.q_const)
                                    qualifier = Symbol::ParameterConstIn;
                                else if (p.q_in && !p.q_const)
                                    qualifier = Symbol::ParameterIn;
                                else if (p.q_out)
                                    qualifier = Symbol::ParameterOut;

                                if (p.special)
                                    qualifier |= Symbol::Special;

                                Symbol* s =
                                    new Symbol((Symbol::Qualifier)qualifier,
                                               p.name, stype);
                                m_parameters.push_back(s);
                                break; // in case of typo in
                                       // symbolTypeAssociations
                            }
                        }
                    }
                }

                if (m_inline && usesSampling)
                {
                    Symbol* s = new Symbol(Symbol::SpecialParameterConstIn,
                                           "_offset", Symbol::Vec2fType);
                    m_parameters.push_back(s);
                    m_glslParameters.push_back("const in vec2 _offset");
                }

                //
                //  NOTE: this is rewriting the *entire* source. Right now
                //  this code is assuming only a single function is there.
                //

                ostringstream str;
                str << m_callName << " ("
                    << algorithm::join(m_glslParameters, ", ") << ")";
                m_sourceCode = regex_replace(m_sourceCode, re, str.str());

                //
                //  Need some declarations if using outputSize or outputST
                //

#if 0
        if (m_usesOutputST || m_usesOutputSize)
        {
            ostringstream prelude;

            if (m_usesOutputSize) prelude << "varying vec2 _fragCoord;" << endl;
            if (m_usesOutputST) prelude << "uniform vec2 _windowSize;" << endl;

            m_sourceCode = prelude.str() + m_sourceCode;
        }
#endif
            }
            else
            {
                TWK_THROW_EXC_STREAM(
                    "expected to find function definition for \'"
                    << name() << "\' but did not");
            }
        }

        void Function::replaceTextureCalls()
        {
            //
            //  If the GLSL version is too low, we need to infer the texture*()
            //  functions from any sampler arguments. The reason its ok to
            //  look for patterns like this is that GLSL puts significant
            //  restrictions on how samplers can be used. The important ones
            //  for us are:
            //
            //      * samplers can be uniforms initialized by gl calls
            //      * samplers can be parameters to functions
            //      * samplers cannot be lvalues
            //
            //  That means a texture() call *must* have its first argument be
            //  one of the parameters to the function (or its an error). So we
            //  just look for those cases and using the information about the
            //  parameters we can figure out which of the deprecated texture
            //  calls needs to be used
            //
            //  Doing this avoids the situation where a compatability profile
            //  doesn't exist (apple) or the old calls cause an error for some
            //  reason (why?)
            //
            //  On the other hand if the user decided to "help" us by using the
            //  old style texture*() calls and the GLSL version is too *new* we
            //  swap in the new style for the old.
            //

            bool useDeprecated = glslMajor <= 1 && glslMinor <= 30;

            for (size_t i = 0; i < m_parameters.size(); i++)
            {
                const Symbol* sym = m_parameters[i];
                const char* func = 0;

                switch (sym->type())
                {
                case Symbol::Sampler1DType:
                    func = "texture1D";
                    break;
                case Symbol::Sampler2DType:
                    func = "texture2D";
                    break;
                case Symbol::Sampler2DRectType:
                    func = "texture2DRect";
                    break;
                case Symbol::Sampler3DType:
                    func = "texture3D";
                    break;
                default:
                    continue;
                }

                ostringstream reStr;
                ostringstream replaceStr;

                if (useDeprecated)
                {
                    reStr << "\\btexture\\s*\\(\\s*" << sym->name() << "\\b";
                    replaceStr << func << "(" << sym->name();
                }
                else
                {
                    //
                    //  Make sure they didn't help us by supporting pre-130 when
                    //  we're using a GLSL newer than that.
                    //

                    reStr << "\\b" << func << "\\s*\\(\\s*" << sym->name()
                          << "\\b";
                    replaceStr << "texture(" << sym->name();
                }

                m_sourceCode = regex_replace(m_sourceCode, regex(reStr.str()),
                                             replaceStr.str());
            }
        }

        namespace
        {
            // param: pass the GL_VERSION, not GLSL -- ex:
            // glGetString(GL_VERSION)
            const char* const get_glsl_header(const char* const glVersion)
            {
                if (nullptr == glVersion)
                {
                    std::cerr << "get_glsl_header error: glVersion is null."
                              << std::endl;
                    return global_glsl;
                }

                const char* glsl_header = global_glsl;
                if (Shader::Function::isGLSLVersionLessThan150())
                {
                    glsl_header = global_glsl_lt_150;
                }
                else
                {
                    glsl_header =
                        glVersion[0] <= '2' ? global_glsl_gl2 : global_glsl;
                }

                return glsl_header;
            }

            void compileGLSL(const string& source, GLuint& shaderID,
                             int& status, vector<char>& log)
            {
                shaderID = glCreateShader(GL_FRAGMENT_SHADER);

                const char* glVersion = (const char*)glGetString(GL_VERSION);
                int logsize;

                const char* src[2];
                src[0] = get_glsl_header(glVersion);
                src[1] = source.c_str();

                glShaderSource(shaderID, 2, src, 0);
                glCompileShader(shaderID);
                glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);
                glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logsize);

                if (logsize > 1)
                {
                    log.resize(logsize + 1);
                    GLsizei rlen;
                    glGetShaderInfoLog(shaderID, logsize, &rlen, &log.front());
                }
                else
                    log.resize(0);
            }

        } // namespace

        bool Function::compile() const
        {
            //
            //  Inline shaders are not compiled. The source code is used to
            //  generate a compiled function later.
            //

            if (isInline())
                return true;
            if (m_state)
                return true;

            m_state = new FunctionGLState();

            vector<char> buffer;
            int status;
            if (Shader::debuggingType() != Shader::NoDebugInfo)
            {
                cout << "INFO: compiling GLSL shader " << name() << " " << m_doc
                     << endl;
            }

            compileGLSL(m_sourceCode, m_state->shader, status, buffer);
            const char* glVersion = (const char*)glGetString(GL_VERSION);
            const std::string glsl_header(get_glsl_header(glVersion));

            if (status != GL_TRUE)
            {
                cout << "ERROR: compiling shader " << m_name << ": " << endl
                     << &buffer.front() << endl;

                cout << "ERROR: ----- source follows ----" << endl;
                outputAnnotatedCode(cout, glsl_header + m_sourceCode);
                releaseCompiledState();
            }
            else if (Shader::debuggingType() != Shader::NoDebugInfo)
            {
                cout << "INFO: ---- " << name() << " source follows ----"
                     << endl;
                outputAnnotatedCode(cout, glsl_header + m_sourceCode);
            }

            return status == GL_TRUE;
        }

        Function::ValidationResult Function::validate() const
        {
            GLuint shader;
            int status;
            vector<char> log;
            const char* glVersion = (const char*)glGetString(GL_VERSION);

            string testGLSL = generateTestGLSL(glVersion[0] <= '2');

            if (Shader::debuggingType() != Shader::NoDebugInfo)
            {
                cout << "INFO: validation shader" << endl;
                outputAnnotatedCode(cout, testGLSL);
            }

            compileGLSL(testGLSL, shader, status, log);
            if (log.empty())
                log.push_back(0);
            string logString = &log.front();
            glDeleteShader(shader);

            return ValidationResult(status == GL_TRUE, logString);
        }

        string Function::prototype(const string& suffix) const
        {
            ostringstream code;
            const SymbolVector& symbols = parameters();

            code << glslNameOfSymbolType(returnType()) << " " << callName()
                 << suffix << "(";

            bool first = true;

            for (size_t q = 0; q < symbols.size(); q++)
            {
                if (symbols[q]->type() == Symbol::InputImageType
                    || symbols[q]->type() == Symbol::OutputImageType)
                    continue;
                if (!first)
                    code << ", ";
                first = false;
                code << symbols[q]->glslQualifierName() << " "
                     << symbols[q]->glslTypeName();
            }

            code << ")";
            return code.str();
        }

        string Function::rewrite(const string& suffix,
                                 const StringVector& argNames,
                                 const StringVector& graphIDs) const
        {
            //
            //  This code is called in the case of Inline functions. Its called
            //  during the compilation of the final shader because the
            //  replacement expressions are not know until that time.
            //
            //  The result is inlined into the main program with a unique call
            //  for each instance.
            //
            //  The function's name has suffix appended to it to make it
            //  unique.
            //
            //  Function calls in the text which match the inputImage
            //  names declared in the function prototype are replaced.
            //

            assert(argNames.size() == m_inImageParameters.size());

            regex nameRE("\\b" + callName() + "\\b");
            string a = regex_replace(source(), nameRE, callName() + suffix);

            const char* glVersion = (const char*)glGetString(GL_VERSION);
            bool gl2 = glVersion[0] <= '2';

            //
            //  Replace: inName.st with COORDNAME.st
            //  Replace: inName.size() with SIZENAME
            //

            if (argNames.size() == graphIDs.size())
            {
                for (size_t i = 0; i < argNames.size(); i++)
                {
                    try
                    {
                        const ImageParameterInfo& pinfo =
                            m_inImageParameters[i];
                        const string& inName = pinfo.name;

                        //
                        //  The ST coordinates are always passed in
                        //  currently. In the future we could pass in a
                        //  uniform matrix and only a single set of STs for
                        //  all images.
                        //

                        if (pinfo.usesST)
                        {
                            ostringstream replaceExpr;
                            replaceExpr << "TexCoord" << graphIDs[i] << ".\\1";

                            regex re("\\b" + inName
                                     + "\\s*\\.\\s*([stpq]{1,4}|[xyzw]{1,4}|["
                                       "rgba]{1,4})\\b");
                            a = regex_replace(a, re, replaceExpr.str());
                        }

                        //
                        //  The size is passed in directly because there may
                        //  be no corresponding sampler. As a future
                        //  optimization we could find an existing associated
                        //  sampler and use that to determine the size, but
                        //  for the time being just use this one consistant
                        //  method. this is also compatible with GL2
                        //

                        if (pinfo.usesSize)
                        {
                            ostringstream replaceExpr;
                            replaceExpr << "Size" << graphIDs[i];

                            regex re2("\\b" + inName
                                      + "\\s*\\.\\s*size\\s*\\(\\s*\\)");
                            a = regex_replace(a, re2, replaceExpr.str());
                        }
                    }
                    catch (std::exception& exc)
                    {
                        cout << "CAUGHT: " << exc.what() << endl;
                    }
                }
            }

            //
            //  Replace: inName(...) with ARGNAME(...) or in the case of
            //  inlined color functions _x_xinName() with ARGNAME()
            //

            for (size_t i = 0; i < argNames.size(); i++)
            {
                const ImageParameterInfo& info = m_inImageParameters[i];
                const string& inName = info.name;

                if (info.usesSampling)
                {
                    //
                    //  find anything call inName() and call it _r_inName() so
                    //  they aren't found by the next regex
                    //

                    regex re0("\\b" + inName + "\\s*\\(\\s*\\)");
                    a = regex_replace(a, re0, "_r_" + inName + "()");

                    //
                    //  Find anything called inName(... and replace it with
                    //  inName(_offset + ...
                    //
                    //  Don't attempt to find the close paren. A GLSL
                    //  expression using a close paren might be inside the
                    //  "call" syntax. Since that expression must be of type
                    //  vec2 we can use '+' without worrying about operator
                    //  precedence. in GLSL '+' has precedence just below that
                    //  of the parens.
                    //

                    regex re("\\b" + inName + "\\s*\\(");
                    a = regex_replace(a, re, argNames[i] + "(_offset + ");

                    //
                    //  Finally replace the _r_InName() calls we protected
                    //  above with inName(offset)
                    //

                    regex re2("\\b_r_" + inName + "\\s*\\(\\s*\\)");
                    a = regex_replace(a, re2, argNames[i] + "(_offset)");
                }
                else
                {
                    //
                    //  Anything which was marked as a vec4 color param was
                    //  prefixed by _x_x. Replace those too
                    //

                    regex re3("\\b_x_x" + inName + "\\b");
                    a = regex_replace(a, re3, inName);
                }
            }

            return a;
        }

        bool Function::isColor() const
        {
            return type() == Color || type() == LinearColor;
        }

        bool Function::isFilter() const { return type() >= Filter; }

        bool Function::isSource() const { return type() == Source; }

        string Function::removeComments(const string& source)
        {
            //
            //  The comments are removed at an early stage in shader
            //  parsing/editing in order to prevent any unexpected
            //  behavior. This class can currently only "parse" the shader
            //  using a bunch of cobbled together regexps which is not
            //  ideal. removing the comments is just insurance against
            //  spurious regexp matching and/or text rewriting.
            //
            //  Comment removal keeps line and char column numbering in
            //  tact. Currently that means that C++ comments retain the
            //  newline (since all the rest is comment) and C comments are
            //  overwritten by spaces (in case the comment is in the middle of
            //  a line of code).
            //
            //  This is done in order to keep error messages with line/column
            //  numbers generated on the comment-free code corresponding to
            //  line/column numbers in the original code.
            //
            //  Improvements:
            //
            //    * Only doing the C comment space overwriting if the comment is
            //      really in the middle of a line of code. If the comment has
            //      no actual code after it there's no reason to keep it.
            //
            //    * Use the #line directive where possible. This might reduce
            //    the
            //      total output from this function.
            //
            //  NOTE: here's the portion of the spec regarding #line. In not
            //  real clear on what source-string-number is refering to:
            //
            //      line must have, after macro substitution, one of the
            //      following two forms:
            //
            //          #line line
            //          #line line source-string-number
            //
            //      where line and source-string-number are constant integer
            //      expressions. After processing this directive (including its
            //      new-line), the implementation will behave as if it is
            //      compiling at line number line+1 and source string number
            //      source-string-number. Subsequent source strings will be
            //      numbered sequentially, until another #line directive
            //      overrides that numbering.
            //
            //  I assume that source-string-number means column number or
            //  "character since newline or beginning of file" and not
            //  "character since beginning of file"
            //

            bool c_comment = false;
            bool cpp_comment = false;
            size_t outindex = 0;

            vector<char> outbuffer(source.size() + 1); // max size needed

            for (size_t i = 0, s = source.size(); i < s; i++)
            {
                const char c = source[i];
                const char p = i ? source[i - 1] : 0;

                if (c_comment)
                //
                //  Inside C comment.
                //
                {
                    if (p && p == '*' && c == '/')
                    {
                        c_comment = false;
                    }
                    outbuffer[outindex++] = (c == '\n') ? '\n' : ' ';
                }
                else if (cpp_comment)
                //
                //  Inside C++ comment.
                //
                {
                    if (c == '\n')
                    {
                        cpp_comment = false;
                    }
                    outbuffer[outindex++] = (c == '\n') ? '\n' : ' ';
                }
                else if (p && p == '/' && c == '*')
                //
                //  Beginning of C comment.
                //
                {
                    c_comment = true;
                    outindex--;
                    outbuffer[outindex++] = ' ';
                    outbuffer[outindex++] = ' ';
                }
                else if (p && p == '/' && c == '/')
                //
                //  Begining of C++ comment.
                //
                {
                    cpp_comment = true;
                    outindex--;
                    outbuffer[outindex++] = ' ';
                    outbuffer[outindex++] = ' ';
                }
                else
                //
                //  Add to output buffer.
                {
                    outbuffer[outindex++] = c;
                }
            }

            outbuffer[outindex++] = 0; // terminate c-string

            return string(&outbuffer.front());
        }

        void Function::hashAuxNames()
        {
            //
            //  Make a smaller string without anything inside { and } and runs
            //  of whitespace reduced to a single ' '. Presumably the string
            //  has already had comments stripped.
            //
            //  GLSL is fairly simple so this should result in a bunch of line
            //  line strings which are either declarations (of const vars,
            //  layouts, or external functions) or a function definition. The
            //  function definition and any struct definitions are relaced by
            //  "{}" making them simpler to deal with.
            //

            vector<char> outbuffer;
            StringVector lines;
            size_t count = 0;
            size_t whitespace = 0;

            for (size_t i = 0, s = m_sourceCode.size(); i < s; i++)
            {
                const char c = m_sourceCode[i];

                if (c == '{')
                {
                    count++;
                    whitespace = 0;
                }
                else if (c == '}')
                {
                    count--;
                    whitespace = 0;

                    if (count == 0)
                    {
                        outbuffer.push_back('{');
                        outbuffer.push_back('}');
                        outbuffer.push_back(0);
                        lines.push_back(&outbuffer.front());
                        outbuffer.clear();
                    }
                }
                else if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
                {
                    if (!whitespace && outbuffer.size())
                        outbuffer.push_back(' ');
                    whitespace++;
                }
                else if (!count)
                {
                    whitespace = 0;
                    outbuffer.push_back(c);

                    if (c == ';')
                    {
                        outbuffer.push_back(0);
                        lines.push_back(&outbuffer.front());
                        outbuffer.clear();
                    }
                }
            }

            if (outbuffer.size())
            {
                outbuffer.push_back(0);
                lines.push_back(&outbuffer.front());
            }

            //
            //  Now we have a bunch of lines each of which is either a
            //  declaration or a function definition.
            //

            static regex funcDefRE(" ?\\w+ (\\w+) ?\\([^\\{]+\\{\\}$");
            static regex varRE("const .*?(\\w+) ?=");
            static regex varArrayRE(
                "const .*?(\\w+)\\[\\w+] ?= ?\\w+\\[\\w+].*$");
            static regex uniformSamplerRE("uniform sampler\\wD (\\w+).*$");

            for (size_t i = 0; i < lines.size(); i++)
            {
                const string& line = lines[i];
                smatch sm;

                if (regex_search(line, sm, funcDefRE)
                    || regex_search(line, sm, varRE)
                    || regex_search(line, sm, varArrayRE)
                    || regex_search(line, sm, uniformSamplerRE))
                {
                    if (sm[1] != m_name)
                    {
                        const string& s = sm[1];
                        m_auxNames.push_back(s);

                        regex re("\\b" + s + "\\b");
                        m_sourceCode =
                            regex_replace(m_sourceCode, re, s + m_hashString);
                    }
                }
            }
        }

        const Function::ImageParameterInfo*
        Function::imageParameterInfo(size_t index) const
        {
            const string& name = m_parameters[index]->name();

            for (size_t i = 0; i < m_inImageParameters.size(); i++)
            {
                if (m_inImageParameters[i].name == name)
                {
                    return &m_inImageParameters[i];
                }
            }

            return 0;
        }

        bool Function::isInputImageParameter(const std::string& name) const
        {
            for (size_t i = 0; i < m_inImageParameters.size(); i++)
            {
                if (m_inImageParameters[i].name == name)
                    return true;
            }

            return false;
        }

        bool Function::equivalentInterface(const Function* other)
        {
            if (m_type != other->m_type
                || m_parameters.size() != other->m_parameters.size())
            {
                return false;
            }

            for (int i = 0; i < m_parameters.size(); ++i)
            {
                const Symbol* a = m_parameters[i];
                const Symbol* b = other->m_parameters[i];
                if (!(*a == *b))
                    return false;
            }

            return true;
        }

        string Function::generateTestGLSL(bool gl2) const
        {
            //
            //  Create a semantically equivalent GLSL shader from the original
            //  GLSL shader by swapping in stand-in function prototypes and/or
            //  unform and varying variables. This shader can then be compiled
            //  by the GLSL compiler to validate the original GLSL code.  This
            //  is necessary because shaders will be rewritten and/or inlined
            //  when actually used. Error reporting becomes difficult when the
            //  shader is no longer in context.
            //

            string glsl = m_originalGLSL;
            size_t lineCount = 0;
            ostringstream str;

            string inKeyword = gl2 ? "varying" : "in";

            //
            //  Swap in the call name (hashed name) in case its called "main"
            //  which in GLSL is reserved for the program entry point
            //

            regex mainRE("\\b" + name() + "(\\s*\\([^)]*)\\)");
            glsl = regex_replace(glsl, mainRE, callName() + "\\1)");

            //
            //  Rename various inputImage parameters so they can be
            //  individually referenced later.
            //
            //  NAME()       _color_NAME
            //  NAME(...)    _sampler_NAME(...)
            //  NAME.st      _st_NAME
            //  NAME.size()  _size_NAME
            //
            //  Later we'll convert those into either funciton prototypes
            //  in the case of NAME(...) or varying/uniform prototypes for
            //  the rest.
            //

            for (size_t i = 0; i < m_inImageParameters.size(); i++)
            {
                const ImageParameterInfo& info = m_inImageParameters[i];

                if (info.usesLookup)
                {
                    //
                    //  NAME() -> _color_NAME
                    //

                    regex colorRE("\\b(" + info.name + ")\\s*\\(\\s*\\)");
                    string replacePat = "_color_\\1";
                    glsl = regex_replace(glsl, colorRE, replacePat);

                    str << inKeyword << " vec4 " << "_color_" << info.name
                        << ";" << endl;
                    lineCount++;
                }

                if (info.usesST)
                {
                    //
                    //  NAME.st -> _st_NAME
                    //

                    regex stRE("\\b(" + info.name
                               + ")\\s*\\.\\s*([stpq]{1,4}|[xyzw]{1,4}|[rgba]{"
                                 "1,4})\\b");
                    string replacePat = "_st_\\1.\\2";
                    glsl = regex_replace(glsl, stRE, replacePat);

                    str << inKeyword << " vec4 " << "_st_" << info.name << ";"
                        << endl;
                    lineCount++;
                }

                if (info.usesSize)
                {
                    //
                    //  NAME.size() -> _size_NAME
                    //

                    regex sizeRE("\\b(" + info.name
                                 + ")\\s*\\.\\s*size\\s*\\(\\s*\\)");
                    string replacePat = "_size_\\1";
                    glsl = regex_replace(glsl, sizeRE, replacePat);

                    str << "uniform vec2 " << "_size_" << info.name << ";"
                        << endl;
                    lineCount++;
                }

                if (info.usesSampling)
                {
                    //
                    //  NAME(...) -> _sampler_NAME(...)
                    //

                    regex samplingRE("\\b(" + info.name + "\\s*\\(\\s*[^\\)])");
                    string replacePat = "_sampler_\\1";
                    glsl = regex_replace(glsl, samplingRE, replacePat);

                    str << "vec4 " << "_sampler_" << info.name
                        << "(const in vec2);" << endl;
                    lineCount++;
                }
            }

            const SymbolVector& params = parameters();

            for (size_t i = 0; i < params.size(); i++)
            {
                if (params[i]->type() == Symbol::OutputImageType)
                {
                    string name = params[i]->name();

                    //
                    //  Change any outputImage parameters to blanks. Just in
                    //  case there's a newline in there someplace, put all the
                    //  whitespace back.
                    //

                    if (i > 0)
                    {
                        regex outRE =
                            regex(",(\\s*)const(\\s+)in(\\s+)outputImage(\\s+)"
                                  + name + "\\b");
                        glsl = regex_replace(glsl, outRE, "\\1\\2\\3\\4");
                    }
                    else
                    {
                        regex outRE =
                            regex("(\\s*)const(\\s+)in(\\s+)outputImage(\\s+)"
                                  + name + "(\\s*),");
                        glsl = regex_replace(glsl, outRE, "\\1\\2\\3\\4\\5");
                    }

                    if (m_usesOutputST)
                    {
                        //
                        //  OUT.st -> _outST.st
                        //

                        regex outSTRE(
                            "\\b" + name
                            + "\\s*\\.\\s*([st]{1,2}|[xy]{1,2}|[rg]{1,2})\\b");
                        glsl = regex_replace(glsl, outSTRE, "_outST.\\1");

                        str << "uniform vec2 _outST;" << endl;
                        lineCount++;
                    }

                    if (m_usesOutputSize)
                    {
                        //
                        //  OUT.size() -> _outSize
                        //

                        regex outSizeRE("\\b" + name
                                        + "\\s*\\.\\s*size\\s*\\(\\s*\\)");
                        glsl = regex_replace(glsl, outSizeRE, "_outSize");

                        str << "uniform vec2 _outSize;" << endl;
                        lineCount++;
                    }
                }
            }

            str << "#line 0" << endl;

            //
            //  replace inputImage token with vec4. This will result in an
            //  unused parameter which should parse sucessfully, but may
            //  result in a "bad" shader on OS X. Since we're never actually
            //  going to use this shader that should not be a problem
            //

            regex inputImageRE("\\binputImage\\b");
            string replacePat = "vec4      ";
            glsl = regex_replace(glsl, inputImageRE, replacePat);

            str << glsl;

            return str.str();
        }

        void Function::deleteRetired()
        {
            auto last = std::unique(
                m_retiredFunctions.begin(),
                m_retiredFunctions
                    .end()); // assumption an object cannot be added more than
                             // once non-consecutively (lifetime of objects)
            for (FunctionVector::iterator i = m_retiredFunctions.begin();
                 i != last; ++i)
            {
                delete *i;
            }
            m_retiredFunctions.resize(0);
        }

    } // namespace Shader
} // namespace IPCore
