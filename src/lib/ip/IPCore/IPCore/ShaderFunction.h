//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ShaderFunction__h__
#define __IPCore__ShaderFunction__h__
#include <IPCore/ShaderSymbol.h>

namespace TwkFB
{
    class FrameBuffer;
}

namespace IPCore
{
    namespace Shader
    {

        struct FunctionGLState;
        struct ProgramGLState;

        //
        //  class Shader::Function
        //
        //  An abstract function with parameters. There could be multiple
        //  implementations of the function (e.g. GLSL, OpenCL, CUDA, native)
        //  but they are all expected to take some variant of the same set of
        //  parameters and to compute the same result.
        //
        //  The Shader's signature should be a result of its type. So
        //  e.g. for a Filter the Shader takes a color and returns a color.
        //
        //  Shader is static and created once to represent a predefined
        //  shader function. These are not constructed on the fly during
        //  evaluation.
        //
        //  The constructor expects an approximation of the number of "fetches"
        //  (as in texture fetches) that the function is expected to produce. If
        //  this varies wildly supply a max expected number. For example, a
        //  source RGBA shader which looks up a single RGBA color value from a
        //  texture does 1 fetch. A 3x3 blur filter does 9 fetches. A function
        //  which converts a color from RGB to HSV does 0 fetches.
        //

        class Function
        {
        public:
            typedef std::vector<std::string> StringVector;
            typedef std::vector<Function*> FunctionVector;
            typedef std::pair<bool, std::string> ValidationResult;

            struct ResourceUsage
            {
                ResourceUsage()
                    : fetches(0)
                    , buffers(0)
                    , coords(0)
                {
                }

                void accumulate(const ResourceUsage& u)
                {
                    fetches += u.fetches;
                    buffers += u.buffers;
                    coords += u.coords;
                }

                void filterAccumulate(const ResourceUsage& u)
                {
                    fetches *= u.fetches;
                    buffers += u.buffers;
                    coords += u.coords;
                }

                void clear()
                {
                    fetches = 0;
                    buffers = 0;
                    coords = 0;
                }

                void set(size_t f, size_t b, size_t c)
                {
                    fetches = f;
                    buffers = b;
                    coords = c;
                }

                size_t fetches;
                size_t buffers;
                size_t coords;
            };

            struct ImageParameterInfo
            {
                ImageParameterInfo(const std::string& s, bool a, bool b,
                                   bool samp, bool lookup)
                    : name(s)
                    , usesST(a)
                    , usesSize(b)
                    , usesSampling(samp)
                    , usesLookup(lookup)
                {
                }

                std::string name;
                bool usesST;
                bool usesSize;
                bool usesSampling;
                bool usesLookup;
            };

            typedef std::vector<ImageParameterInfo> ImageParameterVector;

            //
            //  The Type determines how the function is instantiated (inlined,
            //  rewritten or not) and how passes should be constructed.
            //
            //  UndecidedType can be used to have the Function infer its type
            //  from source code. The Function class infer Source, Color,
            //  Merge, and Filter types. If the real type would be
            //  MorphologicalFilter than it will be labeled as Filter. If the
            //  real type would be LinearColor than it will be labeled Color.
            //
            //  The Main type is reserved for use by Shader::Program.
            //
            //  LinearColor indicates that the color transform maps one linear
            //  space to another (and therefore could have been done as a
            //  built-in matrix).
            //
            //  MorphologicalFilter means the function will sample based on
            //  the derivative of ST. This is a much wider meaning than
            //  "morphological operation" in computer vision. This distinction
            //  is necessary to prevent poor performance of computations on
            //  the GPU. An example of a MorphologicalFilter would be a sinc
            //  filter which changes its sampling pattern and number of
            //  fetches depending on how ST changes from input to output
            //  coordinates. If a MorphologicalFilter is mis-typed as a Filter
            //  it will still work, but may result in poor performance due to
            //  underestimation of number of fetches.
            //

