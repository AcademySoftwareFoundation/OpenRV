//
// Copyright (c) 2009, Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Gto/Reader.h>
#include <Gto/Utilities.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <iterator>
#ifdef GTO_SUPPORT_ZIP
#include <zlib.h>
// zlib might define Byte which clashes with the Gto::Byte enum
#ifdef Byte
#undef Byte
#endif
#endif

#include <TwkUtil/sgcHop.h>

// Unicode filename conversion
#ifdef _MSC_VER
#include <string>
#include <codecvt>
#include <wchar.h>
#endif

int GTOParse(Gto::Reader*);

#if (__GNUC__ == 2)
#define IOS_CUR ios::cur
#define IOS_BEG ios::beg
#else
#define IOS_CUR ios_base::cur
#define IOS_BEG ios_base::beg
#endif

namespace Gto
{
    using namespace std;

    Reader::Reader(unsigned int mode)
        : m_in(0)
        , m_inRAM(0)
        , m_inRAMSize(0)
        , m_inRAMCurrentPos(0)
        , m_gzfile(0)
        , m_gzrval(0)
        , m_needsClosing(false)
        , m_error(false)
        , m_mode(mode)
        , m_linenum(0)
        , m_charnum(0)
    {
    }

    Reader::~Reader() { close(); }

    bool Reader::open(void const* pData, size_t dataSize, const char* name)
    {
        if (m_in)
            return false;
        if (pData == NULL)
            return false;
        if (dataSize <= 0)
            return false;
        if (m_in || m_gzfile)
            close();

        m_inRAM = (char*)pData;
        m_inRAMSize = dataSize;
        m_inRAMCurrentPos = 0;
        m_needsClosing = false;
        m_inName = name;
        m_error = false;

        if (m_mode & TextOnly)
        {
            return readTextGTO();
        }
        else
        {
            readMagicNumber();

            if (m_header.magic != Header::Magic
                && m_header.magic != Header::Cigam)
            {
                return false;
            }

            return readBinaryGTO();
        }
    }

    bool Reader::open(istream& i, const char* name, unsigned int ormode)
    {
        if ((m_in && m_in != &i) || m_gzfile)
            close();

        m_in = &i;
        m_needsClosing = false;
        m_inName = name;
        m_error = false;

        if ((m_mode | ormode) & TextOnly)
        {
            return readTextGTO();
        }
        else
        {
            readMagicNumber();

            if (m_header.magic != Header::Magic
                && m_header.magic != Header::Cigam)
            {
                i.seekg(0, std::ios::beg);
                return readTextGTO();
            }
            else
            {
                return readBinaryGTO();
            }
        }
    }

    bool Reader::open(const char* filename)
    {
        if (m_in)
            return false;

#ifdef _MSC_VER
        wstring_convert<codecvt_utf8<wchar_t>> wstr;
        wstring wstr_filename = wstr.from_bytes(filename);
        const wchar_t* w_filename = wstr_filename.c_str();
#endif

#ifdef _MSC_VER
        struct _stat64 buf;
        if (_wstat64(w_filename, &buf))
#else
        struct stat buf;
        if (stat(filename, &buf))
#endif
        {
            fail("File does not exist");
            return false;
        }

        m_inName = filename;

        //
        //  Fail if not compiled with zlib and the extension is gz
        //

#ifndef GTO_SUPPORT_ZIP
        size_t i = m_inName.find(".gz", 0, 3);
        if (i == (strlen(filename) - 3))
        {
            fail("this library was not compiled with zlib support");
            return false;
        }
#endif

        //
        //  Figure out if this is a text GTO file
        //

#ifdef GTO_SUPPORT_ZIP
#ifdef _MSC_VER
        m_gzfile = gzopen_w(w_filename, "rb");
#else
        m_gzfile = gzopen(filename, "rb");
#endif

        if (!m_gzfile)
        {
            //
            //  Try .gz version before giving up completely
            //

            std::string temp(filename);
            temp += ".gz";
            return open(temp.c_str());
        }

#else
#ifdef _MSC_VER
        m_in = new ifstream(w_filename, ios::in | ios::binary);
#else
        m_in = new ifstream(filename, ios::in | ios::binary);
#endif

        if (!(*m_in))
        {
            m_in = 0;
            fail("stream failed to open");
            return false;
        }
#endif

        m_needsClosing = true;
        m_error = false;

        readMagicNumber();

        if (m_header.magic == Header::MagicText
            || m_header.magic == Header::CigamText)
        {
            close();
#ifdef _MSC_VER
            m_in = new ifstream(w_filename, ios::in | ios::binary);
#else
            m_in = new ifstream(filename, ios::in | ios::binary);
#endif

            if (!(*m_in))
            {
                delete m_in;
                m_in = 0;
                fail("stream failed to open");
                return false;
            }

            //
            //  Note that at this point m_needsClosing is true, but the below
            //  open() will set it to false, so be sure to reset it to true,
            //  since we created this istream and need to close it when we're
            //  done (prob when the Reader object is destroyed).
            //
            bool ret = open(*m_in, filename, TextOnly);
            m_needsClosing = true;
            return ret;
        }
        else
        {
            return readBinaryGTO();
        }
    }

