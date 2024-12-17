//
// Copyright (c) 2009, Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <Gto/Writer.h>
#include <Gto/Utilities.h>
#include <fstream>
#include <ctype.h>
#include <stdio.h>
#include <stdexcept>
#include <sstream>
#include <algorithm>

// Unicode filename conversion
#ifdef _MSC_VER
#include <string>
#include <codecvt>
#include <wchar.h>
#endif

#define GTO_DEBUG 0

#ifdef GTO_SUPPORT_ZIP
#include <zlib.h>
// zlib might define Byte which clashes with the Gto::Byte enum
#ifdef Byte
#undef Byte
#endif
#endif

#ifdef WIN32
#define snprintf _snprintf
#endif

namespace Gto
{
    using namespace std;

    Writer::Writer()
        : m_out(0)
        , m_gzfile(0)
        , m_needsClosing(false)
        , m_error(false)
        , m_tableFinished(false)
        , m_currentProperty(0)
        , m_type(CompressedGTO)
        , m_endDataCalled(false)
        , m_beginDataCalled(false)
        , m_objectActive(false)
        , m_componentActive(false)
    {
        init(0);
    }

    Writer::Writer(ostream& o)
        : m_out(0)
        , m_gzfile(0)
        , m_needsClosing(false)
        , m_error(false)
        , m_tableFinished(false)
        , m_currentProperty(0)
        , m_type(CompressedGTO)
        , m_endDataCalled(false)
        , m_beginDataCalled(false)
        , m_objectActive(false)
        , m_componentActive(false)
    {
        init(&o);
    }

    Writer::~Writer() { close(); }

    void Writer::init(ostream* o) { m_out = o; }

    bool Writer::open(ostream& out, FileType type)
    {
        init(&out);
        return open("", type);
    }

    bool Writer::open(const char* filename, FileType type)
    {
        m_outName = filename;
        m_type = type;
        m_needsClosing = false;

        if (m_outName != "" && (m_out || m_gzfile))
            return false;

#ifdef _MSC_VER
        wstring_convert<codecvt_utf8<wchar_t>> wstr;
        wstring wstr_filename = wstr.from_bytes(filename);
        const wchar_t* w_filename = wstr_filename.c_str();
#endif

#ifndef GTO_SUPPORT_ZIP
        if (type == CompressedGTO)
            type = BinaryGTO;
#endif

        if (!m_out && (type == BinaryGTO || type == TextGTO))
        {
            if (type == BinaryGTO)
            {
#ifdef _MSC_VER
                m_out = new ofstream(w_filename, ios::out | ios::binary);
#else
                m_out = new ofstream(filename, ios::out | ios::binary);
#endif
            }
            else
            {
#ifdef _MSC_VER
                m_out = new ofstream(w_filename, ios::out);
#else
                m_out = new ofstream(filename, ios::out);
#endif
            }

            m_needsClosing = true;

            if (!(*m_out))
            {
                m_out = 0;
                m_error = true;
                return false;
            }
        }
#ifdef GTO_SUPPORT_ZIP
        else if (type == CompressedGTO)
        {
#ifdef _MSC_VER
            m_gzfile = gzopen_w(w_filename, "wb");
#else
            m_gzfile = gzopen(filename, "wb");
#endif
            m_needsClosing = true;

            if (!m_gzfile)
            {
                m_gzfile = 0;
                m_error = true;
                return false;
            }
        }
#endif

        m_error = false;
        return true;
    }

    void Writer::close()
    {
        if (m_beginDataCalled && !m_endDataCalled)
        {
            //
            //  Should this be an error? We can gracefully finish like
            //  this, but its not how you're supposed to use the class.
            //

            cout << "WARNING: Gto::Writer::close() -- you forgot to call "
                    "endData()"
                 << endl;

            endData();
        }

        if (m_out && m_needsClosing)
        {
            delete m_out;
        }
#ifdef GTO_SUPPORT_ZIP
        else if (m_gzfile && m_needsClosing)
        {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
            gzclose((gzFile_s*)m_gzfile);
#else
            gzclose(m_gzfile);
#endif
        }

        m_gzfile = 0;
#endif
        m_out = 0;
    }

