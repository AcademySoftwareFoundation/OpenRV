//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__SourceIPNode__h__
#define __IPGraph__SourceIPNode__h__
#include <IPCore/IPNode.h>

namespace IPCore
{

    ///
    ///  A Source node is a leaf in the graph -- it does not have image
    ///  inputs.
    ///

    class SourceIPNode : public IPNode
    {
    public:
        //
        //  Constructors
        //

        SourceIPNode(const std::string& name, const NodeDefinition* def,
                     IPGraph* graph, GroupIPNode* group = 0,
                     const std::string mediaRepName = "",
                     const bool mediaActive = true);

        virtual ~SourceIPNode();

        //
        //  IPNode API
        //

        virtual IPImage* evaluate(const Context&);
        virtual void prepareForWrite();
        virtual void writeCompleted();
        virtual void readCompleted(const std::string&, unsigned int);

        //
        //  Source Media API. You can find out almost anything about the
        //  underlying media using this API. The MovieInfo structure
        //  contains proxy fb (for attributes), a list of views and
        //  layers, etc.
        //
        //  NOTE: mediaInfo's implementation calls mediaName() and
        //  mediaMovieInfo() to get its data.
        //

        virtual void mediaInfo(const Context&, MediaInfoVector&) const;
        virtual const std::string& mediaRepName() const;
        virtual bool isMediaActive() const;
        virtual void setMediaActive(bool state);

        virtual size_t numMedia() const = 0;
        virtual size_t mediaIndex(const std::string& name) const = 0;
        virtual const MovieInfo& mediaMovieInfo(size_t index) const = 0;
        virtual const std::string& mediaName(size_t index) const = 0;

        //
        //  Utilities for use by derived sources
        //

        void addUserAttributes(TwkFB::FrameBuffer*);
        std::string idFromAttributes();
        ImageComponent selectComponentFromContext(const Context&) const;
        ImageComponent stereoComponent(const ImageComponent&, size_t eye) const;
        std::string eyeView(size_t) const;

        const std::string updateAction()
        {
            std::string action = m_updateAction;
            m_updateAction = "modified";
            return action;
        };

        virtual void onNewMediaComplete() {}

    protected:
        void updateStereoViews(const std::vector<std::string>& views,
                               StringProperty* viewsP) const;

        StringProperty* m_imageComponent;
        StringProperty* m_eyeViews;
        std::string m_updateAction;
        StringProperty* m_mediaRepName;
        IntProperty* m_mediaActive;
    };

} // namespace IPCore

#endif // __IPGraph__SourceIPNode__h__