            enum Type //             FUNCTION TYPE
            {         //             -------------
                UndecidedType =
                    0, //                            (inferred to be one of the
                       //                            below)
                Main = 1 << 0,   //               () -> void
                Source = 1 << 1, // (sampler, coord) -> color4
                Color =
                    1 << 2, //           color4 -> color4 (assumed non-linear)
                LinearColor =
                    1 << 3,      //           color4 -> color4 (linear ops only)
                Merge = 1 << 4,  //    (image,image) -> color4
                Filter = 1 << 5, //          (image) -> color4
                MorphologicalFilter = 1 << 6 //          (image) -> color4
            };

            Function(const std::string& name, const std::string& glsl,
                     Type type, const SymbolVector& params,
                     const SymbolVector& globals, size_t numFetchesApprox = 0,
                     const std::string& doc = "");

            Function(const std::string& name, const std::string& glsl,
                     Type type, size_t numFetchesApprox = 0,
                     const std::string& doc = "");

            static void useShadingLanguageVersion(const char*);
            static bool isGLSLVersionLessThan150();

            const std::string& name() const { return m_name; }

            const std::string& callName() const { return m_callName; }

            size_t hash() const { return m_hash; }

            const std::string& hashString() const { return m_hashString; }

            Type type() const { return m_type; }

            const std::string& source() const { return m_sourceCode; }

            const std::string& originalSource() const { return m_originalGLSL; }

            const SymbolVector& parameters() const { return m_parameters; }

            const SymbolVector& globals() const { return m_globals; }

            const ImageParameterVector& imageParameters() const
            {
                return m_inImageParameters;
            }

            const FunctionGLState* state() const { return m_state; }

            bool isCompiled() const;

            bool isInline() const { return m_inline; }

            bool usesSampling() const { return m_usesSampling; }

            bool compile() const;
            bool isInputImageParameter(const std::string&) const;
            const ImageParameterInfo* imageParameterInfo(size_t) const;

            bool usesOutputSize() const { return m_usesOutputSize; }

            bool usesOutputST() const { return m_usesOutputST; }

            void releaseCompiledState() const;
            const Symbol::Type returnType() const;

            bool isFilter() const;
            bool isColor() const;
            bool isSource() const;

            std::string prototype(const std::string& suffix = "") const;
            std::string rewrite(const std::string& suffix,
                                const StringVector& argNames,
                                const StringVector& graphIDs) const;

            void parseDeclaration(bool);

            const StringVector& parsedParameterDeclarations() const
            {
                return m_glslParameters;
            }

            std::string generateTestGLSL(bool gl2) const;
            ValidationResult validate() const;

            const ResourceUsage& resourceUsage() const
            {
                return m_resourceUsage;
            }

            bool equivalentInterface(const Function*);

            void retire() { m_retiredFunctions.push_back(this); }

        protected:
            static void deleteRetired();

        private:
            //
            //  Function objects should not be directly deleted because pointers
            //  to them are used in sorting shader cache map (see
            //  ShaderProgram).
            //
            ~Function();

            void initHash();
            void inferType();

            static std::string removeComments(const std::string&);
            void hashAuxNames();
            void initResourceUsage();
            void replaceTextureCalls();

        private:
            static FunctionVector m_retiredFunctions;
            mutable FunctionGLState* m_state;
            Type m_type;
            std::string m_name;
            std::string m_callName;
            std::string m_sourceCode;
            std::string m_testGLSL;
            std::string m_originalGLSL;
            std::string m_doc;
            StringVector m_functions;
            StringVector m_glslParameters;
            ImageParameterVector m_inImageParameters;
            StringVector m_auxNames;
            SymbolVector m_parameters;
            SymbolVector m_globals;
            size_t m_hash;
            std::string m_hashString;
            ResourceUsage m_resourceUsage;
            bool m_inline;
            bool m_usesOutputSize;
            bool m_usesOutputST;
            bool m_usesSampling;

            friend class Program;
            friend class ProgramCache;
        };

        typedef std::vector<const Function*> FunctionVector;

        void outputAnnotatedCode(std::ostream&, const std::string&);

    } // namespace Shader
} // namespace IPCore

#endif // __IPCore__ShaderFunction__h__