    void Writer::beginData() { beginData(0, 0); }

    void Writer::beginData(const std::string* orderedStrings, size_t num)
    {
        m_currentProperty = 0;
        constructStringTable(orderedStrings, num);

        if (m_type == TextGTO)
        {
            writeFormatted("GTOa (%d)\n\n", GTO_VERSION);
        }
        else
        {
            writeHead();
        }

        m_beginDataCalled = true;
    }

    void Writer::endData()
    {
        if (m_type == TextGTO)
        {
            PropertyPath p0 = m_propertyMap[m_currentProperty - 1];

            //
            //  Close off existing scope
            //

            size_t s = p0.componentScope.size();

            for (int i = 0; i < s; i++)
            {
                writeIndent((s - i) * 4);
                writeFormatted("}\n");
            }

            writeFormatted("}\n");
        }

        m_endDataCalled = true;
    }

    void Writer::intern(const char* s)
    {
        if (m_tableFinished)
        {
            throw std::runtime_error("Gto::Writer::intern(): Unable to intern "
                                     "strings after string table is finished");
        }

        m_strings[s] = -1;
    }

    void Writer::intern(const string& s)
    {
        if (m_tableFinished)
        {
            throw std::runtime_error("Gto::Writer::intern(): Unable to intern "
                                     "strings after string table is finished");
        }

        m_strings[s] = -1;
    }

    uint32 Writer::lookup(const char* s) const
    {
        if (m_tableFinished)
        {
            StringMap::const_iterator i = m_strings.find(s);

            if (i == m_strings.end())
            {
                return uint32(-1);
            }
            else
            {
                return i->second;
            }
        }
        else
        {
            return -1;
        }
    }

    uint32 Writer::lookup(const string& s) const
    {
        if (m_tableFinished)
        {
            StringMap::const_iterator i = m_strings.find(s);

            if (i == m_strings.end())
            {
                return uint32(-1);
            }
            else
            {
                return i->second;
            }
        }
        else
        {
            return -1;
        }
    }

    std::string Writer::lookup(uint32 n) const
    {
        if ((size_t)n >= m_names.size())
            return "*bad-lookup*";
        return m_names[n];
    }

    void Writer::beginObject(const char* name, const char* protocol,
                             uint32 version)
    {
        if (m_objectActive)
        {
            throw std::runtime_error("ERROR: Gto::Writer::beginObject() -- "
                                     "you forgot to call endObject()");
        }

        if (m_componentActive)
        {
            throw std::runtime_error("ERROR: Gto::Writer::beginObject() -- "
                                     "beginComponent() still active");
        }

        m_names.push_back(name);
        m_names.push_back(protocol);

        ObjectHeader header;
        memset(&header, 0, sizeof(ObjectHeader));
        header.numComponents = 0;
        header.name = m_names.size() - 2;
        header.protocolName = m_names.size() - 1;
        header.protocolVersion = version;

        m_objects.push_back(header);
        m_objectActive = true;
    }

    void Writer::beginComponent(const char* name, uint32 flags)
    {
        beginComponent(name, NULL, flags);
    }

    void Writer::beginComponent(const char* name, const char* interp,
                                uint32 flags)
    {
        if (!m_objectActive)
        {
            throw std::runtime_error("ERROR: Gto::Writer::beginComponent() -- "
                                     "you forgot to call beginObject()");
        }

        string sname = name;
        const bool fullyQualified = sname.find('.') != string::npos;

        if (fullyQualified)
        {
            throw std::runtime_error("ERROR: Gto::Writer::beginComponent() -- "
                                     "illegal character '.' in component name");
        }
        else
        {
            m_componentScope.push_back(sname);

            ostringstream str;

            str << name;

            m_names.push_back(str.str()); // full name
            m_objects.back().numComponents++;
            ComponentHeader header;
            memset(&header, 0, sizeof(ComponentHeader));

            header.numProperties = 0;
            header.flags = flags;
            header.name = m_names.size() - 1;
            header.childLevel = m_componentScope.size() - 1;

            m_names.push_back(interp ? interp : "");
            header.interpretation = m_names.size() - 1;

            m_components.push_back(header);
            m_componentActive = true;
        }
    }

