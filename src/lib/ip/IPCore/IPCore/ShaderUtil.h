//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ShaderUtil__h__
#define __IPCore__ShaderUtil__h__
#include <iostream>
#include <IPCore/ShaderExpression.h>

namespace IPCore
{
    class IPImage;

    namespace Shader
    {

        //
        //  Base class for visiting nodes in shader expressions. Doesn't do
        //  anything by itself.
        //

        class ExpressionVisitor
        {
        public:
            ExpressionVisitor(Expression* root, bool onEnter, bool onLeave,
                              bool eachSymbol);
            ~ExpressionVisitor();

            virtual void visit(Expression*, bool enter) {}

            virtual void child(Expression* parent, BoundSymbol* child,
                               size_t childNum)
            {
            }

            void cancel() { m_stop = true; }

            Expression* root() const { return m_root; }

        protected:
            void visitRecursive(Expression*);

        private:
            bool m_stop;
            Expression* m_root;
            bool m_enter;
            bool m_leave;
            bool m_symbol;
        };

        //
        //  ExpressionQuery
        //
        //  Finds any of the passed in Function::Type expressions.
        //

        class ExpressionQuery : public ExpressionVisitor
        {
        public:
            ExpressionQuery(Expression* fexpr, unsigned int typeFlags,
                            size_t maxCount = 1000000)
                : ExpressionVisitor(fexpr, true, false, false)
                , m_max(maxCount)
                , m_type(typeFlags)
            {
                visitRecursive(root());
            }

            virtual void visit(Expression*, bool);

            bool count() const { return m_fexprs.size(); }

        private:
            std::vector<Expression*> m_fexprs;
            size_t m_max;
            unsigned int m_type;
        };

        //
        //  Install adaptive resize if needed. This is used by "interactive"
        //  views like stack, layout, and sequence.
        //

        void installAdaptiveBoxResizeRecursive(IPImage*);

        //
        //  Count number of Function::Source or Function::(Morphological)Filter
        //  expressions in an expression tree
        //

        size_t sourceFunctionCount(Expression*, size_t limit = 1000000);
        size_t filterFunctionCount(Expression*, size_t limit = 1000000);

    } // namespace Shader
} // namespace IPCore

#endif // __IPCore__ShaderUtil__h__
