//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__LUTIPNode__h__
#define __IPCore__LUTIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkFB/FrameBuffer.h>
#include <boost/thread.hpp>
#include <LUT/ReadLUT.h>

namespace IPCore
{

    /// Provides 3D and Channel LUTs for DisplayIPNode and ColorIPNode

    ///
    /// LUTIPNode two LUTs -- a 3D LUT and a channel LUT.
    ///

    class LUTIPNode : public IPNode
    {
    public:
        typedef boost::mutex Mutex;
        typedef boost::lock_guard<Mutex> LockGuard;

        //
        //  Constructors
        //

        LUTIPNode(const std::string& name, const NodeDefinition* def,
                  IPGraph* graph, GroupIPNode* group = 0);

        void lutinit();
        virtual ~LUTIPNode();

        virtual void generateLUT();

        bool lutActive() const;

        static bool defaultIntLUTs;
        static bool newGLSLlutInterp;
        static std::map<std::string, LUT::LUTData> lutLib;
        static int compiledPreLUTSize;

        virtual void copyNode(const IPNode* node);
        virtual void propertyChanged(const Property* property);
        virtual void prepareForWrite();
        virtual void writeCompleted();
        virtual void readCompleted(const std::string&, unsigned int);
        virtual IPImage* evaluate(const Context&);

    protected:
        void generate3DLUT();
        void generate1DLUT(bool asprelut = false);
        TwkFB::FrameBuffer* evaluateLUT(const Context&);
        void addLUTPipeline(const Context&, IPImage*);

    protected:
        FrameBuffer* m_lutfb;
        FrameBuffer* m_prelut;
        unsigned short* m_lowlut3;
        unsigned short* m_lowlut1;
        bool m_useHalfLUTProp;
        bool m_floatOutLUT;
        bool m_usePowerOf2;
        LUT::LUTData m_lutdata;

    protected:
        Mat44fProperty* m_lutInMatrix;
        Mat44fProperty* m_lutOutMatrix;
        Mat44fProperty* m_matrixOutputRGBA;
        FloatProperty* m_lutLUT;
        FloatProperty* m_lutPreLUT;
        FloatProperty* m_lutScale;
        FloatProperty* m_lutOffset;
        FloatProperty* m_lutGamma;
        StringProperty* m_lutType;
        StringProperty* m_lutName;
        StringProperty* m_lutFile;
        IntProperty* m_lutPreLUTSize;
        IntProperty* m_lutSize;
        IntProperty* m_lutActive;
        IntProperty* m_lutOutputSize;
        StringProperty* m_lutOutputType;
        FloatProperty* m_lutOutputLUT;
        FloatProperty* m_lutOutputPreLUT;

    private:
        void updateProperties();
        LUT::LUTData* parseLUT(std::string filename);

        Mutex m_updateMutex;
    };

} // namespace IPCore

#endif // __IPCore__LUTIPNode__h__