    void Reader::close()
    {
        m_inRAM = 0;
        m_inRAMSize = 0;

        if (m_needsClosing)
        {
            delete m_in;
            m_in = 0;
#ifdef GTO_SUPPORT_ZIP

            if (m_gzfile)
            {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
                gzclose((gzFile_s*)m_gzfile);
#else
                gzclose(m_gzfile);
#endif
                m_gzfile = 0;
            }
#endif
        }

        //
        //  Clean everything up in case the Reader
        //  class is used for another file.
        //

        m_objects.clear();
        m_components.clear();
        m_properties.clear();
        m_strings.clear();
        m_stringMap.clear();
        m_buffer.clear();

        m_error = false;
        m_inName = "";
        m_needsClosing = false;
        m_swapped = false;
        m_why = "";
        m_linenum = 0;
        m_charnum = 0;
        memset(&m_header, 0, sizeof(m_header));
    }

    void Reader::header(const Header&) {}

    Reader::Request Reader::object(const string&, const string&, unsigned int,
                                   const ObjectInfo&)
    {
        return Request(true);
    }

    Reader::Request Reader::component(const string&, const ComponentInfo&)
    {
        return Request(true);
    }

    Reader::Request Reader::property(const string&, const PropertyInfo&)
    {
        return Request(true);
    }

    void* Reader::data(const PropertyInfo&, size_t) { return 0; }

    void Reader::dataRead(const PropertyInfo&) {}

    void Reader::descriptionComplete() {}

    Reader::Request Reader::component(const string& name, const string& interp,
                                      const ComponentInfo& c)
    {
        // make it backwards compatible with version 2
        return component(name, c);
    }

    Reader::Request Reader::property(const string& name, const string& interp,
                                     const PropertyInfo& p)
    {
        // make it backwards compatible with version 2
        return property(name, p);
    }

    static void swapWords(void* data, size_t size)
    {
        struct bytes
        {
            char c[4];
        };

        bytes* ip = reinterpret_cast<bytes*>(data);

        for (size_t i = 0; i < size; i++)
        {
            bytes temp = ip[i];
            ip[i].c[0] = temp.c[3];
            ip[i].c[1] = temp.c[2];
            ip[i].c[2] = temp.c[1];
            ip[i].c[3] = temp.c[0];
        }
    }

    static void swapShorts(void* data, size_t size)
    {
        struct bytes
        {
            char c[2];
        };

        bytes* ip = reinterpret_cast<bytes*>(data);

        for (size_t i = 0; i < size; i++)
        {
            bytes temp = ip[i];
            ip[i].c[0] = temp.c[1];
            ip[i].c[1] = temp.c[0];
        }
    }

    void Reader::readMagicNumber()
    {
        m_header.magic = 0;
        read((char*)&m_header, sizeof(uint32));
    }

    void Reader::readHeader()
    {
        read((char*)&m_header + sizeof(uint32),
             sizeof(Header) - sizeof(uint32));

        if (m_error)
            return;
        m_swapped = false;

        if (m_header.magic == Header::Cigam)
        {
            m_swapped = true;
            swapWords(&m_header, sizeof(m_header) / sizeof(int));
        }
        else if (m_header.magic != Header::Magic)
        {
            ostringstream str;
            str << "bad magic number (" << hex << m_header.magic << ")";
            fail(str.str());
            return;
        }

        if (m_header.version != GTO_VERSION && m_header.version != 3
            && m_header.version != 2)
        {
            fail("version mismatch");
            cerr << "ERROR: Gto::Reader: gto file version == "
                 << m_header.version
                 << ", which is not readable by this version (v" << GTO_VERSION
                 << ")\n";
            return;
        }

        header(m_header);
    }

