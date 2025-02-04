#ifndef __Gto__RawData__h__
#define __Gto__RawData__h__
//
// Copyright (c) 2009, Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Gto/Header.h>
#include <Gto/Reader.h>
#include <Gto/Writer.h>
#include <list>
#include <string>

namespace Gto
{

    //
    //  These classes implement a "raw" database of Gto data. Its mostly
    //  useful for basic gto munging. The data from the gto file is read
    //  into a hierarchy of arrays. This structure is *not* recommended
    //  for use beyond very simple applications. It will also be useful as
    //  an example of how the Reader/Writer are intended to be used.
    //

    //----------------------------------------------------------------------

    struct Property
    {
        //
        //  These are deprecated constructors from v3
        //

        Property(const std::string& n, const std::string& i, Gto::DataType t,
                 size_t s, size_t w, bool allocate = false);

        Property(const std::string& n, Gto::DataType t, size_t s, size_t w,
                 bool allocate = false);

        //
        //  New constructors take all three dimensions
        //

        Property(const std::string& n, const std::string& fn,
                 const std::string& i, Gto::DataType t, size_t s,
                 const Dimensions& d, bool allocate = false);

        Property(const std::string& n, const std::string& fn, Gto::DataType t,
                 size_t s, const Dimensions& d, bool allocate = false);

        ~Property();

        std::string name;
        std::string fullName;
        std::string interp;
        Gto::DataType type;
        size_t size;
        Dimensions dims;

        union
        {
            float* floatData;
            double* doubleData;
            int32* int32Data;
            uint16* uint16Data;
            uint8* uint8Data;
            std::string* stringData;

            void* voidData;
        };
    };

    typedef std::vector<Property*> Properties;

    //----------------------------------------------------------------------

    struct Component;
    typedef std::vector<Component*> Components;

    struct Component
    {
        Component(const std::string& n, const std::string& i, uint16 f)
            : name(n)
            , interp(i)
            , flags(f)
        {
        }

        Component(const std::string& n, uint16 f)
            : name(n)
            , flags(f)
        {
        }

        ~Component();

        std::string name;
        std::string fullName;
        std::string interp;
        uint16 flags;
        Properties properties;
        Components components;
    };

    //----------------------------------------------------------------------

    struct Object
    {
        Object(const std::string& n, const std::string& p, unsigned int v)
            : name(n)
            , protocol(p)
            , protocolVersion(v)
        {
        }

        ~Object();

        std::string name;
        std::string protocol;
        unsigned int protocolVersion;
        Components components;
    };

    typedef std::vector<Object*> Objects;

    //----------------------------------------------------------------------

    typedef std::vector<std::string> Strings;

    struct RawDataBase
    {
        ~RawDataBase();

        Objects objects;
        Strings strings;
    };

    //----------------------------------------------------------------------

    //
    //  class RawDataBaseReader
    //
    //  Call open() with a filename (from the base class) to have it read
    //  the data. This class initializes the Reader base class to not own
    //  the data. This is a bit unusual. Normally you'd copy the data the
    //  reader gives you; but in this case, since its raw anyway we'll
    //  just take the data and keep it.
    //
    //  You need to delete the RawDataBase that's returned from the dataBase()
    //  function. The RawDataBaseReader will not delete it.
    //

    class RawDataBaseReader : public Reader
    {
    public:
        typedef std::vector<Component*> ComponentStack;

        explicit RawDataBaseReader(unsigned int mode = None);
        virtual ~RawDataBaseReader();

        virtual bool open(const char* filename);
        virtual bool open(std::istream&, const char* name);

        RawDataBase* dataBase() { return m_dataBase; }

    protected:
        virtual Request object(const std::string& name,
                               const std::string& protocol,
                               unsigned int protocolVersion,
                               const ObjectInfo& header);

        virtual Request component(const std::string& name,
                                  const std::string& interp,
                                  const ComponentInfo& header);

        virtual Request property(const std::string& name,
                                 const std::string& interp,
                                 const PropertyInfo& header);

        virtual void* data(const PropertyInfo&, size_t bytes);
        virtual void dataRead(const PropertyInfo&);

    protected:
        RawDataBase* m_dataBase;
        ComponentStack m_componentStack;
    };

    //----------------------------------------------------------------------

    //
    //  class RawDataBaseWriter
    //
    //  The uses a Writer class (instead of being one).
    //

    class RawDataBaseWriter
    {
    public:
        RawDataBaseWriter()
            : m_writer()
        {
        }

        bool write(const char* filename, const RawDataBase&,
                   Writer::FileType type = Writer::CompressedGTO);

        bool write(const char* f, const RawDataBase& db, bool c)
        {
            return write(f, db, c ? Writer::CompressedGTO : Writer::BinaryGTO);
        }

        void close() { m_writer.close(); }

    private:
        void writeComponent(bool header, const Component*);
        void writeProperty(bool header, const Property*);

    private:
        Writer m_writer;
    };

} // namespace Gto

#endif // __Gto__RawData__h__
