//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ShaderExpression__h__
#define __IPCore__ShaderExpression__h__
#include <IPCore/ShaderFunction.h>

namespace TwkFB
{
    class FrameBuffer;
}

namespace IPCore
{
    namespace Shader
    {

        //
        //  class Shader::Expression
        //
        //  This object is built at evaluation time and sent to the
        //  renderer. The argument vector contents are owned by the
        //  Expression once passed in. The arguments should be
        //  BoundSymbol objects.
        //

        typedef std::vector<BoundSymbol*,
                            stl_ext::replacement_allocator<BoundSymbol*>>
            ArgumentVector;

        class Expression
        {
        public:
            static void* operator new(size_t s);
            static void operator delete(void* p, size_t s);
            Expression(const Function* F, const ArgumentVector& args,
                       const IPImage* image = 0);
            Expression(const Function* F, const ArgumentVector& args,
                       const std::string& graphID);
            ~Expression();

            Expression* copy() const;
            Expression* copyUnbound() const;

            const Function* function() const { return m_function; }

            const ArgumentVector& arguments() const { return m_arguments; }

            const IPImage* image() const { return m_image; }

            const char* graphID() const { return m_graphID; }

            bool lessThan(const Expression*) const;
            void output(std::ostream&) const;
            void outputHash(std::ostream&) const;

            Function::ResourceUsage computeResourceUsageRecursive() const;

        private:
            const Function* m_function;
            ArgumentVector m_arguments;
            const IPImage* m_image;
            char* m_graphID;
        };

        typedef std::vector<Expression*> ExpressionVector;

        typedef stl_ext::replacement_allocator<const TwkFB::FrameBuffer*>
            FBAlloc;
        typedef std::vector<const TwkFB::FrameBuffer*, FBAlloc> FBVector;

    } // namespace Shader
} // namespace IPCore

#endif // __IPCore__ShaderExpression__h__
