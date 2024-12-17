//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/ShaderExpression.h>
#include <IPCore/ShaderState.h>
#include <IPCore/IPImage.h>
#include <TwkGLF/GL.h>
#include <TwkUtil/Timer.h>
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

        // new and delete implementation
        TWK_CLASS_NEW_DELETE(Expression)

        Expression::Expression(const Function* F, const ArgumentVector& args,
                               const IPImage* image)
            : m_function(F)
            , m_arguments(args)
            , m_image(image)
            , m_graphID(0)
        {
        }

        Expression::Expression(const Function* F, const ArgumentVector& args,
                               const std::string& gid)
            : m_function(F)
            , m_arguments(args)
            , m_image(0)
        {
            m_graphID = new char[gid.size() + 1];
            std::copy(gid.begin(), gid.end(), m_graphID);
            m_graphID[gid.size()] = 0;
        }

        Expression::~Expression()
        {
            for (size_t i = 0, s = m_arguments.size(); i < s; i++)
            {
                delete m_arguments[i];
            }

            delete[] m_graphID;
        }

        void Expression::output(std::ostream& o) const
        {
            o << m_function->callName() << "(";

            for (size_t i = 0; i < m_arguments.size(); i++)
            {
                BoundSymbol* s = m_arguments[i];

                if (i)
                    o << ", ";

                if (s)
                {
                    s->output(o);
                }
                else
                {
                    o << "_";
                }
            }

            o << ")";
        }

        void Expression::outputHash(std::ostream& o) const
        {
            o << m_function->callName() << "(";

            for (size_t i = 0; i < m_arguments.size(); i++)
            {
                BoundSymbol* s = m_arguments[i];

                if (i)
                    o << "$";

                if (s)
                {
                    s->outputHash(o);
                }
                else
                {
                    o << "_";
                }
            }

            o << ")";
        }

        bool Expression::lessThan(const Expression* other) const
        {
            //
            //  A topological ordering for std::map to use as a key. For any
            //  DAG this ordering will be unique (assuming no unconnected
            //  nodes exist).
            //
            //  Expression::function() is assumed to be a unique
            //  pointer. So pointer comparisons are ok. No other objects are
            //  assumed unique.
            //
            //  lessThan will work against mixed "bound" and "unbound"
            //  versions of Expression trees. Its only testing the
            //  tree structure not the bound parameters.
            //

            const Function* F = function();
            const Function* G = other->function();

            if (F < G)
                return true;
            if (F > G)
                return false;

            assert(m_arguments.size() == other->arguments().size());

            for (size_t i = 0, s = m_arguments.size(); i < s; i++)
            {
                BoundSymbol* Fbsym = m_arguments[i];
                BoundSymbol* Gbsym = other->arguments()[i];

                const bool isF = Fbsym->isExpression();
                const bool isG = Gbsym->isExpression();

                if (isF && isG)
                {
                    BoundExpression* Fe = static_cast<BoundExpression*>(Fbsym);
                    BoundExpression* Ge = static_cast<BoundExpression*>(Gbsym);

                    Expression* Fa = Fe->value();
                    Expression* Ga = Ge->value();

                    if (Fa->lessThan(Ga))
                        return true;
                    if (Ga->lessThan(Fa))
                        return false;
                }
                else if (isF || isG)
                {
                    //
                    //  one or both of them is 0 so its not required that Fe
                    //  and Ge to be unique. This is really just saying that
                    //  something is bigger than nothing.
                    //

                    return Fbsym < Gbsym;
                }
            }

            return false;
        }

        Expression* Expression::copy() const
        {
            size_t nargs = m_arguments.size();

            ArgumentVector args(nargs);

            for (size_t i = 0; i < nargs; i++)
            {
                args[i] = m_arguments[i]->copy();
            }

            return new Expression(m_function, args, m_image);
        }

        Expression* Expression::copyUnbound() const
        {
            size_t nargs = m_arguments.size();

            ArgumentVector args(nargs);

            for (size_t i = 0; i < nargs; i++)
            {
                BoundSymbol* s = m_arguments[i];

                if (BoundExpression* e = dynamic_cast<BoundExpression*>(s))
                {
                    assert(e->value());
                    args[i] = new BoundExpression(e->symbol(),
                                                  e->value()->copyUnbound());
                }
                else if (BoundImageCoord* b = dynamic_cast<BoundImageCoord*>(s))
                {
                    //
                    //  BoundImageCoords get converted into names. There is
                    //  some tricky timing here. The graphIDs are not
                    //  available until the entire IPImage tree is
                    //  complete. So during construction, only the IPImage
                    //  pointers themselves are stored in BoundImageCoord. In
                    //  addition IPImages are only valid for one evaluation
                    //  pass. So to keep a permanent record of which IPImage
                    //  coordinate system we need the graphID of the IPImage
                    //  which is stored in the "unbound" expression tree.
                    //
                    //  So i.e., if you want to keep a reference to an IPImage
                    //  *after* evaluation you save its graphID. You cannot
                    //  compare that graphID to a current IPImage unless its
                    //  graphID has been computed which can only happen after
                    //  the entire tree has been constructed. No comparisons
                    //  of graphIDs can occur during evaluation (in the nodes)
                    //

                    args[i] = new BoundImageCoordName(
                        s->symbol(),
                        ImageCoordName(b->value().image->graphID()));
                }
                else
                {
                    args[i] = new ProxyBoundSymbol(s->symbol());
                }
            }

            //
            //  NOTE: The unbound expressions have graphIDs instead of
            //  pointers to images. This makes it possible for Shader::Program
            //  to refer to them.
            //

            return new Expression(m_function, args, m_image->graphID());
        }

        Function::ResourceUsage
        Expression::computeResourceUsageRecursive() const
        {
            Function::ResourceUsage usage = m_function->resourceUsage();
            size_t nargs = m_arguments.size();

            for (size_t i = 0; i < nargs; i++)
            {
                if (const BoundExpression* e =
                        dynamic_cast<const BoundExpression*>(m_arguments[i]))
                {
                    const Expression* fexpr = e->value();
                    Function::ResourceUsage u =
                        e->value()->computeResourceUsageRecursive();
                    usage.coords += u.coords;
                    usage.buffers += u.buffers;

                    //
                    //  Filters multiply the number of fetches, everything
                    //  else sums them.
                    //

                    if (m_function->isFilter())
                    {
                        usage.fetches *= u.fetches;
                    }
                    else
                    {
                        usage.fetches += u.fetches;
                    }
                }
            }

            return usage;
        }

    } // namespace Shader
} // namespace IPCore