    int Reader::internString(const std::string& s)
    {
        StringMap::iterator i = m_stringMap.find(s);

        if (i == m_stringMap.end())
        {
            m_strings.push_back(s);
            int index = m_strings.size() - 1;
            m_stringMap[s] = index;
            return index;
        }
        else
        {
            return i->second;
        }
    }

    void Reader::readStringTable()
    {
        for (uint32 i = 0; i < m_header.numStrings; i++)
        {
            string s;
            char c;

            for (get(c); c && notEOF(); get(c))
            {
                s.push_back(c);
            }

            m_strings.push_back(s);
        }
    }

    void Reader::readObjects()
    {
        int coffset = 0;

        for (uint32 i = 0; i < m_header.numObjects; i++)
        {
            ObjectInfo o;

            if (m_header.version == 2)
            {
                read((char*)&o, sizeof(ObjectHeader_v2));
                o.pad = 0;
            }
            else
            {
                read((char*)&o, sizeof(ObjectHeader));
            }

            if (m_error)
                return;

            if (m_swapped)
                swapWords(&o, sizeof(ObjectHeader) / sizeof(int));

            stringFromId(o.name);
            stringFromId(o.protocolName);
            o.coffset = coffset;
            coffset += o.numComponents;

            if (m_error)
            {
                o.requested = false;
                return;
            }
            else if (!(m_mode & RandomAccess))
            {
                Request r =
                    object(stringFromId(o.name), stringFromId(o.protocolName),
                           o.protocolVersion, o);

                o.requested = r.m_want;
                o.objectData = r.m_data;
            }

            m_objects.push_back(o);
        }
    }

    void Reader::readComponents()
    {
        int poffset = 0;

        for (Objects::iterator i = m_objects.begin(); i != m_objects.end(); ++i)
        {
            const ObjectInfo& o = *i;

            for (uint32 q = 0; q < o.numComponents; q++)
            {
                ComponentInfo c;

                if (m_header.version == 2)
                {
                    read((char*)&c, sizeof(ComponentHeader_v2));
                    c.childLevel = 0;
                    c.interpretation = 0;
                }
                else
                {
                    read((char*)&c, sizeof(ComponentHeader));
                }

                if (m_error)
                    return;

                if (m_swapped)
                    swapWords(&c, sizeof(ComponentHeader) / sizeof(int));

                c.object = &o;
                c.poffset = poffset;
                c.parent = NULL;
                poffset += c.numProperties;

                for (size_t ioffset = 1; c.parent == 0 && ioffset <= q;
                     ioffset++)
                {
                    const size_t index = m_components.size() - ioffset;

                    if (m_components[index].childLevel < c.childLevel)
                    {
                        c.parent = &m_components[index];
                        break;
                    }
                }

                if (c.parent)
                {
                    ostringstream str;
                    str << c.parent->fullName << "." << stringFromId(c.name);
                    c.fullName = str.str();
                }
                else
                {
                    c.fullName = stringFromId(c.name);
                }

                if (o.requested && !(m_mode & RandomAccess))
                {
                    stringFromId(c.name); // sanity checks
                    stringFromId(c.interpretation);
                    if (m_error)
                        return;

                    Request r = component(stringFromId(c.name),
                                          stringFromId(c.interpretation), c);

                    c.requested = r.m_want;
                    c.componentData = r.m_data;
                }
                else
                {
                    c.requested = false;
                }

                m_components.push_back(c);
            }
        }
    }

