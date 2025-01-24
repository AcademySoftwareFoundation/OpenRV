#ifndef __Mu__Name__h__
#define __Mu__Name__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/config.h>
#include <Mu/StringHashTable.h>
#include <iosfwd>

namespace Mu
{

    //
    //  class Name

    //
    //  Small class which points to an entry in a hash table.
    //

    class Name
    {
    public:
        MU_GC_NEW_DELETE
        typedef Mu::StringHashTable::Item Item;

        Name()
            : _item(0)
        {
        }

        Name(const Name& n)
            : _item(n._item)
        {
        }

        ~Name() { _item = (Item*)MU_DELETED; }

        //
        //	You can cast a Name as a const std::string or a bool (meaning
        //	that its empty for false)
        //

        operator bool() const { return _item ? true : false; }

        operator String() const { return _item ? _item->data() : "$noname$"; }
#ifdef MU_USE_BOEHM_COLLECTOR
        operator std::string() const
        {
            return _item ? _item->data().c_str() : "$noname$";
        }
#endif

        //
        //	Just like the std::string function
        //

        const char* c_str() const { return _item ? _item->data().c_str() : ""; }

        //
        //	Assingment does not keep track of which hash table a Name
        //	comes from. This could potentially be dangerous.
        //

        Name& operator=(Name n)
        {
            _item = n._item;
            return *this;
        }

        //
        //	Equality is only true between names that are in the same
        //	NamePool.
        //

        bool operator==(Name n) const { return _item == n._item; }

        bool operator!=(Name n) const { return _item != n._item; }

        bool operator<(Name n) const;

        static bool PointerSortKey(Name a, Name b);

        //
        //	For assertions you often need to compare against a string
        //	constant.
        //

        bool operator==(const char*) const;

        bool operator!=(const char* s) const { return !(*this == s); }

        bool operator==(const String&) const;

        bool operator!=(const String& s) const { return !(*this == s); }

        //
        //  substring comparisons
        //

        bool includes(const char*, size_t start = 0) const;

        //
        //	Returns a hash value for the Name
        //

        unsigned long hash() const;

        //
        // iostreams
        //

        friend std::ostream& operator<<(std::ostream& o, Name n);

        //
        //	struct for unions
        //

        typedef const Item* Ref;

        Ref nameRef() const { return _item; }

        Name(Ref r)
            : _item(r)
        {
        }

    private:
        // Name(const Item* item) : _item(item) {}
        friend class NamePool;

    private:
        const Item* _item;
    };

    //
    //  A qualified name is a name with scope separators in it. Otherwise
    //  its treated just like a name (interned, etc).
    //

    typedef Name QualifiedName;

    inline unsigned long Name::hash() const
    {
        return _item ? StringTraits::hash(_item->data()) : 0;
    }

    inline bool Name::operator==(const char* text) const
    {
        return _item ? _item->data() == text : false;
    }

    inline bool Name::operator==(const String& text) const
    {
        return _item ? _item->data() == text : false;
    }

    inline bool Name::operator<(Name n) const
    {
        return StringTraits::compare(_item->data(), String(n)) < 0;
    }

    //
    //  class NamePool
    //
    //  A hash table of names which returns Name objects.
    //

    class NamePool
    {
    public:
        MU_GC_NEW_DELETE
        NamePool() {}

        ~NamePool() {}

        Name intern(const String& s) { return Name(_hashTable.add(s)); }

        Name find(const String& s) const;
        bool exists(const String& s) const;

    private:
        StringHashTable _hashTable;
        typedef StringHashTable::Item Item;
    };

    inline Name NamePool::find(const String& s) const
    {
        if (const Item* i = _hashTable.find(s))
            return Name(i);
        return 0;
    }

    inline bool NamePool::exists(const String& s) const
    {
        return _hashTable.find(s) ? true : false;
    }

} // namespace Mu

#endif // __Mu__Name__h__
