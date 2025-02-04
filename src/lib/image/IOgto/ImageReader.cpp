//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IOgto/ImageReader.h>
#include <Gto/Utilities.h>
#include <half.h>

namespace TwkFB
{
    using namespace std;

    ImageReader::ImageReader(FrameBufferVector& fbs, istream& instream,
                             const string& filename, bool nopixels)
        : Gto::Reader(Gto::Reader::None)
        , m_fbs(fbs)
        , m_inAttributes(false)
        , m_inGeometry(false)
        , m_nopixels(nopixels)
    {
        open(instream, filename.c_str(), Gto::Reader::None);
        copyAttrs();
    }

    ImageReader::ImageReader(FrameBufferVector& fbs, void* data, size_t size,
                             const string& filename, bool nopixels)
        : Gto::Reader(Gto::Reader::None)
        , m_fbs(fbs)
        , m_inAttributes(false)
        , m_inGeometry(false)
        , m_nopixels(nopixels)
    {
        open((char*)data, size, filename.c_str());
        copyAttrs();
    }

    ImageReader::~ImageReader() {}

    void ImageReader::copyAttrs()
    {
        for (size_t i = 0; i < m_images.size(); i++)
        {
            Image& image = m_images[i];
            bool ok = false;

            for (size_t q = 0; q < image.attributes.size(); q++)
            {
                FBAttribute* a = image.attributes[q];
                // why does this cause a crash?
                if (a->name() != "View")
                    image.fb->addAttribute(a);
                // image.fb->addAttribute(a);
            }
        }
    }

    ImageReader::Request ImageReader::object(const std::string& name,
                                             const std::string& protocol,
                                             unsigned int version,
                                             const ObjectInfo& info)
    {
        m_inAttributes = false;
        m_inGeometry = false;
        m_inPlane = false;

        if (protocol == "image")
        {
            m_images.resize(m_images.size() + 1);
            Image& image = m_images.back();

            while (m_fbs.size() < m_images.size())
                m_fbs.push_back(new FrameBuffer());

            image.fb = m_fbs[m_images.size() - 1];
            image.viewName = name;

            return Request(true, (void*)(m_images.size() - 1));
        }
        else
        {
            return Request(false);
        }
    }

    ImageReader::Request ImageReader::component(const std::string& n,
                                                const std::string& i,
                                                const ComponentInfo& info)
    {
        Image& image = m_images[size_t(info.object->objectData)];
        FrameBuffer* fb = image.fb;
        string compInterp = stringFromId(info.interpretation);

        if (compInterp == "plane")
        {
            image.planes.resize(image.planes.size() + 1);
            Plane& plane = image.planes.back();
            FrameBuffer* pfb = fb;

            for (size_t i = 0; i < image.planes.size(); i++)
            {
                if (i == 0)
                {
                    pfb = pfb->firstPlane();
                }
                else if (!pfb->nextPlane())
                {
                    FrameBuffer* nfb = new FrameBuffer();
                    pfb->appendPlane(nfb);
                    pfb = nfb;
                }
                else
                {
                    pfb = pfb->nextPlane();
                }
            }

            plane.fb = pfb;
            return Request(true, (void*)(image.planes.size() - 1));
        }

        return Request(compInterp == "attributes" || compInterp == "geometry",
                       fb);
    }