    void Reader::readProperties()
    {
        for (Components::iterator i = m_components.begin();
             i != m_components.end(); ++i)
        {
            const ComponentInfo& c = *i;

            for (uint32 q = 0; q < c.numProperties; q++)
            {
                PropertyInfo p;

                if (m_header.version == 2)
                {
                    PropertyHeader_v2 pv2;
                    read((char*)&pv2, sizeof(PropertyHeader_v2));
                    if (m_swapped)
                        swapWords(&pv2,
                                  sizeof(PropertyHeader_v2) / sizeof(uint32));

                    p.name = pv2.name;
                    p.size = pv2.size;
                    p.type = pv2.type;
                    p.dims.x = pv2.width;
                    p.dims.y = 0;
                    p.dims.z = 0;
                    p.dims.w = 0;
                    p.interpretation = 0;
                }
                else if (m_header.version == 3)
                {
                    PropertyHeader_v3 pv3;
                    read((char*)&pv3, sizeof(PropertyHeader_v3));
                    if (m_swapped)
                        swapWords(&pv3,
                                  sizeof(PropertyHeader_v3) / sizeof(uint32));

                    p.name = pv3.name;
                    p.size = pv3.size;
                    p.type = pv3.type;
                    p.dims.x = pv3.width;
                    p.dims.y = 0;
                    p.dims.z = 0;
                    p.dims.w = 0;
                    p.interpretation = pv3.interpretation;
                }
                else
                {
                    read((char*)&p, sizeof(PropertyHeader));
                    if (m_swapped)
                        swapWords(&p, sizeof(PropertyHeader) / sizeof(uint32));
                }

                if (m_error)
                    return;

                p.component = &c;
                p.fullName = c.fullName;
                p.fullName += ".";
                p.fullName += stringFromId(p.name);

                if (c.requested && !(m_mode & RandomAccess))
                {
                    stringFromId(p.name);
                    stringFromId(p.interpretation);
                    if (m_error)
                        return;

                    Request r = property(stringFromId(p.name),
                                         stringFromId(p.interpretation), p);

                    p.requested = r.m_want;
                    p.propertyData = r.m_data;
                }
                else
                {
                    p.requested = false;
                }

                m_properties.push_back(p);
            }
        }
    }

    bool Reader::accessProperty(PropertyInfo& p)
    {
        Request r =
            property(stringFromId(p.name), stringFromId(p.interpretation), p);

        p.requested = r.m_want;
        p.propertyData = r.m_data;

        if (p.requested)
        {
            seekTo(p.offset);
            readProperty(p);
        }

        return true;
    }

    bool Reader::accessComponent(ComponentInfo& c)
    {
        const std::string& nme = stringFromId(c.name);
        Request r = component(nme, c);
        c.requested = r.m_want;
        c.componentData = r.m_data;

        if (c.requested)
        {
            for (uint32 j = 0; j < c.numProperties; j++)
            {
                PropertyInfo& p = m_properties[c.poffset + j];
                bool success = accessProperty(p);
                if (!success)
                    return false;
            }
        }

        return true;
    }

    bool Reader::accessObject(ObjectInfo& o)
    {
        Request r = object(stringFromId(o.name), stringFromId(o.protocolName),
                           o.protocolVersion, o);

        o.requested = r.m_want;
        o.objectData = r.m_data;

        if (o.requested)
        {
            for (uint32 q = 0; q < o.numComponents; q++)
            {
                assert((o.coffset + q) < m_components.size());

                ComponentInfo& c = m_components[o.coffset + q];
                bool success = accessComponent(c);
                if (!success)
                    return false;
            }
        }

        return true;
    }

    bool Reader::readTextGTO()
    {
        HOP_PROF_FUNC();
        m_header.magic = Header::MagicText;

        if (!::GTOParse(this))
        {
            fail("failed to parse text GTO");
            return false;
        }

        header(m_header);
        descriptionComplete();
        return true;
    }

