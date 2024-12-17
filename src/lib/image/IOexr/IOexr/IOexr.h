//******************************************************************************
// Copyright (c) 2005-2013 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOexr__IOexr__h__
#define __IOexr__IOexr__h__
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/StreamingIO.h>
#include <TwkFB/IO.h>
#include <ImfMultiPartInputFile.h>
#include <ImfChannelList.h>
#include <map>
#include <string>
#include <set>

#ifndef NDEBUG
#ifndef DEBUG_IOEXR
#define DEBUG_IOEXR
#endif
#endif

namespace TwkFB
{

    class IOexr : public StreamingFrameBufferIO
    {
    public:
        //
        //  Types
        //

        struct ChannelGroup
        {
            std::string view;
            std::string layer;
            size_t width;
            size_t height;
            std::vector<std::string> channelNames;
            FrameBuffer::DataType dataType;
        };

        enum ReadWindow
        {
            DataWindow = 0,
            DisplayWindow = 1,
            UnionWindow = 2,
            DataInsideDisplayWindow = 3
        };

        enum WriteMethod
        {
            MultiViewWriter = 0,
            MultiPartWriter = 1
        };

        typedef std::set<std::string> LayerNames;
        typedef std::vector<std::string> ViewNames;

        //
        //  Constuctors
        //
        //  rgbaOnly will force the use of the RGBA interface
        //
        //  convertYRYBY will force IOexr to convert the image to RGB
        //  before returning it.
        //
        //  planar3channel will return separate planes for R G B when the
        //  image is three channel. This makes it possible to get around
        //  the Nvidia 3 channel bug for rectangular textures.
        //
        //  stripAlpha will strip the Alpha channel for speed
        //

        IOexr(bool rgbaOnly = false, bool convertYRYBY = true,
              bool planar3channel = false, bool inheritChannels = false,
              bool noOneChannelPlanes = true, bool stripAlpha = false,
              bool readWindowIsDisplayWindow = false,
              ReadWindow window = DisplayWindow,
              WriteMethod writeMethod = MultiViewWriter,
              IOType type = StandardIO, size_t chunkSize = 61440,
              int maxAsync = 16);

        virtual ~IOexr();

        //
        //  Conversion on read is the default. If you want to handle it
        //  yourself you have to tell the plugin. Likewise for planar
        //  storage.
        //

        void setRGBAOnly(bool);
        void convertYRYBY(bool);
        void planar3Channel(bool);
        void noOneChannelPlanes(bool);
        void stripAlpha(bool);
        void inheritChannels(bool);
        void setReadWindow(ReadWindow);
        void setWriteMethod(WriteMethod);

        //
        //  IO API
        //

        virtual void readImages(FrameBufferVector& fbs,
                                const std::string& filename,
                                const ReadRequest& request) const;

        virtual void writeImages(const ConstFrameBufferVector& fbs,
                                 const std::string& filename,
                                 const WriteRequest& request) const;

        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

        virtual bool getBoolAttribute(const std::string& name) const;
        virtual void setBoolAttribute(const std::string& name, bool value);
        virtual int getIntAttribute(const std::string& name) const;
        virtual void setIntAttribute(const std::string& name, int value);

    private:
        void readImagesFromFile(Imf::MultiPartInputFile&,
                                FrameBufferVector& fbs,
                                const std::string& filename,
                                const ReadRequest& request) const;

        void readImagesFromMultiPartFile(Imf::MultiPartInputFile& file,
                                         FrameBufferVector& fbs,
                                         const std::string& filename,
                                         const std::string& requestedView,
                                         const std::string& requestedLayer,
                                         const std::string& requestedChannel,
                                         const bool requestedAllChannels) const;

        void readImagesFromMultiViewFile(
            Imf::MultiPartInputFile& file, FrameBufferVector& fbs,
            const std::string& filename, const std::string& requestedView,
            const std::string& requestedLayer,
            const std::string& requestedBaseChannel,
            const bool requestedAllChannels, const int partNum,
            const ViewNames& views, bool requestedViewIsDefaultView) const;

        void writeImagesToMultiPartFile(const ConstFrameBufferVector& fbs,
                                        const std::string& filename,
                                        const WriteRequest& request) const;

        void writeImagesToMultiViewFile(const ConstFrameBufferVector& fbs,
                                        const std::string& filename,
                                        const WriteRequest& request) const;

        void getMultiPartImageInfo(const Imf::MultiPartInputFile& file,
                                   FBInfo& fbi) const;

        void getMultiViewImageInfo(const Imf::MultiPartInputFile& file,
                                   const ViewNames& views, FBInfo& fbi) const;

        void planarConfig(FrameBuffer&, int, int, int, int, int, int, int, int,
                          FrameBuffer::DataType, const char* C0name,
                          const char* C1name, const char* C2name,
                          const char* C3name = 0) const;

    private:
        typedef struct
        {
            int partNumber;
            std::string partName;
            std::string fullname;
            std::string name;
            Imf::Channel channel;
        } MultiPartChannel;

        static void addToMultiPartChannelList(
            std::vector<MultiPartChannel>& rcl, const int partNumber,
            const std::string& partName, const std::string& channelName,
            const Imf::Channel& channel);

        static bool isAMultiPartSharedAttribute(const std::string& name);

