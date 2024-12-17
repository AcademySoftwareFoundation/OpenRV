//******************************************************************************
// Copyright (c) 2001-2013 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

// for debugging output
// #include <IOtiff/IOtiff.h>

#include <IOexr/IOexr.h>
#include <IOexr/FileStreamIStream.h>
#include <ImfBoxAttribute.h>
#include <ImathBoxAlgo.h>
#include <ImfChannelList.h>
#include <ImfCompression.h>
#include <ImfVersion.h>
#include <ImfStandardAttributes.h>
#include <ImfCompressionAttribute.h>
#include <ImfTileDescriptionAttribute.h>
#include <ImfMultiPartInputFile.h>
#include <ImfInputPart.h>
#include <ImfInputFile.h>
#include <ImfRgbaFile.h>
#include <ImfIntAttribute.h>
#include <ImfRationalAttribute.h>
#include <ImfLineOrderAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfMultiView.h>
#include <ImfOutputFile.h>
#include <ImfAcesFile.h>
#include <ImfStringAttribute.h>
#include <ImfStringVectorAttribute.h>
#include <ImfTileDescription.h>
#include <ImfVecAttribute.h>
#include <ImfTimeCodeAttribute.h>
#include <ImfChromaticitiesAttribute.h>
#include <TwkUtil/Interrupt.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Vec2.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stl_ext/string_algo.h>
#include <string>
#include <vector>