    ImageReader::Request ImageReader::property(const std::string& inname,
                                               const std::string& interp,
                                               const PropertyInfo& info)
    {
        const char* name = inname.c_str();
        string compInterp = stringFromId(info.component->interpretation);
        FBAttribute* attr = 0;

        m_inAttributes = compInterp == "attributes";
        m_inGeometry = compInterp == "geometry";
        m_inPlane = compInterp == "plane";
        m_propName = name;
        m_propInterp = interp;

        if (m_propInterp == "pixels" && m_nopixels)
        {
            //
            //  This is a shortcut for getImageInfo(). All the rest of the
            //  image/plane structs are filled in.
            //

            return Request(false);
        }

        if (m_inAttributes)
        {
            Image& image = m_images[size_t(info.component->object->objectData)];
            FrameBuffer* fb = image.fb;

            if (FBAttribute* a = fb->findAttribute(name))
            {
                fb->deleteAttribute(a);
            }

            if (info.size == 1)
            {
                if (info.dims.x == 1)
                {
                    switch (info.type)
                    {
                    case Gto::Int:
                        attr = new TypedFBAttribute<int>(name, 0);
                        break;
                    case Gto::Float:
                        attr = new TypedFBAttribute<float>(name, 0.0f);
                        break;
                    case Gto::Double:
                        attr = new TypedFBAttribute<double>(name, 0.0);
                        break;
                    case Gto::Half:
                        attr = new TypedFBAttribute<half>(name, 0.0f);
                        break;
                    case Gto::Byte:
                        attr = new TypedFBAttribute<unsigned char>(name, 0);
                        break;
                    case Gto::Boolean:
                        attr = new TypedFBAttribute<bool>(name, false);
                        break;
                    case Gto::Short:
                        attr = new TypedFBAttribute<short>(name, 0);
                        break;
                    case Gto::String:
                        attr = new TypedFBAttribute<string>(name, string(""));
                        break;
                    default:
                        return Request(false);
                    }
                }
                else if (info.dims.x == 2)
                {
                    switch (info.type)
                    {
                    case Gto::Float:
                        attr = new TypedFBAttribute<TwkMath::Vec2f>(
                            name, TwkMath::Vec2f(0.0f));
                        break;
                    default:
                        return Request(false);
                    }
                }
                else if (info.dims.x == 16)
                {
                    switch (info.type)
                    {
                    case Gto::Float:
                        attr = new TypedFBAttribute<TwkMath::Mat44f>(
                            name, TwkMath::Mat44f());
                        break;
                    default:
                        return Request(false);
                    }
                }
            }

            if (attr)
                image.attributes.push_back(attr);
        }

        return Request(m_inGeometry || m_inPlane || m_inAttributes, attr);
    }

    bool ImageReader::isSingleValueInt(const PropertyInfo& info)
    {
        return info.size == 1 && info.dims.x == 1 && info.dims.y == 0
               && info.type == Gto::Int;
    }

    bool ImageReader::isSingleValueFloat(const PropertyInfo& info)
    {
        return info.size == 1 && info.dims.x == 1 && info.dims.y == 0
               && info.type == Gto::Float;
    }

    bool ImageReader::isSingleValueString(const PropertyInfo& info)
    {
        return info.size == 1 && info.dims.x == 1 && info.dims.y == 0
               && info.type == Gto::String;
    }

    void* ImageReader::prepIntVector(IntVector& v, const PropertyInfo& info)
    {
        v.resize(info.size * info.dims.x);
        if (v.empty())
            return 0;
        else
            return &v.front();
    }

    void* ImageReader::data(const PropertyInfo& info, size_t bytes)
    {
        Image& image = m_images[size_t(info.component->object->objectData)];
        string compInterp = stringFromId(info.component->interpretation);
        m_propName = stringFromId(info.name);
        m_propInterp = stringFromId(info.interpretation);
        m_inAttributes = compInterp == "attributes";
        m_inGeometry = compInterp == "geometry";
        m_inPlane = compInterp == "plane";

        if (m_inAttributes)
        {
            FBAttribute* a = reinterpret_cast<FBAttribute*>(info.propertyData);

            if (a)
            {
                if (StringAttribute* sa = dynamic_cast<StringAttribute*>(a))
                {
                    return prepIntVector(m_stringBuffer, info);
                }
                else
                {
                    return a->data();
                }
            }
        }
        else if (m_inGeometry)
        {
            if (info.type == Gto::Int)
            {
                if (m_propName == "size")
                    return prepIntVector(image.imageSizes, info);
                else if (m_propName == "dataWindowOrigin")
                    return prepIntVector(image.dataOrigin, info);
                else if (m_propName == "dataWindowSize")
                    return prepIntVector(image.dataSize, info);
                return 0;
            }
            else if (m_propName == "pixelAspectRatio"
                     && isSingleValueFloat(info))
            {
                return &image.pixelAspect;
            }
            else if (m_propName == "planes" && info.type == Gto::String)
            {
                return prepIntVector(m_stringBuffer, info);
            }
        }
        else if (m_inPlane)
        {
            Plane& plane = image.planes[size_t(info.component->componentData)];
            FrameBuffer* fb = plane.fb;

            if (m_propName == "pixels" && m_propInterp == "pixels")
            {
                //
                //  Better have the size and dataType by now -- otherwise
                //  we need to scarf the data in a copy which will slow
                //  things down significantly.
                //

                size_t w = plane.planeSizes[0];
                size_t h = plane.planeSizes[1];

                fb->restructure(w, h, 0, plane.channels.size(),
                                (FrameBuffer::DataType)plane.dataType, NULL,
                                &plane.channels,
                                (FrameBuffer::Orientation)plane.orientation,
                                true, plane.extraScanlines,
                                plane.scanlinePadded);

                assert(info.size * info.dims.x * Gto::dataSizeInBytes(info.type)
                       <= fb->allocSize());

                return fb->pixels<void>();
            }
            else if (m_propName == "size" && info.type == Gto::Int)
            {
                return prepIntVector(plane.planeSizes, info);
            }
            else if (isSingleValueInt(info))
            {
                if (m_propName == "orientation")
                    return &plane.orientation;
                else if (m_propName == "bitdepth")
                    return &plane.bitdepth;
                else if (m_propName == "scanlinePadding")
                    return &plane.scanlinePadded;
                else if (m_propName == "extraScanlines")
                    return &plane.extraScanlines;
                else if (m_propName == "dataType")
                    return &plane.dataType;
            }
            else if (isSingleValueFloat(info))
            {
                if (m_propName == "pixelAspectRatio")
                    return &plane.pixelAspect;
            }
            else if (info.type == Gto::String)
            {
                return prepIntVector(m_stringBuffer, info);
            }
        }

        return 0;
    }

