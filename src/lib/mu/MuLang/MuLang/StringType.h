#ifndef __MuLang__StringType__h__
#define __MuLang__StringType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0 
// 
#include <Mu/config.h>
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/Node.h>
#include <iosfwd>
#include <string>

#ifdef MU_USE_PCRE
    #include <pcre2.h>
    #include <pcre2posix.h>
#else
    #include <regex.h>
#endif

namespace Mu {

//
//  class StringType
//
//  A mutable string type.
//

class StringType : public Class
{
  public:
    StringType(Context*, Class *super=0);
    ~StringType();

    MU_GC_NEW_DELETE

    class String : public ClassInstance
    {
      public:
	String(const Class*);

        const char* c_str() const { return _utf8string; }
        int compare(const char*) const;
        unsigned long hash() const;

        bool operator==(const char*) const;
        bool operator==(const Mu::StringType::String&) const;
        bool operator!=(const char*) const;
        bool operator!=(const Mu::StringType::String&) const;

        //
        //  Size in bytes
        //

        size_t size() const { return _utf8string ? strlen(_utf8string) : 0; }

        //
        //  Size in chars
        //

        size_t numChars() const;

        //
        //  GC'd string
        //

        UTF32String utf32() const;
        UTF16String utf16() const;
        UTF8String utf8() const { return _utf8string; }

        //
        //  std::string
        //

        stdUTF32String utf32std() const;
        stdUTF16String utf16std() const;
        stdUTF8String utf8std() const { return _utf8string; }

        //
        //  These use GC memory
        //

        void copyConvert(UTF32String&) const;
        void copyConvert(UTF16String&) const;
        void copyConvert(UTF8String&) const;

        //
        //  These are std::string
        //

        void copyConvert(stdUTF32String&) const;
        void copyConvert(stdUTF16String&) const;
        void copyConvert(stdUTF8String&) const;

        void set(const std::string& s) { setn(s.c_str(), s.size()); }
        void set(const Mu::String& s) { setn(s.c_str(), s.size()); }
        void set(const Name& s) { set(s.c_str()); }
        void set(const char* s);

      private:
        void setn(const char* s, size_t size);
        const char* _utf8string;
	friend class StringType;
    };

    //
    //  Allocate a string
    //

    StringType::String* allocate(const Mu::String&) const;
    StringType::String* allocate(const Mu::Name&) const;
    StringType::String* allocate(const std::string&) const;
    StringType::String* allocate(const std::ostringstream&) const;
    StringType::String* allocate(const char*) const;
    StringType::String* allocate(size_t) const;

    //
    //	Return a new Object
    //

    virtual Object* newObject() const;
    virtual size_t objectSize() const;
    virtual void deleteObject(Object*) const;
    virtual void constructInstance(Pointer) const;
    virtual void copyInstance(Pointer, Pointer) const;
    virtual void freeze();

    //
    // Output the appropriate Value in human readable form (note: you
    // can call the static function if you already have a std::string.
    //

    virtual void outputValue(std::ostream&, const Value&, bool full=false) const;
    virtual void outputValueRecursive(std::ostream&,
                                      const ValuePointer,
                                      ValueOutputState&) const;
    static void outputQuotedString(std::ostream&, 
                                   const Mu::String&,
                                   char delim='"');

    //
    //  Archiving.
    //

    virtual void                serialize(std::ostream&,
                                          Archive::Writer&,
                                          const ValuePointer) const;

    virtual void                deserialize(std::istream&, 
                                            Archive::Reader&,
                                            ValuePointer) const;

    //
    //	Load function is called when the symbol is added to the
    //	context.
    //

    virtual void load();

    //
    //	Constant
    //

    static NODE_DECLARATION(construct,Pointer);
    static NODE_DECLARATION(from_int,Pointer);
    static NODE_DECLARATION(from_int64,Pointer);
    static NODE_DECLARATION(from_float,Pointer);
    static NODE_DECLARATION(from_double,Pointer);
    static NODE_DECLARATION(from_bool,Pointer);
    static NODE_DECLARATION(from_vector4,Pointer);
    static NODE_DECLARATION(from_vector3,Pointer);
    static NODE_DECLARATION(from_vector2,Pointer);
    static NODE_DECLARATION(from_string,Pointer);
    static NODE_DECLARATION(from_byte,Pointer);
    static NODE_DECLARATION(to_int,int);
    static NODE_DECLARATION(to_float,float);
    static NODE_DECLARATION(to_double,double);
    static NODE_DECLARATION(to_bool,bool);
    static NODE_DECLARATION(dereference,Pointer);
    static NODE_DECLARATION(from_class,Pointer);
    static NODE_DECLARATION(from_opaque,Pointer);
    static NODE_DECLARATION(from_variant,Pointer);
    static NODE_DECLARATION(print,void);
    static NODE_DECLARATION(assignPlus,Pointer);
    static NODE_DECLARATION(equals,bool);
    static NODE_DECLARATION(notequals,bool);
    static NODE_DECLARATION(hash,int);
    static NODE_DECLARATION(plus,Pointer);
    static NODE_DECLARATION(size,int);
    static NODE_DECLARATION(substr,Pointer);
    static NODE_DECLARATION(copy,Pointer);
    static NODE_DECLARATION(index,int);
    static NODE_DECLARATION(split,Pointer);
    static NODE_DECLARATION(join_array,Pointer);
    static NODE_DECLARATION(compare,int);

    static NODE_DECLARATION(formatOp,Pointer);
    static NODE_DECLARATION(formatOp_int,Pointer);
    static NODE_DECLARATION(formatOp_int64,Pointer);
    static NODE_DECLARATION(formatOp_float,Pointer);
    static NODE_DECLARATION(formatOp_double,Pointer);
    static NODE_DECLARATION(formatOp_half,Pointer);
    static NODE_DECLARATION(formatOp_short,Pointer);
    static NODE_DECLARATION(formatOp_byte,Pointer);
    static NODE_DECLARATION(formatOp_char,Pointer);
    static NODE_DECLARATION(formatOp_bool,Pointer);
    static NODE_DECLARATION(formatOp_Vector4f,Pointer);
    static NODE_DECLARATION(formatOp_Vector3f,Pointer);
    static NODE_DECLARATION(formatOp_Vector2f,Pointer);
    static NODE_DECLARATION(formatOp_charArray,Pointer);
    static NODE_DECLARATION(formatOp_class,Pointer);
    static NODE_DECLARATION(formatOp_opaque,Pointer);

    static bool         _init;
    static regex_t      _format_re;
};

} // namespace Mu



#endif // __MuLang__StringType__h__
