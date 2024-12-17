//
// Copyright (c) 2009, Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __Gto__Writer__h__
#define __Gto__Writer__h__
#include <Gto/Header.h>
#include <Gto/Utilities.h>
#include <assert.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <stdarg.h>
#include <string.h>

namespace Gto
{

    //
    //  class Gto::Writer
    //
    //  Typically, you'll make a Writer on the stack and call its
    //  functions. This class is not intended to be inherited from.
    //

    class Writer
    {
    public:
        typedef std::vector<std::string> StringVector;

        struct PropertyPath
        {
            PropertyPath(size_t a = size_t(-1), const std::string& cname = "",
                         const StringVector& scope = StringVector(1))
                : objectIndex(a)
                , componentName(cname)
                , componentScope(scope)
            {
            }

            size_t objectIndex;
            std::string componentName;
            StringVector componentScope;
        };

        typedef std::map<std::string, int> StringMap;
        typedef std::vector<ComponentHeader> Components;
        typedef std::vector<PropertyHeader> Properties;
        typedef std::vector<ObjectHeader> Objects;
        typedef std::map<size_t, PropertyPath> PropertyMap;

        enum FileType
        {
            BinaryGTO,
            CompressedGTO,
            TextGTO
        };

        Writer();
        explicit Writer(std::ostream&);
        ~Writer();

        //
        //  Optional open function which takes a filename.
        //

        bool open(const char* filename, FileType mode = CompressedGTO);
        bool open(std::ostream&, FileType mode = CompressedGTO);

        //
        //  Deprecated open API
        //

        bool open(const char* f, bool c)
        {
            return open(f, c ? CompressedGTO : BinaryGTO);
        }

        //
        //  Close stream if applicable.
        //

        void close();

        //
        //  Each object in the file has both a name and protocol. The
        //  protocol is a user defined string which tells software how to
        //  interpret the object data. Examples are: "catmull-clark",
        //  "polygon", "loop", "NURBS", "particle".
        //

        void beginObject(const char* name, const char* protocol,
                         uint32 protocolVersion);

        //
        //  beginComponent() -- declare a component. This can only be
        //  called after begin() and before end(). You call one or the
        //  other of these functions.
        //
        //  Components can be nested.
        //

        void beginComponent(const char* name, uint32 flags = 0);
        void beginComponent(const char* name, const char* interp,
                            uint32 flags = 0);

        //
        //  delcare a property of a component
        //

        void property(const char* name, Gto::DataType, size_t numElements,
                      const Dimensions& dims = Dimensions(),
                      const char* interp = 0);

        //
        //  For backwards compatibility
        //

        void property(const char* name, Gto::DataType type, size_t numElements,
                      size_t width, const char* interp = 0)
        {
            property(name, type, numElements, Dimensions(width, 0, 0, 0),
                     interp);
        }

        void endComponent();

        //

        //

        void endObject();

        //
        //  String table entries --- if you have a string property, you
        //  need to add all the strings. Its ok to add them multiple
        //  times.
        //

        void intern(const char*);
        void intern(const std::string&);

        uint32 lookup(const char*) const;
        uint32 lookup(const std::string&) const;

        std::string lookup(uint32) const;

        //
        //  Finish of declaration section
        //

        void beginData();

        //
        // If you want to specify the order for some of the interned
        // strings in order to prevent having to rewrite your string
        // attributes, you can give them to beginData and they will become
        // the first entries in the string table in the given order
        //

        void beginData(const std::string* orderedStrings, size_t num);

        //
        //  data -- these must be called in the same order as the
        //  declaration section above. One of the propertyData..()
        //  functions should be called once for each property declared.
        //  The additional arguments are for sanity checking -- if you
        //  supply them, the writer will check to see that the expected
        //  data matches.
        //

        void propertyDataRaw(const void* data, const char* propertyName = 0,
                             uint32 size = 0,
                             const Dimensions& dims = Dimensions(0, 0, 0, 0));

        void emptyProperty() { propertyDataRaw((void*)0); }

        template <typename T>
        void propertyData(const T* data, const char* propertyName = 0,
                          uint32 size = 0,
                          const Dimensions& dims = Dimensions(0, 0, 0, 0));

        template <typename T>
        void propertyData(const std::vector<T>& data,
                          const char* propertyName = 0, uint32 size = 0,
                          const Dimensions& dims = Dimensions(0, 0, 0, 0));

        template <class T>
        void propertyDataInContainer(
            const T& container, const char* propertyName = 0, uint32 size = 0,
            const Dimensions& dims = Dimensions(0, 0, 0, 0));

        void endData();

        //
        //  Previously declared property data
        //

        const Properties& properties() const { return m_properties; }

    private:
        void init(std::ostream*);
        void constructStringTable(const std::string*, size_t);
        void writeHead();
        void write(const void*, size_t);
        void write(const std::string&);
        void writeFormatted(const char*, ...);
        void writeIndent(size_t n);
        void writeText(const std::string&);
        void writeQuotedString(const std::string&);
        void writeMaybeQuotedString(const std::string&);
        void flush();
        bool propertySanityCheck(const char*, uint32, const Dimensions&);

    private:
        std::ostream* m_out;
        void* m_gzfile;
        Objects m_objects;
        Components m_components;
        Properties m_properties;
        PropertyMap m_propertyMap;
        StringVector m_names;
        StringVector m_componentScope;
        StringMap m_strings;
        std::string m_outName;
        size_t m_currentProperty;
        FileType m_type;
        bool m_needsClosing : 1;
        bool m_error : 1;
        bool m_tableFinished : 1;
        bool m_endDataCalled : 1;
        bool m_beginDataCalled : 1;
        bool m_objectActive : 1;
        bool m_componentActive : 1;
    };

    template <typename T>
    void Writer::propertyData(const T* data, const char* propertyName,
                              uint32 size, const Dimensions& dims)
    {
        propertyDataRaw(data, propertyName, size, dims);
    }

    template <class T>
    void Writer::propertyData(const std::vector<T>& container,
                              const char* propertyName, uint32 size,
                              const Dimensions& dims)
    {
        //
        //  Assumes vector implementation uses contiguous
        //  storage. Currently there are no common implementations for
        //  which this is not true and the upcoming ANSI C++ revision will
        //  make this a requirement of the implementation.
        //

        if (container.empty())
        {
            propertyDataRaw(0, propertyName, 0, dims);
        }
        else
        {
            propertyDataRaw(&container.front(), propertyName, size, dims);
        }
    }

    template <class T>
    void Writer::propertyDataInContainer(const T& container,
                                         const char* propertyName, uint32 size,
                                         const Dimensions& dims)
    {
        typedef typename T::value_type value_type;
        typedef typename T::const_iterator iterator;

        if (container.empty())
        {
            propertyDataRaw(0, propertyName, 0, dims);
        }
        else
        {
            std::vector<value_type> data(container.size());
            std::copy(container.begin(), container.end(), data.begin());
            propertyDataRaw(&data.front(), propertyName, size, dims);
        }
    }

} // namespace Gto

#endif // __Gto__Writer__h__