        static bool isAces(const Imf::Chromaticities& c);
        static const Imf::Chromaticities& acesChromaticities();

        static int
        indexOfChannelName(const std::vector<std::string>& channelNames,
                           const char* names[], bool exact = false);
        static int
        indexOfChannelName(const std::vector<MultiPartChannel>& channelsMP,
                           const char* names[], bool exact = false);

        static int alphaIndex(const std::vector<std::string>& channelNames);
        static int alphaIndex(const std::vector<MultiPartChannel>& channelsMP);

        static int redIndex(const std::vector<std::string>& channelNames);
        static int redIndex(const std::vector<MultiPartChannel>& channelsMP);

        static int greenIndex(const std::vector<std::string>& channelNames);
        static int greenIndex(const std::vector<MultiPartChannel>& channelsMP);

        static int blueIndex(const std::vector<std::string>& channelNames);
        static int blueIndex(const std::vector<MultiPartChannel>& channelsMP);

        static bool channelIsRGB(const std::string& channelName);

        static std::string findAnAlpha(const Imf::MultiPartInputFile& file,
                                       int partNum, const std::string& layer,
                                       const std::string& view);

        static std::string fullChannelName(const MultiPartChannel& mpChannel);

        static void findAnAlphaInView(const Imf::MultiPartInputFile& file,
                                      const std::string& view,
                                      const std::string& partName,
                                      MultiPartChannel& alpha);

        static std::string baseChannelName(const std::string& name);

        static void channelSplit(const std::string& name, std::string& base,
                                 std::string& layerView);

        static void canonicalName(std::string& a);

        static Imf::ChannelList::Iterator
        findChannelWithBasename(const std::string& cname, Imf::ChannelList& cl);

        static std::vector<MultiPartChannel>::const_iterator
        findChannelWithBasenameMP(
            const std::string& cname,
            const std::vector<MultiPartChannel>& mpChannelList);

        static bool LayerOrNamedViewChannel(const std::string& c);
        static bool LayerOrNamedViewChannelMP(const MultiPartChannel& mp);

        static int channelOrder(std::string& s);

        static bool ChannelComp(const std::string& ia, const std::string& ib);
        static bool ChannelCompMP(const MultiPartChannel& ia,
                                  const MultiPartChannel& ib);

        static size_t
        channelListIteratorDifference(Imf::ChannelList::ConstIterator a,
                                      const Imf::ChannelList::ConstIterator& b);

        static size_t channelListSize(const Imf::ChannelList& cl);

        static void setChannelInfo(const Imf::ChannelList::ConstIterator& i,
                                   FBInfo::ChannelInfo& info);

        static void setChannelInfo(const std::string& name, Imf::PixelType type,
                                   FBInfo::ChannelInfo& info);

        static void readAllAttributes(const Imf::MultiPartInputFile& file,
                                      FrameBuffer& fb,
                                      const std::set<int>& parts);

        static void readAllAttributes(const Imf::MultiPartInputFile& file,
                                      FrameBuffer& fb);

        static bool
        doesRequestedLayerExists(bool viewHasChannelConflict,
                                 const std::string& requestedView,
                                 const std::string& requestedLayer,
                                 const std::string& partNameInFile,
                                 const std::string& channelNameInFile);

        static bool
        doesRequestedChannelExists(bool viewHasChannelConflict,
                                   const std::string& requestedView,
                                   const std::string& requestedLayer,
                                   const std::string& requestedChannel,
                                   const std::string& partNameInFile,
                                   const std::string& channelNameInFile);

        static void readMultiViewChannelList(
            const std::string& filename, const std::string& layer,
            const std::string& view, FrameBuffer& fb,
            Imf::MultiPartInputFile& file, int partNum, Imf::ChannelList& cl,
            bool useRGBAReader, bool convertYRYBY, bool planar3channel,
            bool allChannels, bool inheritChannels, bool noOneChannelPlanes,
            bool stripAlpha, bool readWindowIsDisplayWindow,
            IOexr::ReadWindow window);

        static void
        getBiggerFrameBufferAndEXRPixelType(const Imf::Channel& channel,
                                            FrameBuffer::DataType& fbDataType,
                                            Imf::PixelType& exrPixelType);

        static void readMultiPartChannelList(
            const std::string& filename, const std::string& view,
            FrameBuffer& fb, Imf::MultiPartInputFile& file,
            std::vector<MultiPartChannel>& channelsRead, bool convertYRYBY,
            bool planar3channel, bool allChannels, bool inheritChannels,
            bool noOneChannelPlanes, bool stripAlpha,
            bool readWindowIsDisplayWindow, IOexr::ReadWindow window);

        static bool stripViewFromName(std::string& name,
                                      const std::string& view);

        static std::string viewStrippedPartQualifiedName(
            bool doStrip, const Imf::MultiPartInputFile& file, int partnum,
            const std::string& view, const std::string& name);

    private:
        bool m_convertYRYBY;
        bool m_planar3channel;
        bool m_rgbaOnly;
        bool m_inheritChannels;
        bool m_noOneChannelPlanes;
        bool m_stripAlpha;
        bool m_readWindowIsDisplayWindow;
        ReadWindow m_readWindow;
        WriteMethod m_writeMethod;
    };

} // namespace TwkFB

#endif // __IOexr__IOexr__h__