    bool Reader::readBinaryGTO()
    {
        readHeader();
        if (m_error)
            return false;
        readStringTable();
        if (m_error)
            return false;
        readObjects();
        if (m_error)
            return false;
        readComponents();
        if (m_error)
            return false;
        readProperties();
        if (m_error)
            return false;
        descriptionComplete();

        if (m_mode & HeaderOnly)
        {
            return true;
        }

        Properties::iterator p = m_properties.begin();

        for (Components::iterator i = m_components.begin();
             i != m_components.end(); ++i)
        {
            ComponentInfo& comp = *i;

            if (comp.flags & Gto::Transposed)
            {
                cerr << "ERROR: Transposed data for '"
                     << stringFromId(comp.object->name) << "."
                     << stringFromId(comp.name) << "' is currently unsupported."
                     << endl;
                abort();
            }
            else
            {
                for (Properties::iterator e = p + comp.numProperties; p != e;
                     ++p)
                {
                    if (!readProperty(*p))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool Reader::readProperty(PropertyInfo& prop)
    {
        size_t num = prop.size * elementSize(prop.dims);
        size_t bytes = dataSizeInBytes(prop.type) * num;
        char* buffer = 0;

        //
        //  Cache the offset pointer
        //

        prop.offset = tell();
        bool readok = false;

        if (prop.requested)
        {
            if ((buffer = (char*)data(prop, bytes)))
            {
                read(buffer, bytes);
                if (!m_error)
                    readok = true;
            }
            else
            {
                seekForward(bytes);
            }
        }
        else
        {
            seekForward(bytes);
        }

        if (m_error)
            return false;

        if (readok)
        {
            if (m_swapped)
            {
                switch (prop.type)
                {
                case Gto::Int:
                case Gto::String:
                case Gto::Float:
                    swapWords(buffer, num);
                    break;

                case Gto::Short:
                case Gto::Half:
                    swapShorts(buffer, num);
                    break;

                case Gto::Double:
                    swapWords(buffer, num * 2);
                    break;

                case Gto::Byte:
                case Gto::Boolean:
                    break;
                }
            }

            dataRead(prop);
        }

        return true;
    }

    bool Reader::notEOF()
    {
        if (m_inRAM)
        {
            return (m_inRAMCurrentPos < m_inRAMSize);
        }
        else if (m_in)
        {
            return (!m_in->fail());
        }
#ifdef GTO_SUPPORT_ZIP
        else if (m_gzfile)
        {
            return m_gzrval != -1;
        }
#endif

        return false;
    }

    void Reader::read(char* buffer, size_t size)
    {
        if (m_inRAM)
        {
            bool past_eof = false;

            if (m_inRAMCurrentPos + size > m_inRAMSize)
            {
                size = m_inRAMSize - m_inRAMCurrentPos;
                past_eof = true;
            }

            for (size_t i = 0; i < size; i++)
            {
                buffer[i] = m_inRAM[m_inRAMCurrentPos];
                m_inRAMCurrentPos++;
            }

            if (past_eof)
            {
                fail("in memory read fail - too many bytes requested");
            }
        }
        else if (m_in)
        {
            m_in->read(buffer, size);

#ifdef GTO_HAVE_FULL_IOSTREAMS
            if (m_in->fail())
            {
                std::cerr << "ERROR: Gto::Reader: Failed to read gto file: '";
                std::cerr << m_inName << "': " << std::endl;
                memset(buffer, 0, size);
                fail("stream fail");
            }
#endif
        }

#ifdef GTO_SUPPORT_ZIP
        else if (m_gzfile)
        {
            char* buffer_pos = buffer;
            size_t remaining = size;
            // while (true)
            while (remaining != 0)
            {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
                int retval = gzread((gzFile_s*)m_gzfile, buffer_pos, remaining);
#else
                int retval = gzread(m_gzfile, buffer_pos, remaining);
#endif
                if (retval <= 0)
                {
                    int zError = 0;
                    std::cerr
                        << "ERROR: Gto::Reader: Failed to read gto file: ";
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
                    std::cerr << gzerror((gzFile_s*)m_gzfile, &zError);
#else
                    std::cerr << gzerror(m_gzfile, &zError);
#endif
                    std::cerr << std::endl;
                    memset(buffer, 0, size);
                    fail("gzread fail");
                    break;
                }
                remaining -= retval;
                buffer_pos += retval;
            }
        }
#endif
    }

    void Reader::get(char& c)
    {
        if (m_inRAM)
        {
            if (m_inRAMSize > m_inRAMCurrentPos)
            {
                c = m_inRAM[m_inRAMCurrentPos];
                m_inRAMCurrentPos++;
            }
            else
            {
                c = 0;
            }
        }
        else if (m_in)
        {
            m_in->get(c);
        }
#ifdef GTO_SUPPORT_ZIP
        else if (m_gzfile)
        {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
            m_gzrval = gzgetc((gzFile_s*)m_gzfile);
#else
            m_gzrval = gzgetc(m_gzfile);
#endif
            c = char(m_gzrval);
        }
#endif
    }

    void Reader::fail(std::string why)
    {
        m_error = true;
        m_why = why;
    }

    const std::string& Reader::stringFromId(unsigned int i)
    {
        static std::string empty("");
        if (i >= m_strings.size())
        {
            std::cerr << "WARNING: Gto::Reader: Malformed gto file: ";
            std::cerr << "invalid string index" << std::endl;
            fail("malformed file, invalid string index");
            return empty;
        }

        return m_strings[i];
    }

    unsigned int Reader::idFromString(const std::string& s)
    {
        if (m_stringMap.count(s) > 1)
        {
            return m_stringMap[s];
        }
        else
        {
            std::cerr << "WARNING: Gto::Reader: Malformed gto file: ";
            std::cerr << "invalid string \"" << s << "\"" << std::endl;
            fail("malformed file, invalid string");
            return -1;
        }
    }

    void Reader::seekForward(size_t bytes)
    {
        if (m_inRAM)
        {
            m_inRAMCurrentPos += bytes;
            if (m_inRAMCurrentPos > m_inRAMSize)
            {
                m_inRAMCurrentPos = m_inRAMSize;
            }
        }
        else if (m_in)
        {
#ifdef GTO_HAVE_FULL_IOSTREAMS
            m_in->seekg(bytes, ios_base::cur);
#else
            m_in->seekg(bytes, ios::cur);
#endif
        }
#ifdef GTO_SUPPORT_ZIP
        else
        {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
            gzseek((gzFile_s*)m_gzfile, bytes, SEEK_CUR);
#else
            gzseek(m_gzfile, bytes, SEEK_CUR);
#endif
        }
#endif
    }

    void Reader::seekTo(size_t bytes)
    {
        if (m_inRAM)
        {
            if (bytes > m_inRAMSize)
            {
                bytes = m_inRAMSize;
            }
            m_inRAMCurrentPos = bytes;
        }
        else if (m_in)
        {
#ifdef GTO_HAVE_FULL_IOSTREAMS
            m_in->seekg(bytes, ios_base::beg);
#else
            m_in->seekg(bytes, ios::beg);
#endif
        }
#ifdef GTO_SUPPORT_ZIP
        else
        {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
            gzseek((gzFile_s*)m_gzfile, bytes, SEEK_SET);
#else
            gzseek(m_gzfile, bytes, SEEK_SET);
#endif
        }
#endif
    }

    int Reader::tell()
    {
        if (m_inRAM)
        {
            return m_inRAMCurrentPos;
        }
        else if (m_in)
        {
            return m_in->tellg();
        }
#ifdef GTO_SUPPORT_ZIP
        else
        {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
            return gztell((gzFile_s*)m_gzfile);
#else
            return gztell(m_gzfile);
#endif
        }
#else
        else
        {
            fail("m_in undefined");
            return 0;
        }
#endif
    }

    //----------------------------------------------------------------------
    //
    //  Text parser entry points
    //

    void Reader::beginHeader(uint32 version)
    {
        m_header.numStrings = 0;
        m_header.numObjects = 0;
        m_header.version = version ? version : GTO_VERSION;
        m_header.flags = 1;
    }

    void Reader::addObject(const ObjectInfo& info)
    {
        //
        //  This function is requireed when the header info is read
        //  out-of-order (as with a text GTO file). In order to be backwards
        //  compatible with the info structures, we need to update pointers
        //  when STL allocates new memory.
        //

        if ((m_objects.size() > 0)
            && (m_objects.capacity() <= m_objects.size()))
        {
            const ObjectInfo* startAddress = &m_objects.front();
            m_objects.push_back(info);
            const ObjectInfo* newStartAddress = &m_objects.front();

            for (Components::iterator i = m_components.begin();
                 i != m_components.end(); ++i)
            {
                size_t offset = i->object - startAddress;
                i->object = newStartAddress + offset;
            }
        }
        else
        {
            m_objects.push_back(info);
        }
    }

    void Reader::addComponent(const ComponentInfo& info)
    {
        //
        //  See addObject coments
        //

        if ((m_components.size() > 0)
            && (m_components.capacity() <= m_components.size()))
        {
            const ComponentInfo* startAddress = &m_components.front();
            m_components.push_back(info);
            const ComponentInfo* newStartAddress = &m_components.front();

            for (Properties::iterator i = m_properties.begin();
                 i != m_properties.end(); ++i)
            {
                size_t offset = i->component - startAddress;
                i->component = newStartAddress + offset;
            }

            for (Components::iterator i = m_components.begin();
                 i != m_components.end(); ++i)
            {
                size_t offset = i->parent - startAddress;
                i->parent = newStartAddress + offset;
            }
        }
        else
        {
            m_components.push_back(info);
        }
    }

    void Reader::beginObject(unsigned int name, unsigned int proto,
                             unsigned int version)
    {
        ObjectInfo info;
        info.name = name;
        info.protocolName = proto;
        info.protocolVersion = version;
        info.numComponents = 0;
        info.pad = 0;
        info.coffset = 0;

        Request r =
            object(stringFromId(name), stringFromId(proto), version, info);

        info.requested = r.want();
        info.objectData = r.data();

        addObject(info);
    }

    void Reader::beginComponent(unsigned int nameID, unsigned int interpID)
    {

        ostringstream fullname;
        string name = stringFromId(nameID);

        for (int i = 0; i < m_nameStack.size(); i++)
        {
            fullname << m_nameStack[i] << ".";
        }

        fullname << name;

        ComponentInfo info;
        info.name = nameID;
        info.numProperties = 0;
        info.flags = 0;
        info.interpretation = interpID;
        info.childLevel = 0;
        info.poffset = 0;
        info.object = &m_objects.back();
        info.childLevel = m_nameStack.size();
        info.fullName = fullname.str();

        m_nameStack.push_back(name);
        m_indexStack.push_back(m_objects.back().numComponents);
        m_objects.back().numComponents++;

        if (info.object->requested)
        {
            Request r =
                component(stringFromId(nameID), stringFromId(interpID), info);

            info.requested = r.want();
            info.componentData = r.data();
        }
        else
        {
            info.requested = false;
            info.componentData = 0;
        }

        addComponent(info);
    }

    void Reader::endComponent()
    {
        m_nameStack.pop_back();
        m_indexStack.pop_back();
    }

    void Reader::beginProperty(unsigned int name, unsigned int interp,
                               unsigned int size, DataType type,
                               const Dimensions& dims)
    {
        PropertyInfo info;
        info.name = name;
        info.interpretation = interp;
        info.size = 0;
        info.type = type;
        info.dims = dims;
        info.component = &m_components.back();
        info.fullName = m_components.back().fullName;
        info.fullName += ".";
        info.fullName += stringFromId(name);

        m_components.back().numProperties++;

        m_buffer.clear();

        m_currentType.type = type;
        m_currentType.size = size;
        m_currentType.dims = makeDimStruct(dims);

        if (info.component->requested)
        {
            Request r =
                property(stringFromId(name), stringFromId(interp), info);

            info.requested = r.want();
            info.propertyData = r.data();
        }
        else
        {
            info.requested = false;
            info.propertyData = 0;
        }

        m_properties.push_back(info);
    }

    size_t Reader::numAtomicValuesInBuffer() const
    {
        return m_buffer.size() / dataSizeInBytes(m_currentType.type);
    }

    size_t Reader::numElementsInBuffer() const
    {
        return numAtomicValuesInBuffer() / elementSize(m_currentType);
    }

    void Reader::fillToSize(size_t size)
    {
        size_t esize =
            dataSizeInBytes(m_currentType.type) * elementSize(m_currentType);

        ByteArray element(esize);

        memcpy(&element[0], &m_buffer[m_buffer.size() - esize], esize);

        while (numElementsInBuffer() != size)
        {
            copy(element.begin(), element.end(), back_inserter(m_buffer));
        }
    }

    void Reader::parseError(const char* msg)
    {
        cerr << "ERROR: parsing GTO file \"" << infileName() << "\" at line "
             << linenum() << ", char " << charnum() << " : " << msg << endl;
    }

    void Reader::parseWarning(const char* msg)
    {
        cerr << "WARNING: parsing GTO file \"" << infileName() << "\" at line "
             << linenum() << ", char " << charnum() << " : " << msg << endl;
    }

    void Reader::endProperty()
    {
        //
        //  Give the data to the reader sub-class
        //

        PropertyInfo& info = m_properties.back();
        info.size = numElementsInBuffer();

        if (info.requested)
        {
            if (void* buffer = data(info, m_buffer.size()))
            {
                memcpy(buffer, &m_buffer.front(), m_buffer.size());
                dataRead(info);
            }
        }

        m_buffer.clear();
    }

    void Reader::endFile() { m_header.numStrings = m_strings.size(); }

} // namespace Gto