#include <IOexr/Logger.h>

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;

    TwkMath::Vec2i convert(const Imath::V2i& v)
    {
        return TwkMath::Vec2i(v.x, v.y);
    }

    bool IOexr::isAMultiPartSharedAttribute(const std::string& name)
    {
        if (name == "displayWindow" || name == "pixelAspectRatio"
            || name == Imf::TimeCodeAttribute::staticTypeName()
            || name == Imf::ChromaticitiesAttribute::staticTypeName())
        {
            return true;
        }
        return false;
    }

    bool IOexr::isAces(const Imf::Chromaticities& c)
    {
        return c.red == acesChromaticities().red
               && c.green == acesChromaticities().green
               && c.blue == acesChromaticities().blue
               && c.white == acesChromaticities().white;
    }

    // Note: This method was taken from the internal (not exported)
    // Imf::acesChromaticities()
    const Imf::Chromaticities& IOexr::acesChromaticities()
    {
        static const Imf::Chromaticities acesChr(
            Imath::V2f(0.73470f, 0.26530f),  // red
            Imath::V2f(0.00000f, 1.00000f),  // green
            Imath::V2f(0.00010f, -0.07700f), // blue
            Imath::V2f(0.32168f, 0.33767f)); // white

        return acesChr;
    }

    static bool isXYZ(const Imf::Chromaticities& c)
    {
        return c.red == Imath::V2f(1, 0) && c.green == Imath::V2f(0, 1)
               && c.blue == Imath::V2f(0, 0) && c.white == Imath::V2f(0.333333);
    }

    template <class T>
    std::ostream& output(std::ostream& s, const Imath::Matrix33<T>& m)
    {
        s << m[0][0] << " " << m[0][1] << " " << m[0][2] << "\n"
          << " " << m[1][0] << " " << m[1][1] << " " << m[1][2] << "\n"
          << " " << m[2][0] << " " << m[2][1] << " " << m[2][2] << "\n";

        return s;
    }

    template <class T>
    std::ostream& output(std::ostream& s, const Imath::Matrix44<T>& m)
    {
        s << m[0][0] << " " << m[0][1] << " " << m[0][2] << " " << m[0][3]
          << "\n"
          <<

            " " << m[1][0] << " " << m[1][1] << " " << m[1][2] << " " << m[1][3]
          << "\n"
          <<

            " " << m[2][0] << " " << m[2][1] << " " << m[2][2] << " " << m[2][3]
          << "\n"
          <<

            " " << m[3][0] << " " << m[3][1] << " " << m[3][2] << " " << m[3][3]
          << "\n";

        return s;
    }

    int IOexr::indexOfChannelName(const vector<string>& channelNames,
                                  const char* names[], bool exact)
    {
        for (size_t i = 0; i < channelNames.size(); i++)
        {
            string name = channelNames[i];
            string::size_type index = name.rfind(".");
            if (!exact && index != string::npos)
                name = name.substr(index + 1, name.size() - index);

            for (const char** p = names; *p; p++)
            {
                if (name == *p)
                    return i;
            }
        }

        return -1;
    }

    int IOexr::indexOfChannelName(const vector<MultiPartChannel>& channelsMP,
                                  const char* names[], bool exact)
    {
        for (size_t i = 0; i < channelsMP.size(); i++)
        {
            string name = channelsMP[i].name;
            string::size_type index = name.rfind(".");
            if (!exact && index != string::npos)
                name = name.substr(index + 1, name.size() - index);

            for (const char** p = names; *p; p++)
            {
                if (name == *p)
                    return i;
            }
        }

        return -1;
    }

    int IOexr::alphaIndex(const vector<string>& channelNames)
    {
        static const char* names[] = {"a", "A", "alpha", "Alpha", NULL};
        return indexOfChannelName(channelNames, names);
    }

    int IOexr::alphaIndex(const vector<MultiPartChannel>& channelsMP)
    {
        static const char* names[] = {"a", "A", "alpha", "Alpha", NULL};
        return indexOfChannelName(channelsMP, names);
    }

    int IOexr::redIndex(const vector<string>& channelNames)
    {
        static const char* names[] = {"r", "R", "red", "Red", NULL};
        return indexOfChannelName(channelNames, names);
    }

    int IOexr::redIndex(const vector<MultiPartChannel>& channelsMP)
    {
        static const char* names[] = {"r", "R", "red", "Red", NULL};
        return indexOfChannelName(channelsMP, names);
    }

    int IOexr::greenIndex(const vector<string>& channelNames)
    {
        static const char* names[] = {"g", "G", "green", "Green", NULL};
        return indexOfChannelName(channelNames, names);
    }

    int IOexr::greenIndex(const vector<MultiPartChannel>& channelsMP)
    {
        static const char* names[] = {"g", "G", "green", "Green", NULL};
        return indexOfChannelName(channelsMP, names);
    }

    int IOexr::blueIndex(const vector<string>& channelNames)
    {
        static const char* names[] = {"b", "B", "blue", "Blue", NULL};
        return indexOfChannelName(channelNames, names);
    }

    int IOexr::blueIndex(const vector<MultiPartChannel>& channelsMP)
    {
        static const char* names[] = {"b", "B", "blue", "Blue", NULL};
        return indexOfChannelName(channelsMP, names);
    }

    bool IOexr::channelIsRGB(const string& channelName)
    {
        static const char* names[] = {"r",    "R",     "red",   "Red", "g",
                                      "G",    "green", "Green", "b",   "B",
                                      "blue", "Blue",  NULL};
        string name = channelName;
        string::size_type index = name.rfind(".");
        if (index != string::npos)
            name = name.substr(index + 1, name.size() - index);

        for (const char** p = names; *p; p++)
        {
            if (name == *p)
                return true;
        }
        return false;
    }

    string IOexr::findAnAlpha(const Imf::MultiPartInputFile& file, int partNum,
                              const string& layer, const string& view)
    {
        Imf::ChannelList cl = file.header(partNum).channels();

        string defaultA = "";
        string viewA = "";

        for (Imf::ChannelList::Iterator i = cl.begin(); i != cl.end(); ++i)
        {
            string t = i.name();

            if (t == "a" || t == "A" || t == "alpha" || t == "Alpha")
            {
                defaultA = t;
            }

            string::size_type index = t.rfind(".");

            if (index != string::npos)
            {
                string base = t.substr(0, index);

                if (base == view)
                {
                    string n = t.substr(index + 1, t.size() - index);
                    if (n == "a" || n == "A" || n == "alpha" || n == "Alpha")
                    {
                        viewA = n;
                    }
                }
            }
        }

        if (viewA != "")
            return viewA;
        return defaultA;
    }

    string IOexr::fullChannelName(const MultiPartChannel& mpChannel)
    {
        if (!mpChannel.partName.empty())
        {
            return mpChannel.partName + '.' + mpChannel.name;
        }

        return mpChannel.name;
    }

    void IOexr::findAnAlphaInView(const Imf::MultiPartInputFile& file,
                                  const string& view, const string& partName,
                                  MultiPartChannel& alpha)
    {
        const int noOfParts = file.parts();
        bool foundBestMatch = false;
        for (int p = 0; p < noOfParts; ++p)
        {
            if ((file.header(p).hasView() && file.header(p).view() == view)
                || (view == ""))
            {
                Imf::ChannelList cl = file.header(p).channels();
                for (Imf::ChannelList::Iterator ci = cl.begin(); ci != cl.end();
                     ++ci)
                {
                    string t = ci.name();

                    if (t == "a" || t == "A" || t == "alpha" || t == "Alpha")
                    {
                        alpha.name = t;
                        alpha.channel = ci.channel();
                        alpha.partNumber = p;
                        alpha.partName =
                            (file.header(p).hasName() ? file.header(p).name()
                                                      : "");

                        if (partName == alpha.partName)
                        {
                            foundBestMatch = true;
                        }
                    }

                    string::size_type index = t.rfind(".");

                    if (index != string::npos)
                    {
                        string base = t.substr(0, index);

                        if (base == view)
                        {
                            string n = t.substr(index + 1, t.size() - index);
                            if (n == "a" || n == "A" || n == "alpha"
                                || n == "Alpha")
                            {
                                alpha.name = n;
                                alpha.channel = ci.channel();
                                alpha.partNumber = p;
                                alpha.partName = (file.header(p).hasName()
                                                      ? file.header(p).name()
                                                      : "");

                                if (partName == alpha.partName)
                                {
                                    foundBestMatch = true;
                                }
                            }
                        }
                    }
                }
                if (foundBestMatch)
                {
                    // Do not bother searching other parts in the same view.
                    break;
                }
            }
        }

        if (alpha.name != "")
        {
            alpha.fullname = fullChannelName(alpha);
        }
    }

    string IOexr::baseChannelName(const string& name)
    {
        string::size_type i = name.rfind(".");
        if (i == string::npos)
            return name;
        string rval;
        ++i;
        rval.insert(rval.begin(), name.begin() + i, name.end());
        return rval;
    }

    void IOexr::channelSplit(const string& name, string& base,
                             string& layerView)
    {
        layerView.clear();
        base.clear();

        string::size_type i = name.rfind(".");

        if (i == string::npos)
        {
            base = name;
        }
        else
        {
            base.insert(base.begin(), name.begin() + (i + 1), name.end());
            layerView.insert(layerView.begin(), name.begin(), name.begin() + i);
        }
    }

    void IOexr::canonicalName(string& a)
    {
        //
        //  (We need to call blue "B" for any value of BLUE -- the *third* color
        //  value). Because EXR has lexically ordered channels, you actually
        //  have to know which one is red, green, and blue. That's a big
        //  problem. I keep seeing prman output with r g b instead of R G B. I
        //  have now seen one with "red" too.
        //

        if (a == "r")
            a = "R";
        else if (a == "g")
            a = "G";
        else if (a == "b")
            a = "B";
        else if (a == "a")
            a = "A";
        else if (a == "ry")
            a = "RY";
        else if (a == "by")
            a = "BY";
        else if (a == "y")
            a = "Y";
        else if (a == "u")
            a = "U";
        else if (a == "v")
            a = "V";
        else if (a == "red")
            a = "R";
        else if (a == "green")
            a = "G";
        else if (a == "blue")
            a = "B";
        else if (a == "alpha")
            a = "A";
        else if (a == "Red")
            a = "R";
        else if (a == "Green")
            a = "G";
        else if (a == "Blue")
            a = "B";
        else if (a == "Alpha")
            a = "A";
        else if (a == "x")
            a = "X";
        else if (a == "z")
            a = "Z";
    }

    Imf::ChannelList::Iterator
    IOexr::findChannelWithBasename(const string& cname, Imf::ChannelList& cl)
    {
        //
        //  Check first for exact matches, otherwise we'll return "BLAH.R" for
        //  "R" if it happens to appear earlier in the channel list.
        //

        for (Imf::ChannelList::Iterator i = cl.begin(); i != cl.end(); ++i)
        {
            if (i.name() == cname)
            {
                //  cerr << "Found exact " << i.name() << endl;
                return i;
            }
        }

        for (Imf::ChannelList::Iterator i = cl.begin(); i != cl.end(); ++i)
        {
            string base = baseChannelName(i.name());
            canonicalName(base);

            if (base == cname)
            {
                //  cerr << "Found " << i.name() << endl;
                return i;
            }
        }

        return cl.end();
    }

    vector<IOexr::MultiPartChannel>::const_iterator
    IOexr::findChannelWithBasenameMP(
        const string& cname, const vector<MultiPartChannel>& mpChannelList)
    {
        //
        //  Check first for exact matches, otherwise we'll return "BLAH.R" for
        //  "R" if it happens to appear earlier in the channel list.
        //

        for (vector<MultiPartChannel>::const_iterator i = mpChannelList.begin();
             i != mpChannelList.end(); ++i)
        {
            if (i->name == cname)
            {
                // cerr << "Found exact " << i->name << endl;
                return i;
            }
        }

        for (vector<MultiPartChannel>::const_iterator i = mpChannelList.begin();
             i != mpChannelList.end(); ++i)
        {
            string base = baseChannelName(i->name);
            canonicalName(base);

            if (base == cname)
            {
                // cerr << "Found " << i->name << endl;
                return i;
            }
        }

        return mpChannelList.end();
    }

    bool IOexr::LayerOrNamedViewChannel(const string& c)
    {
        return c.rfind(".") != string::npos;
    }

    bool IOexr::LayerOrNamedViewChannelMP(const MultiPartChannel& mp)
    {
        return LayerOrNamedViewChannel(mp.name);
    }

    int IOexr::channelOrder(string& s)
    {
        if (s == "R")
            return 1;
        if (s == "G")
            return 2;
        if (s == "B")
            return 3;
        if (s == "X")
            return 4;
        if (s == "Y")
            return 5;
        if (s == "U")
            return 6;
        if (s == "V")
            return 7;
        if (s == "RY")
            return 8;
        if (s == "BY")
            return 9;
        if (s == "A")
            return 10;

        return 0;
    }

    bool IOexr::ChannelComp(const string& ia, const string& ib)
    {
        //
        //  Sorts channels. The basename is used to sort so that
        //  R < G < B < A
        //

        string a, ap, b, bp;
        channelSplit(ia, a, ap);
        channelSplit(ib, b, bp);

        if (ap != bp)
        {
            // Because we ingest media from everyone, there are unfortunately
            // cases where Nuke appears to break the EXR specification and
            // doesn't write a default layer/view with multipart EXRs.  In these
            // cases, rgba appears to be the most fruitful default layer. Prefer
            // that layer over other layers.
            static const char* defaultLayerNames[] = {"rgba", "rgb", NULL};
            for (const char** p = defaultLayerNames; *p; p++)
            {
                string::size_type index = ap.rfind(".");
                if (index != string::npos)
                {
                    string apLayer = ap.substr(0, index);
                    if (apLayer == *p)
                    {
                        return true;
                    }
                }
                index = bp.rfind(".");
                if (index != string::npos)
                {
                    string bpLayer = bp.substr(0, index);
                    if (bpLayer == *p)
                    {
                        return false;
                    }
                }
            }
        }

        //
        //  prefer names with fewer dots
        //

        int aDotCount = std::count(ia.begin(), ia.end(), '.');
        int bDotCount = std::count(ib.begin(), ib.end(), '.');

        if (aDotCount < bDotCount)
            return true;
        if (aDotCount > bDotCount)
            return false;

        if (ap != bp)
        {
            return ap.compare(bp) < 0;
        }

        //
        //  The PRMAN driver writes out "r" "g" and "b" which completely
        //  messes with display. So we define a bunch of canonical names:
        //  R G B A Y U V RY BY which are always converted to uppercase
        //

        canonicalName(a);
        canonicalName(b);

        //
        //  Must implement a Strict Weak Ordering.  In particular if
        //  "x < y" is true then "y < x" must be false.
        //

        int aOrder = channelOrder(a);
        int bOrder = channelOrder(b);

        if (aOrder && bOrder)
            return aOrder < bOrder;
        if (aOrder)
            return true;
        if (bOrder)
            return false;

        return a.compare(b) < 0;
    }

    bool IOexr::ChannelCompMP(const MultiPartChannel& ia,
                              const MultiPartChannel& ib)
    {
        return ChannelComp(ia.fullname, ib.fullname);
    }

    //----------------------------------------------------------------------

    IOexr::IOexr(bool rgbaOnly, bool convertYRYBY, bool planar3channel,
                 bool inheritChannels, bool noOneChannelPlanes, bool stripAlpha,
                 bool readWindowIsDisplayWindow, ReadWindow readWindow,
                 WriteMethod writeMethod, IOType type, size_t chunk,
                 int maxAsync)
        : StreamingFrameBufferIO("IOexr", "m0", type, chunk, maxAsync)
        , m_rgbaOnly(rgbaOnly)
        , m_convertYRYBY(convertYRYBY)
        , m_planar3channel(planar3channel)
        , m_inheritChannels(inheritChannels)
        , m_noOneChannelPlanes(noOneChannelPlanes)
        , m_stripAlpha(stripAlpha)
        , m_readWindowIsDisplayWindow(readWindowIsDisplayWindow)
        , m_readWindow(readWindow)
        , m_writeMethod(writeMethod)
    {
        unsigned int cap = ImageRead | ImageWrite | BruteForceIO | PlanarRead
                           | PlanarWrite | Float16Capable | Float32Capable;

        StringPairVector codecs;
        codecs.push_back(StringPair("PIZ", "piz-based wavelet compression"));
        codecs.push_back(
            StringPair("ZIP", "zlib compression, in blocks of 16 scan lines"));
        codecs.push_back(
            StringPair("ZIPS", "zlib compression, one scan line at a time"));
        codecs.push_back(StringPair("RLE", "run length encoding"));
        codecs.push_back(StringPair("PXR24", "lossy 24-bit float compression"));
        codecs.push_back(StringPair(
            "B44",
            "lossy 4-by-4 pixel block compression, fixed compression rate"));
        codecs.push_back(
            StringPair("B44A", "lossy 4-by-4 pixel block compression, flat "
                               "fields are comressed more"));
        codecs.push_back(StringPair(
            "DWAA", "lossy DCT based compression, in blocks of 32 scanlines"));
        codecs.push_back(StringPair(
            "DWAB", "lossy DCT based compression, in blocks of 256 scanlines"));
        codecs.push_back(StringPair("NONE", "uncompressed"));

        addType("exr", "OpenEXR Image", cap, codecs);
        addType("txr", "TXR OpenEXR Image", cap, codecs);
        addType("openexr", "OpenEXR Image", cap, codecs);
        addType("aces", "Academy Color Encoding Specification Image", cap,
                codecs);
        addType("sxr", "Stereo/Multiview/Multipart OpenEXR Image", cap, codecs);
    }

    IOexr::~IOexr() {}

    bool IOexr::getBoolAttribute(const std::string& name) const
    {
        if (name == "convertYRYBY")
            return m_convertYRYBY;
        else if (name == "planar3channel")
            return m_planar3channel;
        else if (name == "rgbaOnly")
            return m_rgbaOnly;
        else if (name == "inheritChannels")
            return m_inheritChannels;
        else if (name == "noOneChannelPlanes")
            return m_noOneChannelPlanes;
        else if (name == "stripAlpha")
            return m_stripAlpha;
        else if (name == "readWindowIsDisplayWindow")
            return m_readWindowIsDisplayWindow;
        return StreamingFrameBufferIO::getBoolAttribute(name);
    }

    void IOexr::setBoolAttribute(const std::string& name, bool value)
    {
        if (name == "convertYRYBY")
            m_convertYRYBY = value;
        else if (name == "planar3channel")
            m_planar3channel = value;
        else if (name == "rgbaOnly")
            m_rgbaOnly = value;
        else if (name == "inheritChannels")
            m_inheritChannels = value;
        else if (name == "noOneChannelPlanes")
            m_noOneChannelPlanes = value;
        else if (name == "stripAlpha")
            m_stripAlpha = value;
        else if (name == "readWindowIsDisplayWindow")
            m_readWindowIsDisplayWindow = value;
        else
            StreamingFrameBufferIO::setBoolAttribute(name, value);
    }

    int IOexr::getIntAttribute(const std::string& name) const
    {
        if (name == "readWindow")
            return int(m_readWindow);
        return StreamingFrameBufferIO::getIntAttribute(name);
    }

    void IOexr::setIntAttribute(const std::string& name, int value)
    {
        if (name == "readWindow")
            m_readWindow = (ReadWindow)value;
        else
            StreamingFrameBufferIO::setBoolAttribute(name, value);
    }

    string IOexr::about() const
    {
        ostringstream str;
        str << "OpenEXR (" << Imf::EXR_VERSION << ")";
        return str.str();
    }

    void IOexr::setRGBAOnly(bool only) { m_rgbaOnly = only; }

    void IOexr::convertYRYBY(bool convert) { m_convertYRYBY = convert; }

    void IOexr::planar3Channel(bool convert) { m_planar3channel = convert; }

    void IOexr::noOneChannelPlanes(bool hack) { m_noOneChannelPlanes = hack; }

    void IOexr::stripAlpha(bool strip) { m_stripAlpha = strip; }

    void IOexr::inheritChannels(bool b) { m_inheritChannels = b; }

    void IOexr::setReadWindow(ReadWindow window) { m_readWindow = window; }

    void IOexr::setWriteMethod(WriteMethod choice) { m_writeMethod = choice; }

    void IOexr::readAllAttributes(const Imf::MultiPartInputFile& file,
                                  FrameBuffer& fb, const set<int>& parts)
    {
        set<int> partNumbersRead;
        if (parts.empty())
        {
            // default to part zero in this case.
            partNumbersRead.insert(0);
        }
        else
        {
            partNumbersRead = parts;
        }

        //
        // Determine how many times an attr is shared across parts.
        //
        map<string, int> attrSharedCountTable;
        if (partNumbersRead.size() > 1)
        {
            for (set<int>::const_iterator it = partNumbersRead.begin();
                 it != partNumbersRead.end(); ++it)
            {
                const int partNum = (*it);

                for (Imf::Header::ConstIterator i =
                         file.header(partNum).begin();
                     i != file.header(partNum).end(); ++i)
                {
                    if (attrSharedCountTable.count(i.name()) == 0)
                    {
                        attrSharedCountTable[i.name()] = 1;
                    }
                    else
                    {
                        if (!isAMultiPartSharedAttribute(i.name()))
                        {
                            attrSharedCountTable[i.name()]++;
                        }
                    }
                }
            }
        }

        for (set<int>::const_iterator it = partNumbersRead.begin();
             it != partNumbersRead.end(); ++it)
        {
            const int partNum = (*it);

            // Get the file format version; as reported by exrheader.
            // NB: This is different from the EXR 'version' attribute.
            {
                const int fileFormatVersion = Imf::getVersion(file.version());
                ostringstream ostr;
                ostr << fileFormatVersion
                     << ", "
                        "flags 0x"
                     << setbase(16) << Imf::getFlags(file.version())
                     << setbase(10) << "\n";

                fb.newAttribute("EXR/file format version", ostr.str());
            }

            for (Imf::Header::ConstIterator i = file.header(partNum).begin();
                 i != file.header(partNum).end(); ++i)
            {
                const Imf::Attribute* attr = &(i.attribute());
                string aname = i.name();

                // Determine if the attribute exists more than once across the
                // parts and if so suffix the part number to the metaDataPrefix
                // e.g. /EXR/<part number>
                ostringstream metaDataPrefix;
                metaDataPrefix << "EXR/";
                if (attrSharedCountTable.count(aname) == 1
                    && attrSharedCountTable[aname] > 1)
                {
                    metaDataPrefix << partNum << '/';
                }

                bool hasSlash = aname.find('/') != string::npos;

                string name = hasSlash ? "" : metaDataPrefix.str();
                name += i.name();

#if 0
            cout << attr->typeName() << " " << name 
                 << " (" << typeid(*attr).name() << ")"
                 << endl;
#endif

                if (const Imf::StringAttribute* a =
                        dynamic_cast<const Imf::StringAttribute*>(attr))
                {
                    if (file.parts() > 1)
                    {
                        if ((name != "EXR/name") && (name != "EXR/view"))
                        {
                            fb.newAttribute(name, a->value());
                        }
                    }
                    else
                    {
                        fb.newAttribute(name, a->value());
                    }
                }
                else if (const Imf::StringVectorAttribute* a =
                             dynamic_cast<const Imf::StringVectorAttribute*>(
                                 attr))
                {
                    fb.newAttribute(name, a->value());
                }
                else if (const Imf::FloatAttribute* a =
                             dynamic_cast<const Imf::FloatAttribute*>(attr))
                {
                    ostringstream str;
                    str << a->value();
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::RationalAttribute* a =
                             dynamic_cast<const Imf::RationalAttribute*>(attr))
                {
                    ostringstream str;
                    str << float(a->value());
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::IntAttribute* a =
                             dynamic_cast<const Imf::IntAttribute*>(attr))
                {
                    if (name != "EXR/chunkCount")
                    {
                        ostringstream str;
                        str << a->value();
                        fb.newAttribute(name, str.str());
                    }
                }
                else if (const Imf::V2iAttribute* a =
                             dynamic_cast<const Imf::V2iAttribute*>(attr))
                {
                    ostringstream str;
                    str << a->value();
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::V3iAttribute* a =
                             dynamic_cast<const Imf::V3iAttribute*>(attr))
                {
                    ostringstream str;
                    str << a->value();
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::V2fAttribute* a =
                             dynamic_cast<const Imf::V2fAttribute*>(attr))
                {
                    if (name == "EXR/adoptedNeutral")
                    {
                        fb.setAdoptedNeutral(a->value().x, a->value().y);
                    }
                    else
                    {
                        ostringstream str;
                        str << a->value();
                        fb.newAttribute(name, str.str());
                    }
                }
                else if (const Imf::V3fAttribute* a =
                             dynamic_cast<const Imf::V3fAttribute*>(attr))
                {
                    ostringstream str;
                    str << a->value();
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::Box2iAttribute* a =
                             dynamic_cast<const Imf::Box2iAttribute*>(attr))
                {
                    ostringstream str;
                    str << a->value().min << " - " << a->value().max;
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::Box2fAttribute* a =
                             dynamic_cast<const Imf::Box2fAttribute*>(attr))
                {
                    ostringstream str;
                    str << a->value().min << " - " << a->value().max;
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::LineOrderAttribute* a =
                             dynamic_cast<const Imf::LineOrderAttribute*>(attr))
                {
                    ostringstream str;
                    switch (a->value())
                    {
                    case Imf::INCREASING_Y:
                        str << "INCREASING_Y";
                        break;
                    case Imf::DECREASING_Y:
                        str << "DECREASING_Y";
                        break;
                    case Imf::RANDOM_Y:
                        str << "RANDOM_Y";
                        break;
                    default:
                        str << "** bad value **";
                        break;
                    }
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::CompressionAttribute* a =
                             dynamic_cast<const Imf::CompressionAttribute*>(
                                 attr))
                {
                    ostringstream str;
                    switch (a->value())
                    {
                    case Imf::NO_COMPRESSION:
                        str << "NO_COMPRESSION";
                        break;
                    case Imf::RLE_COMPRESSION:
                        str << "RLE_COMPRESSION";
                        break;
                    case Imf::ZIPS_COMPRESSION:
                        str << "ZIPS_COMPRESSION";
                        break;
                    case Imf::ZIP_COMPRESSION:
                        str << "ZIP_COMPRESSION";
                        break;
                    case Imf::PIZ_COMPRESSION:
                        str << "PIZ_COMPRESSION";
                        break;
                    case Imf::PXR24_COMPRESSION:
                        str << "PXR24_COMPRESSION";
                        break;
                    case Imf::B44_COMPRESSION:
                        str << "B44_COMPRESSION";
                        break;
                    case Imf::B44A_COMPRESSION:
                        str << "B44A_COMPRESSION";
                        break;
                    case Imf::DWAA_COMPRESSION:
                        str << "DWAA_COMPRESSION";
                        break;
                    case Imf::DWAB_COMPRESSION:
                        str << "DWAB_COMPRESSION";
                        break;
                    default:
                        str << a->value();
                        break;
                    }
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::M44fAttribute* a =
                             dynamic_cast<const Imf::M44fAttribute*>(attr))
                {
                    ostringstream str;
                    output(str, a->value());
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::M33fAttribute* a =
                             dynamic_cast<const Imf::M33fAttribute*>(attr))
                {
                    ostringstream str;
                    output(str, a->value());
                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::ChromaticitiesAttribute* a =
                             dynamic_cast<const Imf::ChromaticitiesAttribute*>(
                                 attr))
                {
                    Imf::Chromaticities c = a->value();

                    fb.setPrimaries(c.white.x, c.white.y, c.red.x, c.red.y,
                                    c.green.x, c.green.y, c.blue.x, c.blue.y);

                    if (isAces(c))
                    {
                        fb.setPrimaryColorSpace(ColorSpace::ACES());
                    }
                    else if (isXYZ(c))
                    {
                        fb.setPrimaryColorSpace(ColorSpace::CIEXYZ());
                    }
                    else
                    {
                        fb.setPrimaryColorSpace(ColorSpace::Generic());
                    }
                }
                else if (const Imf::TileDescriptionAttribute* a =
                             dynamic_cast<const Imf::TileDescriptionAttribute*>(
                                 attr))
                {
                    fb.newAttribute("EXR/TileDescription/xSize",
                                    int(a->value().xSize));
                    fb.newAttribute("EXR/TileDescription/ySize",
                                    int(a->value().ySize));

                    string mode = "Unknown Level Mode";
                    switch (a->value().mode)
                    {
                    case Imf::ONE_LEVEL:
                        mode = "ONE_LEVEL";
                        break;
                    case Imf::MIPMAP_LEVELS:
                        mode = "MIPMAP_LEVELS";
                        break;
                    case Imf::RIPMAP_LEVELS:
                        mode = "RIPMAP_LEVELS";
                        break;
                    default:
                        break;
                    }

                    fb.newAttribute("EXR/TileDescription/mode", mode);

                    mode = "Unknown Rounding Mode";

                    switch (a->value().roundingMode)
                    {
                    case Imf::ROUND_DOWN:
                        mode = "ROUND_DOWN";
                        break;
                    case Imf::ROUND_UP:
                        mode = "ROUND_UP";
                        break;
                    default:
                        break;
                    }

                    fb.newAttribute("EXR/TileDescription/roundingMode", mode);
                }
                else if (const Imf::EnvmapAttribute* a =
                             dynamic_cast<const Imf::EnvmapAttribute*>(attr))
                {
                    ostringstream str;

                    if (a->value() == Imf::ENVMAP_CUBE)
                    {
                        str << "ENVMAP_CUBE";
                    }
                    else if (a->value() == Imf::ENVMAP_LATLONG)
                    {
                        str << "ENVMAP_LATLONG";
                    }
                    else
                    {
                        str << "Unknown ENV Type";
                    }

                    fb.newAttribute(name, str.str());
                }
                else if (const Imf::TimeCodeAttribute* a =
                             dynamic_cast<const Imf::TimeCodeAttribute*>(attr))
                {
                    fb.newAttribute("EXR/timeCode/userData",
                                    (int)a->value().userData());
                    fb.newAttribute("EXR/timeCode/bgf2",
                                    (int)a->value().bgf2());
                    fb.newAttribute("EXR/timeCode/bgf1",
                                    (int)a->value().bgf1());
                    fb.newAttribute("EXR/timeCode/bgf0",
                                    (int)a->value().bgf0());
                    fb.newAttribute("EXR/timeCode/fieldPhase",
                                    (int)a->value().fieldPhase());
                    fb.newAttribute("EXR/timeCode/colorFrame",
                                    (int)a->value().colorFrame());
                    fb.newAttribute("EXR/timeCode/dropFrame",
                                    (int)a->value().dropFrame());
                    fb.newAttribute("EXR/timeCode/frame", a->value().frame());
                    fb.newAttribute("EXR/timeCode/seconds",
                                    a->value().seconds());
                    fb.newAttribute("EXR/timeCode/minutes",
                                    a->value().minutes());
                    fb.newAttribute("EXR/timeCode/hours", a->value().hours());
                }
                else if (const Imf::KeyCodeAttribute* a =
                             dynamic_cast<const Imf::KeyCodeAttribute*>(attr))
                {
                    fb.newAttribute(metaDataPrefix.str()
                                        + string("KeyCode/filmMfcCode"),
                                    a->value().filmMfcCode());
                    fb.newAttribute(metaDataPrefix.str()
                                        + string("KeyCode/filmType"),
                                    a->value().filmType());
                    fb.newAttribute(metaDataPrefix.str()
                                        + string("KeyCode/prefix"),
                                    a->value().prefix());
                    fb.newAttribute(metaDataPrefix.str()
                                        + string("KeyCode/count"),
                                    a->value().count());
                    fb.newAttribute(metaDataPrefix.str()
                                        + string("KeyCode/perfOffset"),
                                    a->value().perfOffset());
                    fb.newAttribute(metaDataPrefix.str()
                                        + string("KeyCode/perfsPerFrame"),
                                    a->value().perfsPerFrame());
                    fb.newAttribute(metaDataPrefix.str()
                                        + string("KeyCode/perfsPerCount"),
                                    a->value().perfsPerCount());
                }
            }

            Imath::Box2i dspWin = file.header(partNum).displayWindow();
            Imath::Box2i datWin = file.header(partNum).dataWindow();

            if (dspWin.min != datWin.min || dspWin.max != datWin.max)
            {
                fb.newAttribute(
                    "DataWindowSize",
                    convert(datWin.max - datWin.min + Imath::V2i(1, 1)));
                fb.newAttribute("DataWindowOrigin", convert(datWin.min));
                fb.newAttribute(
                    "DisplayWindowSize",
                    convert(dspWin.max - dspWin.min + Imath::V2i(1, 1)));
                fb.newAttribute("DisplayWindowOrigin", convert(dspWin.min));
            }
        }
    }

    void IOexr::readAllAttributes(const Imf::MultiPartInputFile& file,
                                  FrameBuffer& fb)
    {
        set<int> parts;
        parts.insert(0);
        readAllAttributes(file, fb, parts);
    }

    size_t IOexr::channelListIteratorDifference(
        Imf::ChannelList::ConstIterator a,
        const Imf::ChannelList::ConstIterator& b)
    {
        size_t count = 0;
        while (a != b)
        {
            ++a;
            count++;
        }
        return count;
    }

    size_t IOexr::channelListSize(const Imf::ChannelList& cl)
    {
        return channelListIteratorDifference(cl.begin(), cl.end());
    }

    void IOexr::setChannelInfo(const Imf::ChannelList::ConstIterator& i,
                               FBInfo::ChannelInfo& info)
    {
        info.name = i.name();

        switch (i.channel().type)
        {
        case Imf::FLOAT:
            info.type = FrameBuffer::FLOAT;
            break;
        case Imf::HALF:
            info.type = FrameBuffer::HALF;
            break;
        case Imf::UINT:
            info.type = FrameBuffer::UINT;
            break;
        default:
            break;
        }
    }

    void IOexr::setChannelInfo(const string& name, Imf::PixelType type,
                               FBInfo::ChannelInfo& info)
    {
        info.name = name;

        switch (type)
        {
        case Imf::FLOAT:
            info.type = FrameBuffer::FLOAT;
            break;
        case Imf::HALF:
            info.type = FrameBuffer::HALF;
            break;
        case Imf::UINT:
            info.type = FrameBuffer::UINT;
            break;
        default:
            break;
        }
    }

    void IOexr::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        //
        //  This will throw if it can't open the file
        //

        Imf::MultiPartInputFile file(filename.c_str());
        const Imf::StringVectorAttribute* vattr = 0;
        int partNum = 0;
        if (file.parts() > 1)
        {
            // Implies read a MultiPart file
            getMultiPartImageInfo(file, fbi);
        }
        else
        {
            ViewNames views;
            if (vattr = file.header(partNum)
                            .findTypedAttribute<Imf::StringVectorAttribute>(
                                "multiView"))
            {
                // Implies read a MultiView file
                views = vattr->value();
            }
            getMultiViewImageInfo(file, views, fbi);
        }
    }

    void IOexr::readImages(FrameBufferVector& fbs, const std::string& filename,
                           const ReadRequest& request) const
    {
        if (m_iotype == StandardIO)
        {
            Imf::MultiPartInputFile file(filename.c_str());
            readImagesFromFile(file, fbs, filename, request);
        }
        else
        {
            FileStream::Type ftype = (FileStream::Type)(m_iotype - 1);
            FileStreamIStream fstream(filename, ftype, m_iosize, m_iomaxAsync);
            Imf::MultiPartInputFile file(fstream);
            readImagesFromFile(file, fbs, filename, request);
        }
    }

    void IOexr::readImagesFromFile(Imf::MultiPartInputFile& file,
                                   FrameBufferVector& fbs,
                                   const std::string& filename,
                                   const ReadRequest& request) const
    {
        fbs.clear();

#ifdef DEBUG_IOEXR
        LOG.log("Requesting views:");
        for (int i = 0; i < request.views.size(); ++i)
        {
            LOG.log("view  %s", request.views[i].c_str());
        }

        LOG.log("Requesting layers:");
        for (int i = 0; i < request.layers.size(); ++i)
        {
            LOG.log("layer %s", request.layers[i].c_str());
        }

        LOG.log("Requesting channels:");
        for (int i = 0; i < request.channels.size(); ++i)
        {
            LOG.log("channel %s", request.channels[i].c_str());
        }
#endif

        const string& requestedLayer =
            request.layers.empty() ? "" : request.layers.front();
        const string& requestedChannel =
            request.channels.empty() ? "" : request.channels.front();

        const Imf::StringVectorAttribute* vattr = 0;
        int partNum = 0;
        if (file.parts() > 1)
        {
            string requestedView;
            if (request.views.empty() || request.views.front().empty())
            {
                //
                //  Set the requesteView to the default view.
                //  Check if there is an exrAttribute first, if there isnt then
                //  use the "view" of part zero.
                //
                const Imf::Header& header = file.header(0);
                if (const Imf::StringAttribute* sAttr =
                        header.findTypedAttribute<Imf::StringAttribute>(
                            "defaultView"))
                {
                    requestedView = sAttr->value();
                }
                else
                {
                    requestedView = (header.hasView() ? header.view() : "");
                }
            }
            else
            {
                requestedView = request.views.front();
            }

            // Implies read a MultiPart file
            readImagesFromMultiPartFile(file, fbs, filename, requestedView,
                                        requestedLayer, requestedChannel,
                                        request.allChannels);
        }
        else
        {
            ViewNames views;
            if (vattr = file.header(partNum)
                            .findTypedAttribute<Imf::StringVectorAttribute>(
                                "multiView"))
            {
                // Implies read a MultiView file
                views = vattr->value();
            }

            string defaultView = Imf::defaultViewName(views);
            string requestedView;
            if (request.views.empty() || request.views.front().empty())
            {
                //
                //  Set the requestedView to the default view for multiview.
                //
                requestedView = defaultView;
            }
            else
            {
                if (std::find(views.begin(), views.end(), request.views.front())
                    != views.end())
                {
                    requestedView = request.views.front();
                }
                else
                {
                    // Requested view not found in views, reverting to default
                    requestedView = defaultView;
                }
            }

            readImagesFromMultiViewFile(file, fbs, filename, requestedView,
                                        requestedLayer, requestedChannel,
                                        request.allChannels, partNum, views,
                                        (requestedView == defaultView));
        }
    }

    void IOexr::writeImages(const ConstFrameBufferVector& fbs,
                            const std::string& filename,
                            const WriteRequest& request) const
    {
        if (m_writeMethod == IOexr::MultiPartWriter)
        {
            // Implies read a MultiPart file
            writeImagesToMultiPartFile(fbs, filename, request);
        }
        else
        {
            writeImagesToMultiViewFile(fbs, filename, request);
        }
    }

} //  End namespace TwkFB