    void Writer::endComponent()
    {
        m_componentActive = false;
        m_componentScope.pop_back();
    }

    void Writer::endObject() { m_objectActive = false; }

    void Writer::property(const char* name, DataType type, size_t numElements,
                          const Dimensions& dims, const char* interp)
    {
        if (!m_objectActive || !m_componentActive)
        {
            throw std::runtime_error("ERROR: Gto::Writer::property() -- "
                                     "no active component or object");
        }

        m_names.push_back(name);
        m_components.back().numProperties++;

        PropertyHeader header;
        memset(&header, 0, sizeof(PropertyHeader));
        header.size = numElements;
        header.type = type;
        header.name = m_names.size() - 1;
        header.dims = dims;

        if (!interp)
            interp = "";
        m_names.push_back(interp);
        header.interpretation = m_names.size() - 1;

        m_properties.push_back(header);

        if (m_type == TextGTO)
        {
            PropertyPath ppath(m_objects.size() - 1, name, m_componentScope);
            m_propertyMap[m_properties.size() - 1] = ppath;
        }
    }

    void Writer::constructStringTable(const std::string* orderedStrings,
                                      size_t num)
    {
        if (num > 0 && orderedStrings == 0)
        {
            throw std::runtime_error("Gto::Writer::constructStringTable(): "
                                     "ordered string list length non-zero "
                                     "but string list was null");
        }

        for (size_t i = 0; i < m_names.size(); i++)
        {
            intern(m_names[i]);
        }

        //
        //  Assign numbers -- note, this is a workaround for the fact that
        //  set does not implement operator-() for its
        //  iterators. Otherwise, we could use a set instead of a map.
        //

        int count = 0;
        size_t bytes = 0;

        //
        // populate the first entries with the given ordered strings
        //

        for (size_t i = 0; i < num; ++i)
        {
            intern(orderedStrings[i]);
        }

        for (; count < num; ++count)
        {
            const std::string& s = orderedStrings[count];
            if (m_strings[s] != -1)
            {
                throw std::runtime_error("Gto::Writer::constructStringTable(): "
                                         "duplicate string present in ordered "
                                         "string list");
            }
            m_strings[s] = count;
        }

        for (StringMap::iterator i = m_strings.begin(); i != m_strings.end();
             ++i)
        {
            if (i->second == -1)
                i->second = count++;
            bytes += (i->first.size() + 1) * sizeof(char);
        }

        //
        //  Find all the name ids for the header structs
        //

        for (size_t o = 0, c = 0, p = 0, n = 0; o < m_objects.size(); o++)
        {
            ObjectHeader& oh = m_objects[o];
            oh.name = m_strings[m_names[n++]];
            oh.protocolName = m_strings[m_names[n++]];

            for (size_t i = 0; i < oh.numComponents; i++, c++)
            {
                ComponentHeader& ch = m_components[c];
                ch.name = m_strings[m_names[n++]];
                ch.interpretation = m_strings[m_names[n++]];

                for (size_t q = 0; q < ch.numProperties; q++, p++)
                {
                    PropertyHeader& ph = m_properties[p];
                    ph.name = m_strings[m_names[n++]];
                    ph.interpretation = m_strings[m_names[n++]];
                }
            }
        }

        //
        //  Reformat the m_names to do inverse lookups for debugging
        //

        m_names.resize(m_strings.size());

        for (StringMap::iterator i = m_strings.begin(); i != m_strings.end();
             ++i)
        {
            m_names[i->second] = i->first;
        }

        m_tableFinished = true;
    }

    static bool gto_isalnum(const string& str)
    {
        bool allnumbers = true;

        for (size_t i = 0, s = str.size(); i < s; i++)
        {
            int c = str[i];

            if (!isalnum(c) && c != '_')
            {
                return false;
            }

            if (!isdigit(c))
                allnumbers = false;
        }

        return !allnumbers;
    }

