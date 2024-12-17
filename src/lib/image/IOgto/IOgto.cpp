//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IOgto/IOgto.h>
#include <IOgto/ImageReader.h>
#include <TwkUtil/FileStream.h>
#include <TwkUtil/FileMMap.h>
#include <TwkUtil/File.h>
#include <TwkFB/Exception.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <TwkFB/Operations.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/StdioBuf.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkUtil/MemBuf.h>
#include <TwkMath/Color.h>
#include <stl_ext/string_algo.h>
#include <string>
#include <Gto/Reader.h>
#include <Gto/Writer.h>

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkMath;

    IOgto::IOgto(IOType type, size_t chunkSize, int maxAsync)
        : StreamingFrameBufferIO("IOgto", "m1", type, chunkSize, maxAsync)
    {
        //
        //  Indicate which extensions this plugin will handle The
        //  comparison against. The extensions are case-insensitive so
        //  there's no reason to provide upper case versions.
        //

        StringPairVector codecs;
        codecs.push_back(
            StringPair("text", "Text (no compression, huge file)"));
        codecs.push_back(StringPair("raw", "Raw (no compression)"));
        codecs.push_back(StringPair("zip", "ZIP compression"));

        unsigned int cap = ImageRead | ImageWrite | Int8Capable;

        addType("gto", "GTO Image", cap, codecs);
        addType("igto", "GTO Image", cap, codecs);
    }

    IOgto::~IOgto() {}

    string IOgto::about() const { return "GTO (Tweak)"; }

    void IOgto::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        ifstream file(UNICODE_C_STR(filename.c_str()));

        if (file)
        {
            FrameBufferVector fbs;
            ImageReader reader(fbs, file, filename, true); // don't read
                                                           // pixel data

            const ImageReader::ImageVector& images = reader.images();

            for (size_t i = 0; i < images.size(); i++)
            {
                const ImageReader::Image& image = images[i];

                if (i == 0)
                {
                    fbi.width = image.imageSizes[0];
                    fbi.height = image.imageSizes[1];
                    fbi.uncropWidth = image.dataSize[0];
                    fbi.uncropHeight = image.dataSize[1];
                    fbi.uncropX = image.dataOrigin[0];
                    fbi.uncropY = image.dataOrigin[1];
                    fbi.pixelAspect = image.pixelAspect;

                    if (image.planes.size())
                    {
                        const ImageReader::Plane& plane = image.planes[0];

                        fbi.numChannels = image.planes.size() > 1
                                              ? image.planes.size()
                                              : plane.channels.size();
                        fbi.dataType = (FrameBuffer::DataType)plane.dataType;
                        fbi.orientation =
                            (FrameBuffer::Orientation)plane.orientation;
                    }

                    image.fb->copyAttributesTo(&fbi.proxy);
                }

                fbi.views.push_back(image.viewName);
                // fbi.layers.push_back(image.viewName); TODO
            }
        }
    }

    void IOgto::readImages(FrameBufferVector& fbs, const string& filename,
                           const ReadRequest& request) const
    {
        FileStream::Type ftype = m_iotype == StandardIO
                                     ? FileStream::Buffering
                                     : (FileStream::Type)(m_iotype - 1);

        FileStream fstream(filename, ftype, m_iosize, m_iomaxAsync);

        readImageInMemory(fbs, fstream.data(), fstream.size(), filename,
                          request);
    }

    void IOgto::readImageStream(FrameBufferVector& fbs, istream& instream,
                                const string& filename,
                                const ReadRequest& request) const
    {
        ImageReader reader(fbs, instream, filename);
    }

    void IOgto::readImageInMemory(FrameBufferVector& fbs, void* data,
                                  size_t size, const string& filename,
                                  const ReadRequest& request) const
    {
        ImageReader reader(fbs, data, size, filename);
    }

    //----------------------------------------------------------------------
    //
    //  WRITING
    //

    void IOgto::writeImages(const ConstFrameBufferVector& imgs,
                            const std::string& filename,
                            const WriteRequest& request) const
    {
        ofstream file(UNICODE_C_STR(filename.c_str()));

        if (file)
        {
            writeImageStream(imgs, file, request);
        }
        else
        {
            TWK_THROW_STREAM(IOException,
                             "cannot open \"" << filename << "\" for writing");
        }
    }

    void IOgto::writeImageStream(const ConstFrameBufferVector& fbs,
                                 ostream& outstream,
                                 const WriteRequest& request) const
    {
        Gto::Writer writer(outstream);
        vector<WriteState> states(fbs.size());

        writer.intern("raw");   // codec and encoding for planes
        writer.intern("color"); // role for planes

        for (size_t i = 0; i < fbs.size(); i++)
        {
            ostringstream name;
            name << "image" << i;

            states[i].viewName = name.str();
            states[i].index = i;
            states[i].fb = fbs[i];

            //
            //  Plane names are all the channel names concatenated
            //  together. Normally a plane will only have one channel so
            //  that's usually the name
            //

            for (const FrameBuffer* f = fbs[i]; f; f = f->nextPlane())
            {
                ostringstream str;
                for (size_t q = 0; q < f->numChannels(); q++)
                    str << f->channelName(q);
                states[i].planeNames.push_back(str.str());
                states[i].planes.push_back(f);
                writer.intern(str.str());
            }
        }

        for (size_t i = 0; i < fbs.size(); i++)
        {
            declareOneImageStream(writer, states[i], request);
        }

        writer.beginData();

        for (size_t i = 0; i < fbs.size(); i++)
        {
            writeOneImageStream(writer, states[i], outstream, request);
        }

        writer.endData();
    }

    void IOgto::declareOneImageStream(Gto::Writer& writer, WriteState& state,
                                      const WriteRequest& request) const
    {
        //
        //  The way GTO write works, you have to declare everything up
        //  front before you provide the actual data. So the writing is
        //  done in two passes. This is the declaration pass.
        //

        writer.beginObject(state.viewName.c_str(), "image", 1);

        writer.beginComponent("geometry", "geometry");
        writer.property("size", Gto::Int,
                        2); // only 2D images for the time being
        writer.property("dataWindowOrigin", Gto::Int, 2);
        writer.property("dataWindowSize", Gto::Int, 2);
        writer.property("pixelAspectRatio", Gto::Float, 1);
        writer.property("planes", Gto::String, state.planeNames.size());
        writer.endComponent();

        writer.beginComponent("attributes", "attributes");

        const FrameBuffer* fb = state.fb;
        const FrameBuffer::AttributeVector& attrs = fb->attributes();

        //
        //  Declare all the remaining attrs including any user defined
        //  ones
        //
        //  NOTE: clang makes the scope of a variable declared in if test
        //  continue through else. Is that correct? Doesn't VS do this too?
        //

        for (size_t i = 0; i < attrs.size(); i++)
        {
            Gto::DataType atype = Gto::ErrorType;
            int awidth = 1;

            const FBAttribute* a = attrs[i];

            if (const FloatAttribute* ta =
                    dynamic_cast<const FloatAttribute*>(a))
            {
                atype = Gto::Float;
            }
            else if (const Vec2fAttribute* va =
                         dynamic_cast<const Vec2fAttribute*>(a))
            {
                atype = Gto::Float;
                awidth = 2;
            }
            else if (const Mat44fAttribute* ma =
                         dynamic_cast<const Mat44fAttribute*>(a))
            {
                atype = Gto::Float;
                awidth = 16;
            }
            else if (const IntAttribute* ia =
                         dynamic_cast<const IntAttribute*>(a))
            {
                atype = Gto::Int;
            }
            else if (const StringAttribute* sa =
                         dynamic_cast<const StringAttribute*>(a))
            {
                atype = Gto::String;
                string v = sa->value(); // prevent windows issues
                writer.intern(v.c_str());
            }

            if (atype != Gto::ErrorType)
            {
                //
                //  NOTE: this is a bit weird. When rvio generates a
                //  framebuffer from the graph, it accumulates *all* of
                //  the attributes from *all* of the images. In order to
                //  keep track of where they came from the full path to
                //  the source+media is prepended to the name. So here
                //  we're just removing that part and using the simple
                //  attribute name directly.
                //

                string::size_type p = a->name().find("/./");
                string aname;

                if (p == string::npos)
                    aname = a->name();
                else
                    aname = a->name().substr(p + 3, string::npos);

                //
                //  Only single value of one dimension right now
                //

                writer.property(aname.c_str(), atype, 1, awidth);
            }
        }

        writer.endComponent();

        //
        //  Declare the pixel planes
        //

        for (size_t i = 0; i < state.planeNames.size(); i++)
        {
            const FrameBuffer* plane = state.planes[i];

            for (size_t q = 0; q < plane->numChannels(); q++)
            {
                writer.intern(plane->channelName(q).c_str());
            }

            writer.beginComponent(state.planeNames[i].c_str(), "plane");

            writer.property("size", Gto::Int, 2);
            writer.property("orientation", Gto::Int, 1);
            writer.property("encoding", Gto::String, 1);
            writer.property("codec", Gto::String, 1);
            writer.property("channels", Gto::String, plane->numChannels());
            writer.property("role", Gto::String, 1);
            writer.property("bitdepth", Gto::Int, 1);
            writer.property("scanlinePadding", Gto::Int, 1);
            writer.property("extraScanlines", Gto::Int, 1);
            writer.property("pixelAspectRatio", Gto::Float, 1);
            writer.property("dataType", Gto::Int, 1);

            Gto::DataType ptype = Gto::ErrorType;

            switch (plane->dataType())
            {
            case FrameBuffer::BIT:
            case FrameBuffer::UCHAR:
                ptype = Gto::Byte;
                break;
            case FrameBuffer::USHORT:
                ptype = Gto::Short;
                break;
            case FrameBuffer::FLOAT:
                ptype = Gto::Float;
                break;
            case FrameBuffer::HALF:
                ptype = Gto::Half;
                break;
            case FrameBuffer::DOUBLE:
                ptype = Gto::Double;
                break;
            case FrameBuffer::UINT:
            case FrameBuffer::PACKED_R10_G10_B10_X2:
            case FrameBuffer::PACKED_X2_B10_G10_R10:
            case FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
            case FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
                ptype = Gto::Int;
                break;
            }

            size_t total = ((plane->width() + plane->scanlinePixelPadding())
                            * (plane->height() + plane->extraScanlines()))
                           * plane->numChannels();

            writer.property("pixels", ptype, total, 1, "pixels");
            writer.endComponent();
        }

        writer.endObject();
    }

    void IOgto::writeOneImageStream(Gto::Writer& writer, WriteState& state,
                                    ostream& outstream,
                                    const WriteRequest& request) const
    {
        const FrameBuffer* fb = state.fb;
        const FrameBuffer::AttributeVector& attrs = fb->attributes();

        vector<int> sizes(2);
        sizes[0] = fb->width();
        sizes[1] = fb->height();

        vector<int> dataWindowOrigin(2);
        vector<int> dataWindowSize(2);

        dataWindowOrigin[0] = fb->uncropX();
        dataWindowOrigin[1] = fb->uncropY();
        dataWindowSize[0] = fb->uncropWidth();
        dataWindowSize[1] = fb->uncropHeight();

        //
        //  Need to convert the strings into stringIDs
        //
        vector<int> planeNames(state.planeNames.size());

        for (size_t i = 0; i < state.planeNames.size(); i++)
        {
            planeNames[i] = writer.lookup(state.planeNames[i].c_str());
        }

        float pixelAspect = fb->pixelAspectRatio();

        writer.propertyData(sizes, "size");
        writer.propertyData(dataWindowOrigin, "dataWindowOrigin");
        writer.propertyData(dataWindowSize, "dataWindowSize");
        writer.propertyData(&pixelAspect, "pixelAspectRatio");
        writer.propertyData(planeNames, "planes");

        //
        //  Now the user attrs
        //

        for (size_t i = 0; i < attrs.size(); i++)
        {
            const FBAttribute* a = attrs[i];

            string::size_type p = a->name().find("/./");
            string aname;

            if (p == string::npos)
                aname = a->name();
            else
                aname = a->name().substr(p + 3, string::npos);

            if (const FloatAttribute* fa =
                    dynamic_cast<const FloatAttribute*>(a))
            {
                float v = fa->value();
                writer.propertyData(&v, aname.c_str(), 1);
            }
            else if (const Vec2fAttribute* va =
                         dynamic_cast<const Vec2fAttribute*>(a))
            {
                TwkMath::Vec2f v = va->value();
                writer.propertyData(&v, aname.c_str(), 1);
            }
            else if (const Mat44fAttribute* ma =
                         dynamic_cast<const Mat44fAttribute*>(a))
            {
                TwkMath::Mat44f M = ma->value();
                writer.propertyData(&M, aname.c_str(), 1);
            }
            else if (const IntAttribute* ia =
                         dynamic_cast<const IntAttribute*>(a))
            {
                int v = ia->value();
                writer.propertyData(&v, aname.c_str(), 1);
            }
            else if (const StringAttribute* sa =
                         dynamic_cast<const StringAttribute*>(a))
            {
                string v = sa->value();
                int id = writer.lookup(v.c_str());
                writer.propertyData(&id, aname.c_str(), 1);
            }
        }

        //
        //  Write the plane data
        //

        for (size_t i = 0; i < planeNames.size(); i++)
        {
            const FrameBuffer* plane = state.planes[i];
            vector<int> channelNames(plane->numChannels());

            for (size_t q = 0; q < plane->numChannels(); q++)
            {
                channelNames[q] = writer.lookup(plane->channelName(q).c_str());
            }

            int orientation = (int)(plane->orientation());

            vector<int> sizes(2);
            sizes[0] = plane->width();
            sizes[1] = plane->height();

            int rawID = writer.lookup("raw");
            int colorID = writer.lookup("color");
            int bitdepth = plane->bytesPerChannel();
            int padding = plane->scanlinePixelPadding();
            int extraScanlines = plane->extraScanlines();
            int dataType = int(plane->dataType());
            float pixelAspect = plane->pixelAspectRatio();

            writer.propertyData(sizes, "size");
            writer.propertyData(&orientation, "orientation");
            writer.propertyData(&rawID, "encoding", 1);
            writer.propertyData(&rawID, "codec", 1);
            writer.propertyData(channelNames, "channels");
            writer.propertyData(&colorID, "role", 1);
            writer.propertyData(&bitdepth, "bitdepth", 1);
            writer.propertyData(&padding, "scanlinePadding", 1);
            writer.propertyData(&extraScanlines, "extraScanlines", 1);
            writer.propertyData(&pixelAspect, "pixelAspectRatio", 1);
            writer.propertyData(&dataType, "dataType", 1);
            writer.propertyData(plane->pixels<void>(), "pixels");
        }
    }

} // namespace TwkFB
