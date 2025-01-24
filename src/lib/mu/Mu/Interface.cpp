//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ClassInstance.h>
#include <Mu/Function.h>
#include <Mu/Interface.h>
#include <Mu/InterfaceImp.h>
#include <Mu/MachineRep.h>
#include <Mu/SymbolTable.h>

namespace Mu
{
    using namespace std;

    Interface::Interface(Context* context, const char* name)
        : Type(context, name, Mu::PointerRep::rep())
        , _numFunctions(0)
    {
    }

    Interface::Interface(Context* context, const char* name,
                         const Interfaces& interfaces)
        : Type(context, name, Mu::PointerRep::rep())
        , _numFunctions(0)
    {
    }

    Interface::~Interface() {}

    Object* Interface::newObject() const
    {
        //
        //  Interfaces cannot be created
        //

        return 0;
    }

    void Interface::deleteObject(Object*) const {}

    void Interface::output(std::ostream& o) const
    {
        o << "interface " << fullyQualifiedName();
    }

    void Interface::outputNode(std::ostream& o, const Node* n) const
    {
        const DataNode* dn = static_cast<const DataNode*>(n);
        output(o);
        o << " = ";
        outputValue(o, dn->_data);
    }

    void Interface::outputValue(std::ostream& o, const Value& value,
                                bool full) const
    {
        ValueOutputState state(o, full);
        outputValueRecursive(o, ValuePointer(&value._Pointer), state);
    }

    void Interface::outputValueRecursive(std::ostream& o, const ValuePointer p,
                                         ValueOutputState& state) const
    {
        ClassInstance* i = *reinterpret_cast<ClassInstance**>(p);

        if (i)
        {
            i->type()->outputValueRecursive(o, p, state);
        }
        else
        {
            o << "nil";
        }
    }

    Value Interface::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._PointerFunc)(*n, thread));
    }

    void Interface::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        Pointer* pp = reinterpret_cast<Pointer*>(p);
        *pp = (*n->func()._PointerFunc)(*n, thread);
    }

    void Interface::addSymbol(Symbol* s)
    {
        Type::addSymbol(s);

        if (Function* f = dynamic_cast<Function*>(s))
        {
            f->setInterfaceIndex(_numFunctions);
            _numFunctions++;
        }
    }

    const Symbol* Interface::findSymbol(Name n) const
    {
        return Type::findSymbol(n);
    }

    void Interface::freeze() {}

    InterfaceImp* Interface::construct(const Class* c) const
    {
        _frozen = true;

        //
        //  Search the class for functions with the same name and
        //  signature. Enter the NodeFuncs.
        //

        STLVector<const Function*>::Type functions(_numFunctions);

        if (symbolTable())
        {
            for (SymbolTable::Iterator i(symbolTable()); i; ++i)
            {
                const Symbol* s = *i;

                for (s = s->firstOverload(); s; s = s->nextOverload())
                {
                    const Function* f = 0;
                    const Function* cf = 0;

                    if (f = dynamic_cast<const Function*>(s))
                    {
                        if (cf = c->findFunction(f->name(), f->signature()))
                        {
                            //
                            //  Associate the class function with the
                            //  interface function.
                            //

                            functions[f->interfaceIndex()] = cf;
                        }
                        else
                        {
                            if (!f->native() && !f->body())
                            {
                                //
                                //  Fail. class does not implement it
                                //

                                return 0;
                            }

                            if (f->func())
                            {
                                //
                                //  Default implementation
                                //

                                functions[f->interfaceIndex()] = f;
                            }
                            else
                            {
                                //
                                //  Fail. class does not implement it
                                //

                                return 0;
                            }
                        }
                    }
                }
            }
        }

        InterfaceImp* ii = new InterfaceImp(c, this);

        for (int i = 0; i < functions.size(); i++)
        {
            ii->_vtable[i] = functions[i]->func(0);
        }

        return ii;
    }

    bool Interface::match(const Type* type) const
    {
        if (const Class* c = dynamic_cast<const Class*>(type))
        {
            return c->implementation(this) != 0;
        }
        else
        {
            return Type::match(type);
        }
    }

} // namespace Mu
