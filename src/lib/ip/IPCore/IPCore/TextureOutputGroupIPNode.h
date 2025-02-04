//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__TextureOutputGroupIPNode__h__
#define __IPCore__TextureOutputGroupIPNode__h__
#include <IPCore/DisplayGroupIPNode.h>
#include <iostream>

namespace IPCore
{

    ///
    /// class TextureOutputGroupIPNode
    ///
    /// A DisplayGroup which generates a texture for use by the UI. This
    /// can be used to render thumbnails, previews, etc.
    ///
    /// The resulting texture is owned by the CompositeRenderer
    ///

    class TextureOutputGroupIPNode : public DisplayGroupIPNode
    {
    public:
        TextureOutputGroupIPNode(const std::string& name,
                                 const NodeDefinition* def, IPGraph* graph,
                                 GroupIPNode* group = 0);

        virtual ~TextureOutputGroupIPNode();

        void setActive(bool);
        void setTag(const std::string&);
        void setFrame(int);
        void setGeometry(int w, int h, const std::string& datatype);
        void setFlip(bool);
        void setFlop(bool);

        int width() const { return m_width->front(); }

        int height() const { return m_height->front(); }

        const std::string& tag() const { return m_tag->front(); }

        int frame() const { return m_outFrame->front(); }

        bool isActive() const;
        bool cached() const;

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);

        void initContext(Context&) const;

    private:
        IntProperty* m_active;
        IntProperty* m_ndcCoords;
        IntProperty* m_width;
        IntProperty* m_height;
        IntProperty* m_outFrame;
        IntProperty* m_flip;
        IntProperty* m_flop;
        StringProperty* m_dataType;
        StringProperty* m_tag;
        FloatProperty* m_pixelAspect;
    };

} // namespace IPCore

#endif // __IPCore__TextureOutputGroupIPNode__h__