    void Writer::writeMaybeQuotedString(const string& str)
    {
        static const char* keywords[] = {"float", "double", "half", "bool",
                                         "int",   "short",  "byte", "as",
                                         "GTOa",  "string", 0};

        for (const char** k = keywords; *k; k++)
        {
            if (str == *k)
            {
                writeQuotedString(str);
                return;
            }
        }

        if (gto_isalnum(str))
        {
            writeText(str);
        }
        else
        {
            writeQuotedString(str);
        }
    }

    void Writer::writeQuotedString(const string& str)
    {
        writeFormatted("\"");
        static const char quote = '"';
        static const char slash = '\\';
        char lastc = 0;

        for (size_t i = 0; i < str.size(); i++)
        {
            char c = str[i];

            if (c == 0)
            {
                write("");
            }
            else if (lastc & 0x80)
            {
                write(&c, sizeof(char));
            }
            else if (iscntrl(c))
            {
                switch (c)
                {
                case '\n':
                    writeFormatted("\\n");
                    break;
                case '\b':
                    writeFormatted("\\b");
                    break;
                case '\r':
                    writeFormatted("\\r");
                    break;
                case '\t':
                    writeFormatted("\\t");
                    break;
                default:
                {
                    char temp[41];
                    temp[40] = 0;
                    snprintf(temp, 40, "\\%o", int(c));
                    writeFormatted(temp);
                }
                break;
                }
            }
            else if (c == quote)
            {
                writeFormatted("\\%c", quote);
            }
            else if (c == slash)
            {
                writeFormatted("\\%c", slash);
            }
            else if (c & 0x80)
            {
                // UTF-8
                write(&c, sizeof(char));
            }
            else
            {
                write(&c, sizeof(char));
            }

            lastc = c;
        }

        writeFormatted("\"");
    }

    void Writer::writeText(const std::string& s)
    {
        if (m_out)
        {
            (*m_out) << s;
        }
#ifdef GTO_SUPPORT_ZIP
        else if (m_gzfile)
        {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
            gzwrite((gzFile_s*)m_gzfile, (void*)s.c_str(), s.size());
#else
            gzwrite(m_gzfile, (void*)s.c_str(), s.size());
#endif
        }
#endif
    }

    void Writer::writeIndent(size_t n)
    {
        ostringstream str;
        for (size_t i = 0; i < n; i++)
            str << " ";
        string s = str.str();
        write(s.c_str(), s.size());
    }

    void Writer::writeFormatted(const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        char* m = new char[1024 * 10];
        // vasprintf(&m, format, ap);
        vsprintf(m, format, ap);
        write(m, strlen(m));
        delete[] m;
    }

    void Writer::write(const void* p, size_t s)
    {
        if (s == 0)
            return;
        if (m_out)
        {
            m_out->write((const char*)p, s);
        }
#ifdef GTO_SUPPORT_ZIP
        else if (m_gzfile)
        {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
            gzwrite((gzFile_s*)m_gzfile, (void*)p, s);
#else
            gzwrite(m_gzfile, (void*)p, s);
#endif
        }
#endif
    }

    void Writer::write(const std::string& s)
    {
        if (m_out)
        {
            (*m_out) << s;
            m_out->put(0);
        }
#ifdef GTO_SUPPORT_ZIP
        else if (m_gzfile)
        {
#if ZLIB_VERNUM >= UPDATED_ZLIB_VERNUM
            gzwrite((gzFile_s*)m_gzfile, (void*)s.c_str(), s.size() + 1);
#else
            gzwrite(m_gzfile, (void*)s.c_str(), s.size() + 1);
#endif
        }
#endif
    }

    void Writer::flush()
    {
        if (m_out)
            (*m_out) << std::flush;
    }

