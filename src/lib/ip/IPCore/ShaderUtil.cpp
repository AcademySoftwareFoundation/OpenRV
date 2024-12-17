//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/ShaderUtil.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/IPImage.h>
#include <TwkMath/Vec2.h>

namespace IPCore
{
    namespace Shader
    {
        using namespace std;
        using namespace TwkMath;

        ExpressionVisitor::ExpressionVisitor(Expression* Fexpr, bool onEnter,
                                             bool onLeave, bool eachSymbol)
            : m_root(Fexpr)
            , m_stop(false)
            , m_enter(onEnter)
            , m_leave(onLeave)
            , m_symbol(eachSymbol)
        {
        }

        ExpressionVisitor::~ExpressionVisitor() {}

        void ExpressionVisitor::visitRecursive(Expression* Fexpr)
        {
            if (m_enter)
            {
                visit(Fexpr, true);
                if (m_stop)
                    return;
            }

            const ArgumentVector& args = Fexpr->arguments();

            for (size_t i = 0; i < args.size() && !m_stop; i++)
            {
                if (BoundExpression* be =
                        dynamic_cast<BoundExpression*>(args[i]))
                {
                    visitRecursive(be->value());
                }
            }

            if (m_symbol)
            {
                for (size_t i = 0; i < args.size() && !m_stop; i++)
                {
                    child(Fexpr, args[i], i);
                }
            }

            if (m_leave && !m_stop)
            {
                visit(Fexpr, false);
            }
        }

        void ExpressionQuery::visit(Expression* Fexpr, bool)
        {
            if (Fexpr->function()->type() & m_type)
            {
                m_fexprs.push_back(Fexpr);
                if (m_fexprs.size() >= m_max)
                    cancel();
            }
        }

        void installAdaptiveBoxResizeRecursive(IPImage* image)
        {
            if (!image->shaderExpr)
            {
                for (IPImage* i = image->children; i; i = i->next)
                {
                    installAdaptiveBoxResizeRecursive(i);
                }
            }
            else
            {
                ExpressionQuery findResampler(image->shaderExpr,
                                              Function::MorphologicalFilter, 1);

                if (findResampler.count() == 0)
                {
                    image->shaderExpr = newInlineBoxResize(
                        true, image, image->shaderExpr, Vec2f(0.0f));
                }
            }
        }

        size_t sourceFunctionCount(Expression* expr, size_t limit)
        {
            ExpressionQuery Q(expr, Function::Source, limit);
            return Q.count();
        }

        size_t filterFunctionCount(Expression* expr, size_t limit)
        {
            ExpressionQuery Q(
                expr, Function::Filter | Function::MorphologicalFilter, limit);
            return Q.count();
        }

    } // namespace Shader
} // namespace IPCore