    void ImageReader::dataRead(const PropertyInfo& info)
    {
        Image& image = m_images[size_t(info.component->object->objectData)];
        string compInterp = stringFromId(info.component->interpretation);

        if (m_inAttributes)
        {
            FBAttribute* a = (FBAttribute*)info.propertyData;

            if (StringAttribute* sa = dynamic_cast<StringAttribute*>(a))
            {
                vector<string> strings;

                for (size_t i = 0; i < m_stringBuffer.size(); i++)
                {
                    strings.push_back(stringFromId(m_stringBuffer[i]));
                }

                if (!strings.empty())
                {
                    string s = strings.front();
                    sa->value() = s;
                }
            }

            // ... string vector too
            //
            //  the rest of the POD attrs were handled automatically
            //
        }
        else if (m_inGeometry)
        {
            if (info.type == Gto::Int)
            {
                FrameBuffer* fb = image.fb;

                if (m_propName == "size")
                {
                }
                if (m_propName == "dataWindowOrigin" && info.type == Gto::Int
                    && info.size == 2 && info.dims.x == 1 && info.dims.y == 0)
                {
                    fb->setUncrop(fb->uncropWidth(), fb->uncropHeight(),
                                  image.dataOrigin[0], image.dataOrigin[1]);
                }
                else if (m_propName == "dataWindowSize" && info.type == Gto::Int
                         && info.size == 2 && info.dims.x == 1
                         && info.dims.y == 0)
                {
                    fb->setUncrop(image.dataSize[0], image.dataSize[1],
                                  fb->uncropX(), fb->uncropY());
                }
                else if (m_propName == "pixelAspectRatio"
                         && isSingleValueFloat(info))
                {
                    fb->setPixelAspectRatio(image.pixelAspect);
                }
            }
            else if (m_propName == "planes" && info.type == Gto::String)
            {
                image.planeNames.resize(m_stringBuffer.size());

                for (size_t i = 0; i < m_stringBuffer.size(); i++)
                {
                    image.planeNames[i] = stringFromId(m_stringBuffer[i]);
                }
            }
        }
        else if (m_inPlane)
        {
            Plane& plane = image.planes[size_t(info.component->componentData)];
            FrameBuffer* fb = plane.fb;

            if (m_propName == "channels" && info.type == Gto::String
                && info.dims.x == 1 && info.dims.y == 0)
            {
                plane.channels.resize(m_stringBuffer.size());

                for (size_t i = 0; i < m_stringBuffer.size(); i++)
                {
                    plane.channels[i] = stringFromId(m_stringBuffer[i]);
                }
            }
            else if (isSingleValueString(info))
            {
                if (m_propName == "encoding")
                    plane.encoding = stringFromId(m_stringBuffer[0]);
                else if (m_propName == "codec")
                    plane.codec = stringFromId(m_stringBuffer[0]);
                else if (m_propName == "role")
                    plane.role = stringFromId(m_stringBuffer[0]);
            }
            else if (isSingleValueFloat(info))
            {
                if (m_propName == "pixelAspectRatio")
                {
                    fb->setPixelAspectRatio(plane.pixelAspect);
                }
            }
        }
    }

    void ImageReader::descriptionComplete() {}

} // namespace TwkFB