    void Writer::writeHead()
    {
        Header header;
        header.magic = GTO_MAGIC;
        header.numObjects = m_objects.size();
        header.numStrings = m_strings.size();
        header.version = GTO_VERSION;
        header.flags = 0;

        write(&header, sizeof(Header));

        for (StringVector::iterator i = m_names.begin(); i != m_names.end();
             ++i)
        {
            write(*i);
        }

        for (size_t i = 0; i < m_objects.size(); i++)
        {
            ObjectHeader& o = m_objects[i];
            write(&o, sizeof(ObjectHeader));
        }

        for (size_t i = 0; i < m_components.size(); i++)
        {
            ComponentHeader& c = m_components[i];
            write(&c, sizeof(ComponentHeader));
        }

        for (size_t i = 0; i < m_properties.size(); i++)
        {
            PropertyHeader& p = m_properties[i];
            write(&p, sizeof(PropertyHeader));
        }

        flush();
    }

    bool Writer::propertySanityCheck(const char* propertyName, uint32 size,
                                     const Dimensions& dims)
    {
        if (!propertyName)
            return true;
        size_t p = m_currentProperty - 1;

        if (propertyName != NULL
            && propertyName != m_names[m_properties[p].name])
        {
            std::cerr << "ERROR: Gto::Writer: propertyData expected '"
                      << m_names[m_properties[p].name] << "' but got data for '"
                      << propertyName << "' instead." << std::endl;
            m_error = true;
            return false;
        }

        if (size > 0 && size != m_properties[p].size)
        {
            std::cerr
                << "ERROR: Gto::Writer: propertyData expected data of size "
                << m_properties[p].size << " but got data of size " << size
                << " instead while writing property '"
                << m_names[m_properties[p].name] << "'" << std::endl;
            m_error = true;
            return false;
        }

        if (dims.x > 0 && dims.x != m_properties[p].dims.x
            && dims.y != m_properties[p].dims.y
            && dims.z != m_properties[p].dims.z
            && dims.w != m_properties[p].dims.w)
        {
            std::cerr << "ERROR: Gto::Writer: propertyData expected data of "
                         "dimension "
                      << m_properties[p].dims.x << "x" << m_properties[p].dims.y
                      << "x" << m_properties[p].dims.z << "x"
                      << m_properties[p].dims.w << " but got data of dimension "
                      << dims.x << "x" << dims.y << "x" << dims.z << "x"
                      << dims.w << " instead while writing property '"
                      << m_names[m_properties[p].name] << "'" << std::endl;
            m_error = true;
            return false;
        }

        return true;
    }

