//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __ICCNodes__ICCIPNode__h__
#define __ICCNodes__ICCIPNode__h__
#include <iostream>
#include <boost/thread.hpp>
#include <IPCore/IPNode.h>

namespace TwkFB
{
    class FrameBuffer;
}

namespace IPCore
{

    //
    //  class ICCIPNode
    //
    //  Implements an ICC profile transform. Uses lcms2
    //

    class ICCIPNode : public IPNode
    {
    public:
        typedef boost::mutex Mutex;
        typedef boost::mutex::scoped_lock ScopedLock;
        typedef TwkFB::FrameBuffer FrameBuffer;

        enum Mode
        {
            DisplayMode,
            LinearizeMode,
            TransformMode
        };

        enum Method
        {
            ChannelLUTMethod,
            LUT3DMethod,
            ExpressionMethod
        };

        struct ProfileState
        {
            ProfileState()
                : data(0)
                , profile(0)
                , csProfile(0)
                , ownsData(false)
                , dataSize(0)
                , dataHash(0)
                , white(0.0f)
                , red(0.0f)
                , green(0.0f)
                , blue(0.0f)
            {
            }

            void* data;
            void* profile;
            void* csProfile;
            bool ownsData;
            size_t dataSize;
            size_t dataHash;
            Vec3 white;
            Vec3 red;
            Vec3 green;
            Vec3 blue;
        };

        //
        //  Constructors
        //

        ICCIPNode(const std::string& name, const NodeDefinition* def,
                  IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ICCIPNode();

        virtual IPImage* evaluate(const Context& context);
        virtual IPImage* evaluateDisplay(const Context& context);
        virtual IPImage* evaluateLinearize(const Context& context);
        virtual IPImage* evaluateTransform(const Context& context);
        virtual void propertyChanged(const Property*);
        virtual void readCompleted(const std::string& type,
                                   unsigned int version);

        void setInProfile(const std::string&);
        void setOutProfile(const std::string&);
        void setInProfileData(const void*, size_t);
        void setOutProfileData(const void*, size_t);
        std::string inProfile() const;
        std::string outProfile() const;

        std::string inProfileDescription() const;
        std::string outProfileDescription() const;

        virtual void copyNode(const IPNode*);

    private:
        void clearState();
        void updateState();
        void updateProfileMetaData(const ProfileState&, StringProperty*,
                                   FloatProperty*);
        void applyTransform(IPImage*, bool, bool) const;

    private:
        IntProperty* m_active;

        StringProperty* m_inProfile;
        ByteProperty* m_inProfileData;
        StringProperty* m_inProfileDesc;
        FloatProperty* m_inProfileVersion;
        ProfileState m_inState;

        StringProperty* m_outProfile;
        ByteProperty* m_outProfileData;
        StringProperty* m_outProfileDesc;
        FloatProperty* m_outProfileVersion;
        ProfileState m_outState;

        IntProperty* m_samples2D;
        IntProperty* m_samples3D;
        Mutex m_stateMutex;
        bool m_updating;
        void* m_transform;
        void* m_cstransform;
        FrameBuffer* m_fb;
        Method m_method;
        Mode m_mode;
        Matrix m_conversionMatrix;
    };

} // namespace IPCore

#endif // __ICCNodes__ICCIPNode__h__
