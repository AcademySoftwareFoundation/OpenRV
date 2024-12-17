//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Archive.h>
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/Interface.h>
#include <Mu/InterfaceImp.h>
#include <Mu/MachineRep.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/VariantType.h>
#include <Mu/VariantInstance.h>
#include <Mu/SymbolTable.h>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <sstream>

namespace Mu
{
    using namespace std;

    Class::Class(Context* context, const char* name, Class* super)
        : Type(context, name, Mu::PointerRep::rep())
        , _frozen(false)
        , _instanceSize(0)
        , _nebulousAncestry(false)
    {
        _isMutable = true;
        if (super)
            addSuperClass(super);
        _isPrimitive = false;
    }

    Class::Class(Context* context, const char* name, const ClassVector& supers)
        : Type(context, name, Mu::PointerRep::rep())
        , _frozen(false)
        , _instanceSize(0)
        , _nebulousAncestry(false)
    {
        _isMutable = true;

        for (size_t i = 0; i < supers.size(); i++)
        {
            if (supers[i])
                addSuperClass(supers[i]);
        }

        _isPrimitive = false;
    }

    Class::~Class() {}

    void Class::addSuperClass(const Class* s)
    {
        _superClasses.push_back(s);
        s->_children.push_back(this);
    }

    void Class::addSymbol(Symbol* sym)
    {
        Type::addSymbol(sym);

        if (MemberVariable* m = dynamic_cast<MemberVariable*>(sym))
        {
            _memberVariables.push_back(m);
        }
    }

    const Symbol* Class::findSymbol(Name n) const
    {
        //
        //  Same as python's multiple inheritance.
        //

        if (const Symbol* s = Type::findSymbol(n))
        {
            return s;
        }

        for (size_t i = 0, s = _superClasses.size(); i < s; i++)
        {
            if (const Symbol* sym = _superClasses[i]->findSymbol(n))
            {
                return sym;
            }
        }

        for (size_t i = 0; i < _interfaces.size(); i++)
        {
            InterfaceImp* imp = _interfaces[i];

            if (const Symbol* s = imp->iface()->findSymbol(n))
            {
                return s;
            }
        }

        return 0;
    }

    void Class::findSymbols(QualifiedName name, SymbolVector& symbols)
    {
        Symbol::findSymbols(name, symbols);

        for (size_t i = 0, s = _superClasses.size(); i < s; i++)
        {
            const_cast<Class*>(_superClasses[i])->findSymbols(name, symbols);
        }
    }

    void Class::findSymbols(QualifiedName name,
                            ConstSymbolVector& symbols) const
    {
        Symbol::findSymbols(name, symbols);

        for (size_t i = 0, s = _superClasses.size(); i < s; i++)
        {
            _superClasses[i]->findSymbols(name, symbols);
        }
    }

    void Class::deleteObject(Object* obj) const
    {
        // delete static_cast<ClassInstance*>(obj);
        return ClassInstance::deallocate(static_cast<ClassInstance*>(obj));
    }

    Object* Class::newObject() const
    {
        // return new ClassInstance(this);
        return ClassInstance::allocate(this);
    }

    size_t Class::objectSize() const
    {
        if (!isFrozen())
        {
            Class* c = const_cast<Class*>(this);
            c->freeze();
        }

        return instanceSize() + sizeof(ClassInstance);
    }

    static bool memberMatch(const Signature* a, const Signature* b)
    {
        if (a->size() != b->size())
            return false;

        for (int i = 0; i < a->size(); i++)
        {
            if (i == 1)
                continue;
            if ((*a)[i].symbol != (*b)[i].symbol)
                return false;
        }

        return true;
    }