    void Writer::propertyDataRaw(const void* data, const char* propertyName,
                                 uint32 size, const Dimensions& dims)
    {
        if (!m_beginDataCalled)
            beginData();

        size_t p = m_currentProperty++;
        const PropertyHeader& info = m_properties[p];
        size_t esize = elementSize(info.dims);
        size_t n = info.size * esize;
        size_t ds = dataSizeInBytes(info.type);
        char* bdata = (char*)data;

        if (propertySanityCheck(propertyName, size, dims))
        {
            if (m_type == TextGTO)
            {
                PropertyPath p0 =
                    p == 0 ? PropertyPath() : m_propertyMap[p - 1];
                PropertyPath p1 = m_propertyMap[p];

                if (p0.objectIndex != p1.objectIndex)
                {
                    if (p != 0)
                    {
                        int s0 = p0.componentScope.size();

                        for (size_t i = 0; i < s0; i++)
                        {
                            writeIndent((s0 - i) * 4);
                            writeFormatted("}\n");
                        }

                        writeFormatted("}\n\n");
                    }

                    const ObjectHeader& o = m_objects[p1.objectIndex];
                    writeMaybeQuotedString(lookup(o.name));
                    writeFormatted(" : ");
                    writeMaybeQuotedString(lookup(o.protocolName));
                    writeFormatted(" (%d)\n{\n", o.protocolVersion);
                    p0 = PropertyPath();

                    //
                    //  Open up new scope
                    //

                    size_t s1 = p1.componentScope.size();

                    for (int i = 0; i <= s1 - 1; i++)
                    {
                        writeIndent((i + 1) * 4);
                        writeMaybeQuotedString(p1.componentScope[i].c_str());
                        writeFormatted("\n");
                        writeIndent((i + 1) * 4);
                        writeFormatted("{\n");
                    }
                }
                else
                {
                    size_t s0 = p0.componentScope.size();
                    size_t s1 = p1.componentScope.size();
                    int dindex = s1 > s0 ? s0 : -1;

                    for (int i = 0; i < s0 && i < s1; i++)
                    {
                        if (p0.componentScope[i] != p1.componentScope[i])
                        {
                            dindex = i;
                            break;
                        }
                    }

                    if (s0 != s1 || dindex != -1)
                    {
                        //
                        //  Close off existing scope
                        //

                        if (dindex > -1 && dindex <= s0 - 1)
                        {
                            for (int i = dindex; i < s0; i++)
                            {
                                writeIndent((s0 - i) * 4);
                                writeFormatted("}\n");
                            }
                        }

                        //
                        //  Open up new scope
                        //

                        for (int i = std::max(dindex, 0); i <= s1 - 1; i++)
                        {
                            if (i == dindex)
                                writeFormatted("\n");

                            writeIndent((i + 1) * 4);
                            writeMaybeQuotedString(
                                p1.componentScope[i].c_str());
                            writeFormatted("\n");
                            writeIndent((i + 1) * 4);
                            writeFormatted("{\n");
                        }
                    }
                }

                size_t childLevel1 = p1.componentScope.size();
                writeIndent((childLevel1 + 1) * 4);
                writeText(typeName(Gto::DataType(info.type)));

                //
                //  NOTE: we're not handling cases like float[2,0,2] which
                //  would be a 2x2 image in the X-Z plane. The assumption
                //  here is that you can't have a degenerate dimension
                //  before a non-degenerate one.
                //

                if (info.dims.x > 1 && info.dims.y == 0 && info.dims.z == 0)
                {
                    writeFormatted("[%d]", info.dims.x);
                }
                else if (info.dims.x > 0 && info.dims.y > 0 && info.dims.z == 0)
                {
                    writeFormatted("[%d,%d]", info.dims.x, info.dims.y);
                }
                else if (info.dims.x > 0 && info.dims.y > 0 && info.dims.z > 0)
                {
                    writeFormatted("[%d,%d,%d]", info.dims.x, info.dims.y,
                                   info.dims.z);
                }
                else if (info.dims.x > 0 && info.dims.y > 0 && info.dims.z > 0
                         && info.dims.w > 0)
                {
                    writeFormatted("[%d,%d,%d,%d]", info.dims.x, info.dims.y,
                                   info.dims.z, info.dims.w);
                }

                writeText(" ");
                writeMaybeQuotedString(lookup(info.name));

                if (info.interpretation)
                {
                    writeText(" as ");
                    writeMaybeQuotedString(lookup(info.interpretation));
                }

                writeText(" =");

                if (n == 0)
                    writeText(" [ ]");
                if (n > 1)
                    writeText(" [");

                for (size_t i = 0; i < n; i++)
                {
                    if (esize > 1)
                    {
                        if (i % esize == 0)
                        {
                            if (i)
                                writeFormatted(" ]");
                            writeFormatted(" [");
                        }
                    }

                    if (isNumber(DataType(info.type)))
                    {
                        Number num =
                            asNumber(bdata + (i * ds), DataType(info.type));

                        if (num.type == Int)
                        {
                            writeFormatted(" %d", num._int);
                        }
                        else if (num.type == Double)
                        {
                            writeFormatted(" %.18g", double(num._double));
                        }
                        else
                        {
                            writeFormatted(" %.9g", double(num._double));
                        }
                    }
                    else
                    {
                        char* s = bdata + (i * ds);
                        writeText(" ");
                        writeQuotedString(lookup(*(int*)s));
                    }
                }

                if (n > 0
                    && (info.dims.x > 1 || info.dims.y > 0 || info.dims.z > 0
                        || info.dims.w > 0))
                {
                    writeText(" ]");
                }

                if (n > 1)
                    writeText(" ]");

                writeText("\n");
            }
            else
            {
                size_t bytes = dataSizeInBytes(m_properties[p].type) * n;
                write(data, bytes);
            }
        }
    }

} // namespace Gto

#ifdef _MSC_VER
#undef snprintf
#endif
