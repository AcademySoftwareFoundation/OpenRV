//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Class.h>
#include <Mu/Context.h>
#include <Mu/FunctionType.h>
#include <Mu/Interface.h>
#include <Mu/ListType.h>
#include <Mu/MachineRep.h>
#include <Mu/Module.h>
#include <Mu/NilType.h>
#include <Mu/OpaqueType.h>
#include <Mu/ReferenceType.h>
#include <Mu/TupleType.h>
#include <Mu/TypePattern.h>
#include <Mu/Value.h>
#include <Mu/VariantTagType.h>
#include <Mu/VariantType.h>
#include <iostream>
#include <string.h>

namespace Mu
{
    using namespace std;

    TypePattern::TypePattern(Context* context, const char* tname)
        : Type(context, tname, VoidRep::rep())
        , _variadic(false)
    {
        _isTypePattern = true;
    }

    Object* TypePattern::newObject() const { return 0; }

    void TypePattern::outputValue(std::ostream& o, Value& value) const
    {
        o << "\"" << name() << "\" is a type pattern";
    }

    void TypePattern::argumentAdjust(int& index, int& funcIndex) const
    {
        //
        // The symbol table asks pseudo types how to advance the
        // index into the functions argument list. This allows the
        // the type to define special "repeater" patterns. The
        // default is to simply allow the index to advance to the next
        // argument after the pseudo type.
        //

        return;
    }

    Value TypePattern::nodeEval(const Node* n, Thread& t) const
    {
        //
        //  An exception *should* be thrown by whatever function is called
        //  here.
        //

        (*n->func()._voidFunc)(*n, t);
        return Value();
    }

    void TypePattern::nodeEval(void*, const Node*, Thread& t) const { return; }

    const Type* TypePattern::nodeReturnType(const Node*) const
    {
        if (globalModule())
        {
            return globalModule()->context()->unresolvedType();
        }
        else
        {
            return this;
        }
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyThing::match(const Type* other, Bindings&) const
    {
        return Match;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyType::match(const Type* other, Bindings&) const
    {
        //
        // matches anything other than another type pattern
        //

        return other->isTypePattern() ? NoMatch : Match;
    }

    //----------------------------------------------------------------------

    Type::MatchResult VarArg::match(const Type* other, Bindings&) const
    {
        return Match;
    }

    void VarArg::argumentAdjust(int& index, int& funcIndex) const
    {
        //
        //	This will tell the symbol table to repeat the comparison
        //	with the "..." vararg type ad infinitum
        //

        funcIndex--;
    }

    //----------------------------------------------------------------------

    Type::MatchResult OneRepeatedArg::match(const Type* other, Bindings&) const
    {
        return Match;
    }

    void OneRepeatedArg::argumentAdjust(int& index, int& funcIndex) const
    {
        //
        //	This will tell the symbol table to repeat the comparison
        //	for the last argument forever -- note that you can't use
        //  "+" as the type of the first argument of a function,
        //  it will simply be ignored.
        //

        if (index && funcIndex)
        {
            funcIndex -= 2;
            index--;
        }
    }

    //----------------------------------------------------------------------

    Type::MatchResult TwoRepeatedArg::match(const Type* other, Bindings&) const
    {
        return Match;
    }

    void TwoRepeatedArg::argumentAdjust(int& index, int& funcIndex) const
    {
        //
        //	This will tell the symbol table to repeat the comparison for
        //	the last 2 arguments forever. Requires at least 2 preceding
        //	arguments.
        //

        if (index && funcIndex)
        {
            if (funcIndex < 3)
            {
                cerr << "TwoRepeatedArg::argumentAdjust(): "
                        "requires two preceding arguments.\n";
            }

            funcIndex -= 3;
            index--;
        }
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyClass::match(const Type* other, Bindings&) const
    {
        return dynamic_cast<const Class*>(other) != 0 ? Match : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyTuple::match(const Type* other, Bindings&) const
    {
        if (dynamic_cast<const TupleType*>(other))
        {
            return Match;
        }

        return NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyObjectButNotTuple::match(const Type* other,
                                                       Bindings&) const
    {
        if (dynamic_cast<const Class*>(other)
            || dynamic_cast<const VariantType*>(other))
        {
            if (dynamic_cast<const TupleType*>(other))
            {
                return NoMatch;
            }
            else
            {
                return Match;
            }
        }

        return NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyClassButNotTuple::match(const Type* other,
                                                      Bindings&) const
    {
        if (const Class* c = dynamic_cast<const Class*>(other))
        {
            if (dynamic_cast<const TupleType*>(c))
            {
                return NoMatch;
            }
            else
            {
                return Match;
            }
        }

        return NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyClassButNotTupleOrList::match(const Type* other,
                                                            Bindings&) const
    {
        if (const Class* c = dynamic_cast<const Class*>(other))
        {
            if (dynamic_cast<const TupleType*>(c)
                || dynamic_cast<const ListType*>(c))
            {
                return NoMatch;
            }
            else
            {
                return Match;
            }
        }

        return NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyInterface::match(const Type* other,
                                               Bindings&) const
    {
        return dynamic_cast<const Interface*>(other) != 0 ? Match : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyClassOrInterface::match(const Type* other,
                                                      Bindings&) const
    {
        return (dynamic_cast<const Interface*>(other) != 0
                || dynamic_cast<const Class*>(other) != 0)
                   ? Match
                   : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchNonPrimitiveOrNil::match(const Type* other,
                                                    Bindings&) const
    {
        return !other->isPrimitiveType() ? Match : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyReference::match(const Type* other,
                                               Bindings&) const
    {
        return dynamic_cast<const ReferenceType*>(other) != 0 ? Match : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyNonPrimitiveReference::match(const Type* other,
                                                           Bindings&) const
    {
        const ReferenceType* rtype = dynamic_cast<const ReferenceType*>(other);
        return (rtype && !rtype->dereferenceType()->isPrimitiveType())
                   ? Match
                   : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyFunction::match(const Type* other,
                                              Bindings&) const
    {
        return dynamic_cast<const FunctionType*>(other) != 0 ? Match : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchAnyVariant::match(const Type* other, Bindings&) const
    {
        return dynamic_cast<const VariantType*>(other) != 0 ? Match : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchList::match(const Type* other, Bindings&) const
    {
        return dynamic_cast<const ListType*>(other) != 0 ? Match : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchABoolRep::match(const Type* other, Bindings&) const
    {
        return other->machineRep() == BoolRep::rep() ? Match : NoMatch;
    }

    //----------------------------------------------------------------------

    Type::MatchResult MatchOpaque::match(const Type* other, Bindings&) const
    {
        return dynamic_cast<const OpaqueType*>(other) != 0 ? Match : NoMatch;
    }

} // namespace Mu