    Value Class::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._PointerFunc)(*n, thread));
    }

    void Class::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        Pointer* pp = reinterpret_cast<Pointer*>(p);
        *pp = (*n->func()._PointerFunc)(*n, thread);
    }

    void Class::outputValueRecursive(ostream& o, const ValuePointer valuePtr,
                                     ValueOutputState& state) const
    {
        if (!valuePtr)
            return;
        ClassInstance* obj = *reinterpret_cast<ClassInstance**>(valuePtr);

        if (obj)
        {
            o << fullyQualifiedName() << " {";

            if (state.traversedObjects.find(obj)
                != state.traversedObjects.end())
            {
                o << "...ad infinitum...";
            }
            else
            {
                state.traversedObjects.insert(obj);

                for (int i = 0, s = _memberVariables.size(); i < s; i++)
                {
                    if (!_memberVariables[i]->isHidden())
                    {
                        if (i)
                            o << ", ";
                        // o << _memberVariables[i]->name() << "=";
                        const Type* t = fieldType(i);
                        t->outputValueRecursive(o, obj->field(i), state);
                    }
                }

                // state.traversedObjects.erase(obj);
            }

            o << "}";
        }
        else
        {
            o << "nil";
        }
    }

    void Class::outputValue(ostream& o, const Value& value, bool full) const
    {
        ValueOutputState state(o, full);
        outputValueRecursive(o, ValuePointer(&value._Pointer), state);
    }

    void Class::symbolDependancies(ConstSymbolVector& symbols) const
    {
        if (symbolState() != ResolvedState)
            resolveSymbols();

        for (int i = 0; i < _memberVariables.size(); i++)
        {
            const MemberVariable* v = _memberVariables[i];
            symbols.push_back(v->storageClass());
        }
    }

    const Type* Class::fieldType(size_t t) const
    {
        if (t >= _memberVariables.size())
            return 0;
        return _memberVariables[t]->storageClass();
    }

    ValuePointer Class::fieldPointer(Object* o, size_t index) const
    {
        if (index >= _memberVariables.size())
            return 0;
        ClassInstance* i = static_cast<ClassInstance*>(o);
        return i->field(index);
    }

    const ValuePointer Class::fieldPointer(const Object* o, size_t index) const
    {
        if (index >= _memberVariables.size())
            return 0;
        const ClassInstance* i = static_cast<const ClassInstance*>(o);
        return i->field(index);
    }

    const Function* Class::findFunction(Name name, const Signature* sig) const
    {
        typedef SymbolTable::SymbolHashTable HT;

        if (symbolTable())
        {
            for (SymbolTable::Iterator i(symbolTable()); i; ++i)
            {
                const Symbol* s = *i;

                if (s->name() == name)
                {
                    for (s = s->firstOverload(); s; s = s->nextOverload())
                    {
                        if (const Function* f =
                                dynamic_cast<const Function*>(s))
                        {
                            if (memberMatch(sig, f->signature()))
                            {
                                return f;
                            }
                        }
                    }
                }
            }
        }

        for (size_t i = 0, s = _superClasses.size(); i < s; i++)
        {
            if (const Function* f = _superClasses[i]->findFunction(name, sig))
            {
                return f;
            }
        }

        return 0;
    }

    bool Class::isA(const Class* c) const
    {
        if (_nebulousAncestry)
        {
            return nebulousIsA(c);
        }
        else
        {
            if (c == this)
                return true;

            for (size_t i = 0, s = _superClasses.size(); i < s; i++)
            {
                if (_superClasses[i]->isA(c))
                    return true;
            }
        }

        return false;
    }

    bool Class::substitutable(const Class* c) const
    {
        if (this == c)
        {
            return true;
        }
        else if (!_superClasses.empty() && isA(c))
        {
            return _superClasses.front()->substitutable(c);
        }
        else
        {
            return false;
        }
    }

    ClassInstance* Class::dynamicCast(ClassInstance* obj, const Class* c,
                                      bool upcastOK) const
    {
        if (_nebulousAncestry)
        {
            return obj;
        }
        else
        {
            if (c == this)
                return obj;

            for (size_t i = 0, s = _superClasses.size(); i < s; i++)
            {
                const Class* super = _superClasses[i];
                ClassInstance* sobj =
                    i ? (reinterpret_cast<ClassInstance*>(obj->structure()
                                                          + _superOffsets[i]))
                      : obj;

                if (sobj = super->dynamicCast(sobj, c, false))
                {
                    return sobj;
                }
            }
        }

        //
        //  NOTE: this is funky: we're using the GC to determine what the base
        //  pointer of the GC object is to resurrect the base pointer to the
        //  most derived class. We can't completely assume that the base
        //  pointer points to a ClassInstance -- it could also be a
        //  VariantInstance -- but we can assume its an Object.
        //

        if (upcastOK)
        {
            Object* base = reinterpret_cast<Object*>(GC_base(obj));

            if (obj != base)
            {
                //
                //  The object is embedded in another one
                //

#if 0
            cout << "embedded " 
                 << obj->type()->fullyQualifiedName()
                 << " inside of "
                 << base->type()->fullyQualifiedName()
                 << endl;
#endif

                while (dynamic_cast<const VariantTagType*>(base->type()))
                {
                    //
                    //  Bump it to the VariantInstance's enclosed object
                    //  until we have something that's not a
                    //  VariantInstance
                    //

                    VariantInstance* iobj = static_cast<VariantInstance*>(base);
                    base = iobj->data<Object>();
                }

                if (dynamic_cast<const Class*>(base->type()))
                {
                    //
                    //  Assume this is the most derived class.
                    //
                    //  NOTE: this is assuming that multiple inheritance
                    //  classes are never embedded inside each other.
                    //

                    ClassInstance* bobj = static_cast<ClassInstance*>(base);
                    const Class* btype = bobj->classType();

                    //
                    //  If the root is the target then we're done
                    //

                    if (btype == c)
                        return bobj;

                    //
                    //  Find the partition in the object from which the
                    //  original pointer came. Bump the object to that super
                    //  class and then recursively call this function to
                    //  downcast to the right value.
                    //

                    const Class::ClassVector& supers = btype->superClasses();
                    const Class::ClassOffsets& offsets = btype->superOffsets();

                    const size_t off = (unsigned char*)obj - bobj->structure();

                    for (size_t i = 1; i < offsets.size(); i++)
                    {
                        if (off < offsets[i])
                        {
                            if (i == 1)
                            {
                                return supers.front()->dynamicCast(bobj, c,
                                                                   false);
                            }
                            else
                            {
                                ClassInstance* sobj =
                                    reinterpret_cast<ClassInstance*>(
                                        bobj->structure() + offsets[i - 1]);

                                return supers[i - 1]->dynamicCast(sobj, c,
                                                                  false);
                            }
                        }
                        else if (i == offsets.size() - 1
                                 && i < btype->instanceSize())
                        {
                            ClassInstance* sobj =
                                reinterpret_cast<ClassInstance*>(
                                    bobj->structure() + offsets[i]);

                            return supers[i]->dynamicCast(sobj, c, false);
                        }
                    }
                }
            }
        }

        return 0;
    }

    bool Class::nebulousIsA(const Class*) const
    {
        cerr << "IMPLEMENTATION ERROR: Class " << name()
             << " did not implement Class::nebulousIsA()" << endl;
        abort();

        /* AJG - added to appease "return required" */
        return 0;
    }

    Type::MatchResult Class::match(const Type* type, Bindings& b) const
    {
        if (const Class* c = dynamic_cast<const Class*>(type))
        {
            return c->isA(this) ? Match : NoMatch;
        }
        else
        {
            return Type::match(type, b);
        }
    }

    const MemberFunction* Class::dynamicLookup(const MemberFunction* F) const
    {
        //
        //  NOTE: if the member function is overloaded you MUST implement
        //  all overloads together. Otherwise this function will find some
        //  weird combo of functions.
        //

        if (const Symbol* sym = findSymbol(F->name()))
        {
            for (const Symbol* s = sym; s; s = s->nextOverload())
            {
                if (const MemberFunction* f =
                        dynamic_cast<const MemberFunction*>(s))
                {
                    if (memberMatch(f->signature(), F->signature()))
                        return f;
                }
            }
        }

        return 0;
    }

    void Class::freeze()
    {
        typedef SymbolTable::SymbolHashTable HT;
        if (_frozen)
            return;

        //
        //	Must freeze the super class in order to freeze this class.
        //

        _frozen = true;

        for (size_t i = 0, s = _superClasses.size(); i < s; i++)
        {
            Class* super = const_cast<Class*>(_superClasses[i]);
            if (super && !super->isFrozen())
                super->freeze();
        }

        //
        //	Compute the size of the instance. If any member var type is
        //  not GCAtomic then the class is also not GCAtomic.
        //
        //  The first portion of the object is just a single inheritance
        //  hierarchy. Additional super classes are appened after the main
        //  lineage.
        //
        //  +------------------------------+
        //  | This Class Pointer           |    <- valid This ClassInstance
        //  +------------------------------+
        //  | Super lineage state          |
        //  +------------------------------+
        //  | This state                   |
        //  +------------------------------+
        //  | 2nd Super Class pointer      |    <- valid 2nd Super ClassInstance
        //  +------------------------------+
        //  | 2nd Super lineage state      |
        //  | ...                          |
        //  +------------------------------+
        //  | 3rd Super Class pointer      |    <- valid 3rd Super ClassInstance
        //  +------------------------------+
        //  | 3rd Super lineage state      |
        //  | ...                          |
        //  +------------------------------+
        //
        //  So if you have a single inheritance situation (usual) then you
        //  get a layout with a single class pointer at the top and all of
        //  the state in order as you go down.
        //
        //  NOTE: we assume its ok to be GCAtomic for heap objects
        //  pointing to symbols. This is because by definition the symbols
        //  will exist in a context which will reference them. The context
        //  absolutely will outlive any heap object.
        //

        size_t start = 0;
        _isGCAtomic =
            !_superClasses.empty() ? _superClasses.front()->isGCAtomic() : true;
        Context* c = context();
        MemberVariableVector mvars;
        _superOffsets.resize(_superClasses.size());
        if (!_superClasses.empty())
            _superOffsets.front() = 0;

        //
        //  Insert new member variable for each in the super classes
        //

        for (size_t i = 0; i < _superClasses.size(); i++)
        {
            const Class* s = _superClasses[i];
            size_t size = s->_memberVariables.size();
            mvars.resize(size);

            if (i != 0)
            {
                //
                //  Add in the object header before each non-first super
                //  class
                //

                String n = "__";
                n += s->name().c_str();

                _superOffsets[i] = _memberVariables.size();
                InternalTypeMemberVariable* im =
                    new InternalTypeMemberVariable(c, n.c_str(), s);

                _typeMembers.push_back(im);
                _memberVariables.push_back(im);
                Symbol::addSymbol(_memberVariables.back());
            }

            for (size_t q = 0; q < size; q++)
            {
                MemberVariable* m0 = s->_memberVariables[q];
                MemberVariable* m = 0;

                if (InternalTypeMemberVariable* tm =
                        dynamic_cast<InternalTypeMemberVariable*>(m0))
                {
                    //
                    //  This is coming from one of the base classes. Its
                    //  an internal type member to that class (which has
                    //  multiple inheritance). We'll need to initialize
                    //  this in ClassInstance.
                    //

                    InternalTypeMemberVariable* im =
                        new InternalTypeMemberVariable(c, m0->name().c_str(),
                                                       tm->value());
                    m = im;
                    _typeMembers.push_back(im);
                }
                else
                {
                    m = new MemberVariable(c, m0->name().c_str(),
                                           m0->storageClassName().c_str());
                }

                Symbol::addSymbol(m);
                mvars[q] = m;
            }

            size_t pos = i ? _memberVariables.size() : 0;
            _memberVariables.insert(_memberVariables.begin() + pos,
                                    mvars.begin(), mvars.end());
        }

        //
        //  Compute the actual byte offsets
        //

        for (size_t i = 0; i < _memberVariables.size(); i++)
        {
            MemberVariable* v = _memberVariables[i];
            const MachineRep* rep = v->storageClass()->machineRep();
            size_t alignment = rep->structAlignment();
            v->setAddress(i);

            if (rep == PointerRep::rep())
                _isGCAtomic = false;

            while (start % alignment != 0)
                start++;
            v->_instanceOffset = start;
            start += rep->size();
        }

        _instanceSize = start;

        if (!_memberVariables.empty())
        {
            for (size_t i = 0; i < _superClasses.size(); i++)
            {
                _superOffsets[i] =
                    _memberVariables[_superOffsets[i]]->instanceOffset();
            }
        }
    }

    const InterfaceImp* Class::implementation(const Interface* intface) const
    {
        //
        //	Try to find the interface implementation
        //	do fastest case first then search list and sort
        //

        if (_interfaces.size())
        {
            if (_interfaces.front()->iface() == intface)
            {
                return _interfaces.front();
            }

            for (int i = 1; i < _interfaces.size(); i++)
            {
                InterfaceImp* imp = _interfaces[i];

                if (imp->iface() == intface)
                {
                    //
                    // Bubble sort by usage
                    //

                    InterfaceImp* imp2 = _interfaces[i - 1];
                    _interfaces[i - 1] = imp;
                    _interfaces[i] = imp2;

                    return imp;
                }
            }
        }

        if (InterfaceImp* ii = intface->construct(this))
        {
            _interfaces.push_back(ii);
            return ii;
        }
        else
        {
            return 0;
        }
    }

    void Class::findOverridingFunctions(const MemberFunction* mf,
                                        MemberFunctionVector& funcs) const
    {
        const Class::ClassVector& derived = derivedClasses();
        // cout << "Looking in ";
        // output(cout);
        // cout << endl;

        for (int i = 0; i < derived.size(); i++)
        {
            const Class* d = derived[i];

            if (const Symbol* fo = d->findSymbolOfType<Symbol>(mf->name()))
            {
                for (const Symbol* s = fo->firstOverload(); s;
                     s = s->nextOverload())
                {
                    if (const MemberFunction* sf =
                            dynamic_cast<const MemberFunction*>(s))
                    {
                        // cout << "  comparing ";
                        // sf->output(cout);
                        // cout << endl;

                        if (sf->offset() == mf->offset())
                        {
                            funcs.push_back(sf);
                        }
                    }
                }
            }

            d->findOverridingFunctions(mf, funcs);
        }
    }

    void Class::constructInstance(Pointer p) const
    {
        new (p) ClassInstance(this);
    }

    void Class::copyInstance(Pointer a, Pointer b) const
    {
        memcpy(b, a, objectSize());
    }

    bool Class::isInBaseClass(const MemberVariable* v) const
    {
        for (size_t i = 0; i < _superClasses.size(); i++)
        {
            const Class* super = _superClasses[i];
            if (super->findSymbolOfType<MemberVariable>(v->name()))
                return true;
            else if (super->isInBaseClass(v))
                return true;
        }

        return false;
    }

    void Class::allMemberVariables(MemberVariableVector& vars) const
    {
        // if (_superClasses.empty())
        // {
        copy(_memberVariables.begin(), _memberVariables.end(),
             back_inserter(vars));
        // }
        // else
        // {
        //     for (size_t i = 0; i < _superClasses.size(); i++)
        //     {
        //         _superClasses[i]->allMemberVariables(vars);
        //         if (i == 0) copy(_memberVariables.begin(),
        //                          _memberVariables.end(),
        //                          back_inserter(vars));
        //     }
        // }
    }

} // namespace Mu
