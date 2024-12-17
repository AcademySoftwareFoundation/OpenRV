//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__ImageSourceIPNode__h__
#define __IPGraph__ImageSourceIPNode__h__
#include <iostream>
#include <pthread.h>
#include <IPBaseNodes/SourceIPNode.h>

namespace IPCore
{

    ///
    ///  An ImageSourceIPNode holds one ore more image layers
    ///  "in-state". In other words, it has a bunch of properties which
    ///  define image geometry as well as the actual pixels. These are
    ///  stored in the file like any other property.
    ///

    class ImageSourceIPNode : public SourceIPNode
    {
    public:
        //
        //  Types
        //

        typedef TwkContainer::Property Property;
        typedef TwkContainer::FloatProperty FloatProperty;
        typedef TwkContainer::IntProperty IntProperty;
        typedef TwkContainer::StringProperty StringProperty;
        typedef TwkContainer::ByteProperty ByteProperty;
        typedef TwkContainer::HalfProperty HalfProperty;
        typedef TwkContainer::ShortProperty ShortProperty;
        typedef TwkContainer::Component Component;
        typedef std::map<const Property*, size_t> PropModIDMap;
        typedef std::vector<StringVector> LayerTree;
        typedef std::set<std::string> NameSet;

        ImageSourceIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph*, GroupIPNode* group = 0,
                          const std::string mediaRepName = "");

        virtual ~ImageSourceIPNode();

        void set(const std::string& mediaName, const MovieInfo& mediaInfo);

        //
        //  Create or get the pixel property for the frame. The default
        //  view and layer will be used if none is specified.
        //

        Property* findCreatePixels(int frame, const std::string& view = "-",
                                   const std::string& layer = "-");

        //
        //  IPNode API
        //

        virtual ImageRangeInfo imageRangeInfo() const;
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;
        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void propertyChanged(const Property*);
        virtual void flushAllCaches(const FlushContext&);

        //
        //  SourceIPNode API
        //

        virtual size_t numMedia() const;
        virtual size_t mediaIndex(const std::string& name) const;
        virtual const MovieInfo& mediaMovieInfo(size_t index) const;
        virtual const std::string& mediaName(size_t index) const;

        //
        //  Configure the image state information
        //

        void setFrameRange(int fs, int fe);

        //
        //  Pixel Data. The x, y are relative to the literal image data
        //  not any uncrop region.
        //

        void insertPixels(const std::string& view, const std::string& layer,
                          int frame, int x, int y, int w, int h,
                          const void* pixels, size_t size);

        virtual void readCompleted(const std::string&, unsigned int);
        virtual void writeCompleted();

    private:
        std::string defaultView() const;
        std::string defaultLayer() const;

        void pixelPropNamesViewLayer(const StringVector& suffixes1,
                                     const StringVector& suffixes2,
                                     const std::string& sep1,
                                     const std::string& sep2, int frame,
                                     LayerTree& propNames, bool existingOnly);

        void pixelPropNames(const StringVector& views,
                            const StringVector& layers, int frame,
                            LayerTree& propNames, bool existingOnly);

        Property* makePixels(const std::string& comp, const std::string& prop);

        void updateViewLayerInfo();
        void updateInfoFromProps();

    private:
        MovieInfo m_mediaInfo;
        StringProperty* m_mediaName;
        StringProperty* m_mediaMovie;
        StringProperty* m_mediaLocation;
        IntProperty* m_width;
        IntProperty* m_height;
        IntProperty* m_uncropWidth;
        IntProperty* m_uncropHeight;
        IntProperty* m_uncropX;
        IntProperty* m_uncropY;
        FloatProperty* m_pixelAspect;
        IntProperty* m_start;
        IntProperty* m_end;
        IntProperty* m_cutIn;
        IntProperty* m_cutOut;
        FloatProperty* m_fps;
        IntProperty* m_inc;
        StringProperty* m_encoding;
        StringProperty* m_channels;
        IntProperty* m_bits;
        IntProperty* m_float;
        StringProperty* m_defaultView;
        StringProperty* m_defaultLayer;
        StringProperty* m_layers;
        StringProperty* m_views;

        PropModIDMap m_propMap;
    };

} // namespace IPCore

#endif // __IPGraph__ImageSourceIPNode__h__
