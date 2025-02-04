//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ShaderProgram__h__
#define __IPCore__ShaderProgram__h__
#include <IPCore/ShaderExpression.h>
#include <TwkGLF/GLProgram.h>

namespace IPCore
{
    namespace Shader
    {

        struct FunctionGLState;
        struct ProgramGLState;

        struct CompareExpr
        {
            bool operator()(const Expression* A, const Expression* B) const
            {
                return A->lessThan(B);
            }
        };

        //
        //  class Program
        //
        //  A final program object which incorporates a bunch of Functions and
        //  generates the final shader main.
        //
        //  Programs are cached according the to the expression passed
        //  in. Equivalent expressions (excluding bound values) can be used as
        //  a key to the select() function to find an already compiled
        //  identical program (if it exists).
        //
        //  So there are two types of expression trees: unbound (no values)
        //  and bound (values). The Program stores an unbound tree which no
        //  longer has evaluation dependencies (IPImages, fbs, time, etc). The
        //  unbound tree must contain enough information for the Program to
        //  generate a working shader.
        //
        //  The program can be bound using bind() to a bound expression
        //  tree. Texture unit assignments must be passed into bind() by the
        //  renderer.  After calling bind() all glUniform values are bound and
        //  the program is ready for a call to use().
        //

        class Program : public TwkGLF::GLProgram
        {
        public:
            //
            //  Types
            //
            struct ImageAndCoordinateUnit
            {
                ImageAndCoordinateUnit()
                    : textureUnit(-1)
                    , coordinateSet(-1)
                    , textureTarget(-1)
                    , plane(-1)
                    , hasST(false)
                    , width(0)
                    , height(0)
                    , uncropX(0)
                    , uncropY(0)
                    , uncropWidth(0)
                    , uncropHeight(0)
                    , outputSize(false)
                {
                }

                std::string idhash;
                std::string graphID;
                int textureUnit;
                int coordinateSet;
                int textureTarget;
                int plane;
                bool hasST;
                int height;
                int width;
                int uncropX;
                int uncropY;
                int uncropWidth;
                int uncropHeight;
                bool outputSize;
            };

            struct NameBinding
            {
                NameBinding()
                    : bsymbol(0)
                    , name("UNDEFINED")
                    , uniform(false)
                {
                }

                NameBinding(const BoundSymbol* s, const std::string& n, bool u)
                    : bsymbol(s)
                    , name(n)
                    , uniform(u)
                {
                }

                const BoundSymbol* bsymbol;
                std::string name;
                bool uniform;
            };

            typedef std::vector<const BoundSymbol*> BoundSymbolVector;

            struct LocalFunction
            {
                LocalFunction(const std::string& n, const std::string& gid,
                              const Expression* r, bool e = true)
                    : name(n)
                    , graphID(gid)
                    , root(r)
                    , shouldEmit(e)
                {
                }

                std::string name;
                std::string graphID;
                const Expression* root;
                BoundSymbolVector bsymbols;
                bool shouldEmit;
            };

            typedef std::vector<LocalFunction> LocalFunctionVector;
            typedef std::map<const Expression*, size_t> ReverseLocalFunctionMap;
            typedef std::vector<ImageAndCoordinateUnit> TextureUnitAssignments;
            typedef std::vector<NameBinding> NameBindingVector;
            typedef std::map<std::string, size_t> NameCountMap;
            typedef std::set<const Function*> FunctionSet;
            typedef std::map<const Expression*, std::string> ExprSuffixMap;
            typedef std::map<const BoundSymbol*, NameBinding> BindingMap;
            typedef std::map<const BoundSymbol*, int> CoordBindingMap;
            typedef std::map<int, const Expression*> InvSourceBindingMap;
            typedef std::set<std::string> GraphIDSet;

            //
            //  Constructors
            //

            Program(Expression*);
            ~Program();

            //
            //  Internal State
            //

            const Expression* expression() const { return m_expr; }

            //
            //  GLProgram API
            //

            virtual bool compile();

            //
            //  ShaderProgram API
            //

            void releaseCompiledState();
            bool validate() const;

            void bind(const Expression*, const TextureUnitAssignments&,
                      const TwkMath::Vec2f&) const;

            const GraphIDSet& outputSTSet() const { return m_outputSTSet; }

            const GraphIDSet& outputSizeSet() const { return m_outputSizeSet; }

        private:
            void collectSymbolNames(const Expression*, size_t);
            void bind2(size_t&, const Expression*, const Expression*,
                       const TextureUnitAssignments&,
                       const TwkMath::Vec2f&) const;

            std::string recursiveOutputExpr(const Expression*, bool);

            void outputLocalFunction(std::ostream&, const LocalFunction&);

            // output uniform variables needed in case of uncropped images
            void outputUncropUniforms(std::ostream&, const Expression*);

        private:
            Expression* m_expr;
            GraphIDSet m_outputSTSet;
            GraphIDSet m_outputSizeSet;
            FunctionSet m_functions;
            NameCountMap m_nameCountMap;
            CoordBindingMap m_coordBindingMap;
            InvSourceBindingMap m_invSourceBindingMap;
            BindingMap m_bindingMap;
            ExprSuffixMap m_inlineMap;
            LocalFunctionVector m_localFunctions;
            ReverseLocalFunctionMap m_localIndexMap;
            size_t m_totalST;
            Function* m_main;
            std::string m_vertexCode;
            bool m_needOutputSize;
            bool m_needOutputST;
        };

        class ProgramCache
        {
        public:
            ProgramCache();
            ~ProgramCache();

            typedef std::map<const Expression*, Program*, CompareExpr>
                ProgramCacheMap;
            typedef std::vector<Function*> FunctionVector;

            //
            //  select() will compile a new Program object and cache it if one
            //  does not yet exist.
            //

            const Program* select(const Expression*);
            void flush();

        private:
            ProgramCacheMap m_programCache;
        };

    } // namespace Shader
} // namespace IPCore

#endif // __IPCore__ShaderProgram__h__
