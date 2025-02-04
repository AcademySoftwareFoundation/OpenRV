//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IOgto__ImageReader__h__
#define __IOgto__ImageReader__h__
#include <iostream>
#include <Gto/Reader.h>
#include <TwkFB/FrameBuffer.h>

namespace TwkFB
{

    //
    //  ImageReader
    //
    //  This finds all "image" objects in a GTO file and converts them into
    //  TwkFB::FrameBuffers on demand.
    //
    //  looks for objects of protocol "image". The name of the image is its
    //  view name. So stereo would be two image objects.
    //
    //  Each image object has a "geometry" component:
    //
    //  these are all in image.attributes:
    //
    //  int[1][]      size                          dimension and sizes of image
    //  window int[1][]      dataWindowOrigin              data window int[1][]
    //  dataWindowSize                data window float[1][]    PixelAspectRatio
    //  pixel aspect string[1][]   planes                        plane names
    //  (e.g. plane0, plane1, ...)
    //
    //  int[1][]      image.plane0.size            must match dim of window
    //  string[1]     image.plane0.encoding        "none" could be "packed10",
    //  etc string[1]     image.plane0.codec           "raw", "jpeg", whatever
    //  we support string[1][]   image.plane0.channels        channel names
    //  string[1][1]  image.plane0.role            "color" is part of viewable
    //  int[1][]      image.plane0.bitdepth        8, 10, 12, etc
    //  float[1][]    image.plane0.pixelAspectRatio pixel aspect
    //  float[1][]    image.plane0.dataType        pixel aspect
    //  byte[N][]     image.plane0.pixels          could also be float, half,
    //  short
    //
    //
    //  planes are stored as independent components
    //

    class ImageReader : public Gto::Reader
    {
    public:
        //
        //  Types
        //

        typedef Gto::Reader::Request Request;
        typedef Gto::Reader::ObjectInfo ObjectInfo;
        typedef Gto::Reader::ComponentInfo ComponentInfo;
        typedef Gto::Reader::PropertyInfo PropertyInfo;
        typedef Gto::uint32 uint32;
        typedef std::vector<FrameBuffer*> FrameBufferVector;
        typedef std::vector<int> IntVector;
        typedef std::vector<std::string> StringVector;
        typedef FrameBuffer::AttributeVector AttributeVector;

        struct Plane
        {
            Plane()
                : fb(0)
                , orientation(0)
                , bitdepth(0)
                , scanlinePadded(0)
                , extraScanlines(0)
                , dataType(0)
                , pixelAspect(0.0f)
            {
            }

            FrameBuffer* fb;
            IntVector planeSizes;
            StringVector channels;
            std::string encoding;
            std::string codec;
            std::string role;
            float pixelAspect;
            int orientation;
            int bitdepth;
            int scanlinePadded;
            int extraScanlines;
            int dataType;
        };

        typedef std::vector<Plane> PlaneVector;

        struct Image
        {
            Image()
                : fb(0)
                , pixelAspect(0.0f)
            {
            }

            FrameBuffer* fb;
            AttributeVector attributes;
            std::string viewName;
            IntVector imageSizes;
            IntVector dataOrigin;
            IntVector dataSize;
            StringVector planeNames;
            PlaneVector planes;
            float pixelAspect;
        };

        typedef std::vector<Image> ImageVector;

        //
        //  Constructor(s). NOTE: calls open() immediately. Don't call it
        //  yourself.
        //

        ImageReader(FrameBufferVector& fbs, std::istream& instream,
                    const std::string& infilename, bool nopixels = false);

        ImageReader(FrameBufferVector& fbs, void* data, size_t size,
                    const std::string& infilename, bool nopixels = false);

        virtual ~ImageReader();

        //
        //  Gto::Reader API
        //

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

        virtual void descriptionComplete();

        void* data(const PropertyInfo& info, size_t bytes);
        void dataRead(const PropertyInfo& info);

        const ImageVector& images() const { return m_images; }

    private:
        void copyAttrs();
        bool isSingleValueInt(const PropertyInfo&);
        bool isSingleValueFloat(const PropertyInfo&);
        bool isSingleValueString(const PropertyInfo&);
        void* prepIntVector(IntVector&, const PropertyInfo&);

    private:
        FrameBufferVector& m_fbs;
        bool m_nopixels;
        IntVector m_stringBuffer;
        bool m_inAttributes;
        bool m_inGeometry;
        bool m_inPlane;
        std::string m_propName;
        std::string m_propInterp;
        ImageVector m_images;
        PlaneVector m_planes;
    };

} // namespace TwkFB

#endif // __IOgto__ImageReader__h__
