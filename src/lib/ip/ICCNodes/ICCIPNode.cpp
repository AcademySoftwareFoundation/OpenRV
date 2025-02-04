//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <ICCNodes/ICCIPNode.h>
#include <ICCNodes/ColorSyncBridge.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/IPProperty.h>
#include <IPCore/ShaderCommon.h>
#include <TwkUtil/FileStream.h>
#include <TwkUtil/FourCC.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkFB/Operations.h>
#include <TwkApp/Bundle.h>
#include <lcms2.h>
#include <boost/functional/hash.hpp>

extern "C"
{
#include <lcms2_internal.h>
}

namespace IPCore
{
    using namespace std;
    using namespace TwkUtil;
    using namespace boost;

    namespace
    {

        static bool initialized = false;
        static int useOldGLLUTInterp = -1;

        struct RawParaTag
        {
            cmsUInt32Number type;
            cmsUInt32Number reserved;
            cmsUInt16Number funcType;
            cmsUInt16Number reserved2;
            cmsS15Fixed16Number coefficient0;
        };

        struct RawXYZTag
        {
            cmsUInt32Number type;
            cmsUInt32Number reserved;
            cmsS15Fixed16Number x;
            cmsS15Fixed16Number y;
            cmsS15Fixed16Number z;
        };

        struct ParaTag
        {
            string type;
            size_t funcType;
            double gamma;
            vector<double> coefficients;
        };

        struct ParaCoefficients
        {
            TwkMath::Vec3f gamma;
            TwkMath::Vec3f a;
            TwkMath::Vec3f b;
            TwkMath::Vec3f c;
            TwkMath::Vec3f d;
            TwkMath::Vec3f e;
            TwkMath::Vec3f f;
        };

        size_t hashBlock(const void* data, size_t size)
        {
            int sizeOverflow = size % sizeof(size_t); // in bytes
            const size_t* p = reinterpret_cast<const size_t*>(data);
            const size_t* e = reinterpret_cast<const size_t*>(
                reinterpret_cast<const char*>(data) + size - sizeOverflow);
            boost::hash<size_t> h;
            size_t v = 0;
            for (; p < e; p++)
                v ^= h(*p);
            if (sizeOverflow)
            {
                size_t lastOne = 0;
                ;
                memcpy(reinterpret_cast<char*>(&lastOne),
                       reinterpret_cast<const char*>(e), sizeOverflow);
                v ^= h(lastOne);
            }

            return v;
        }

        void parseParametricCurve(char* rawTagData, size_t rawDataSize,
                                  ParaTag& para)
        {
#ifdef TWK_LITTLE_ENDIAN
            swapWords(rawTagData, 2);
            swapShorts(rawTagData + 8, 2);
            size_t ncoeffs = (rawDataSize - 12) / 4;
            swapWords(rawTagData + 12, ncoeffs);
#endif

            RawParaTag* tag = reinterpret_cast<RawParaTag*>(rawTagData);

            para.type = packedFourCCAsString(tag->type);
            para.funcType = tag->funcType;

            for (size_t i = 0; i < ncoeffs; i++)
            {
                double frac = _cms15Fixed16toDouble((&tag->coefficient0)[i]);
                if (i == 0)
                    para.gamma = frac;
                else
                    para.coefficients.push_back(frac);
            }
        }

        IPNode::Vec3 parseXYZValue(char* rawTagData, size_t rawDataSize)
        {
#ifdef TWK_LITTLE_ENDIAN
            swapWords(rawTagData, 2);
            size_t nnums = (rawDataSize - 8) / 4;
            swapWords(rawTagData + 8, nnums);
#endif

            RawXYZTag* tag = reinterpret_cast<RawXYZTag*>(rawTagData);

            double x = _cms15Fixed16toDouble(tag->x);
            double y = _cms15Fixed16toDouble(tag->y);
            double z = _cms15Fixed16toDouble(tag->z);

            return IPNode::Vec3(x, y, z);
        }

        void logErrorHandlerFunction(cmsContext ContextID,
                                     cmsUInt32Number ErrorCode,
                                     const char* Text)
        {
            cout << "ERROR: LCMS: " << Text << " (" << ErrorCode << ")" << endl;
        }

        //
        //  Scarfed off some board
        //

        cmsHPROFILE createLinearRec709ICCProfile()
        {
            //
            //  Make a 1.0 gamma curve - basically we want an identity profile
            //  with Rec709 primaries
            //

            cmsToneCurve* gamma1 = cmsBuildGamma(NULL, 1);
            cmsToneCurve* curve3[3];
            curve3[0] = gamma1;
            curve3[1] = gamma1;
            curve3[2] = gamma1;

            cmsCIExyY white; // = {0.3127, 0.3290, 1.0}; // Rec709
            cmsWhitePointFromTemp(&white, 6504);

            cmsCIExyYTRIPLE chromaticities = {{0.639998686, 0.330010138, 1.0},
                                              {0.300003784, 0.600003357, 1.0},
                                              {0.150002046, 0.059997204, 1.0}};

            cmsHPROFILE p =
                cmsCreateRGBProfile(&white, &chromaticities, curve3);
            cmsFreeToneCurve(gamma1);
            return p;
        }

        IPNode::Vec3 XYZ_to_xyY(const IPNode::Vec3& v)
        {
            cmsCIEXYZ a;
            a.X = v.x;
            a.Y = v.y;
            a.Z = v.z;

            cmsCIExyY b;
            cmsXYZ2xyY(&b, &a);

            return IPNode::Vec3(b.x, b.y, b.Y);
        }

        cmsHPROFILE createSRGBProfileInProfileSpace(ICCIPNode::ProfileState& p)
        {
            cmsCIEXYZ white;
            cmsCIEXYZ red;
            cmsCIEXYZ green;
            cmsCIEXYZ blue;
            cmsCIExyY whiteY;
            cmsCIExyYTRIPLE rgbY;
            cmsHPROFILE hsRGB;
            cmsFloat64Number Parameters[5];
            cmsToneCurve* sRGB[3];

            white.X = p.white.x;
            white.Y = p.white.y;
            white.Z = p.white.z;
            red.X = p.red.x;
            red.Y = p.red.y;
            red.Z = p.red.z;
            green.X = p.green.x;
            green.Y = p.green.y;
            green.Z = p.green.z;
            blue.X = p.blue.x;
            blue.Y = p.blue.y;
            blue.Z = p.blue.z;

            cmsXYZ2xyY(&whiteY, &white);
            cmsXYZ2xyY(&rgbY.Red, &red);
            cmsXYZ2xyY(&rgbY.Green, &green);
            cmsXYZ2xyY(&rgbY.Blue, &blue);

            Parameters[0] = 2.4;
            Parameters[1] = 1. / 1.055;
            Parameters[2] = 0.055 / 1.055;
            Parameters[3] = 1. / 12.92;
            Parameters[4] = 0.04045;

            sRGB[0] = cmsBuildParametricToneCurve(NULL, 4, Parameters);
            sRGB[1] = sRGB[0];
            sRGB[2] = sRGB[0];

            if (sRGB[0] == NULL)
                return NULL;

            hsRGB = cmsCreateRGBProfileTHR(NULL, &whiteY, &rgbY, sRGB);
            cmsFreeToneCurve(sRGB[0]);
            if (hsRGB == NULL)
                return NULL;
            return hsRGB;
        }

        CSProfile createCSProfileFromLCMSProfile(cmsHPROFILE p)
        {
            cmsUInt32Number nbytes;
            bool ok = cmsSaveProfileToMem(p, NULL, &nbytes);

            vector<char> buffer(nbytes);
            ok = cmsSaveProfileToMem(p, &buffer.front(), &nbytes);

            return newColorSyncProfileFromData(&buffer.front(), buffer.size());
        }

        void clearProfileState(ICCIPNode::ProfileState& state)
        {
            if (state.profile)
                cmsCloseProfile(state.profile);
            if (state.data && state.ownsData)
                FileStream::deleteDataPointer(state.data);

            state.ownsData = false;
            state.data = 0;
            state.profile = 0;
            state.dataSize = 0;
            state.dataHash = 0;
        }

        void parseTags(ICCIPNode::ProfileState& state)
        {
            //
            //  Scan the tags to figure out how we're going to represent this
            //  profile on the GPU
            //

            const _cmsICCPROFILE* iccProfile = (_cmsICCPROFILE*)state.profile;

            for (cmsInt32Number i = 0, s = cmsGetTagCount(state.profile); i < s;
                 i++)
            {
                cmsTagSignature sig = cmsGetTagSignature(state.profile, i);
                string sigName = packedFourCCAsString(sig);

                const size_t tagSize = iccProfile->TagSizes[i];
                vector<char> buffer(tagSize);
                char* rawTagData = &buffer.front();
                const size_t nread =
                    cmsReadRawTag(state.profile, sig, rawTagData, tagSize);

                if (sigName == "aarg" || sigName == "aabg" || sigName == "aagg")
                {
                    ParaTag para;
                    parseParametricCurve(rawTagData, nread, para);
                }
                else if (sigName == "wtpt")
                {
                    state.white = parseXYZValue(rawTagData, nread);
                }
                else if (sigName == "rXYZ")
                {
                    state.red = parseXYZValue(rawTagData, nread);
                }
                else if (sigName == "gXYZ")
                {
                    state.green = parseXYZValue(rawTagData, nread);
                }
                else if (sigName == "bXYZ")
                {
                    state.blue = parseXYZValue(rawTagData, nread);
                }
            }
        }

        void createProfileStateFromMemory(ICCIPNode::ProfileState& state,
                                          void* data, size_t size)
        {
            clearProfileState(state);

            state.ownsData = false;
            state.data = data;
            state.dataSize = size;
            state.dataHash = hashBlock(state.data, state.dataSize);
            state.profile = cmsOpenProfileFromMem(state.data, state.dataSize);
            state.csProfile = createCSProfileFromLCMSProfile(state.profile);

            parseTags(state);
        }

        void createProfileStateFromFile(ICCIPNode::ProfileState& state,
                                        const string& filename)
        {
            //
            //  Load the file using FileStream and stash the data. This
            //  ensures we don't have extra open files, etc, consuming
            //  resources we might need for e.g. media
            //

            clearProfileState(state);

            FileStream fstream(filename, FileStream::Buffering, 0, 0, false);

            state.ownsData = true;
            state.data = fstream.data();
            state.dataSize = fstream.size();
            state.dataHash = hashBlock(state.data, state.dataSize);
            state.profile = cmsOpenProfileFromMem(state.data, state.dataSize);
            state.csProfile = newColorSyncProfileFromPath(filename);

            parseTags(state);
        }

        void createLinearProfileState(ICCIPNode::ProfileState& state)
        {
            clearProfileState(state);

            state.ownsData = false;
            state.profile = createLinearRec709ICCProfile();
            state.csProfile = createCSProfileFromLCMSProfile(state.profile);
            state.data = 0;
            state.dataSize = 0;
            state.dataHash = 1;
        }

        void createSRGBProfileState(ICCIPNode::ProfileState& state)
        {
            clearProfileState(state);

            state.ownsData = false;
            state.profile = cmsCreate_sRGBProfile();
            state.csProfile = createCSProfileFromLCMSProfile(state.profile);
            state.data = 0;
            state.dataSize = 0;
            state.dataHash = 2;
        }

        void createSRGBProfileStateInOtherProfileSpace(
            ICCIPNode::ProfileState& state, ICCIPNode::ProfileState& other)
        {
            clearProfileState(state);

            state.ownsData = false;
            state.profile = createSRGBProfileInProfileSpace(other);
            state.csProfile = createCSProfileFromLCMSProfile(state.profile);
            state.data = 0;
            state.dataSize = 0;
            state.dataHash = other.dataHash ^ (size_t(-1));
        }

        void copyProfileToDataProperty(
            ICCIPNode::ProfileState& state,
            IPNode::ByteProperty::container_type& dataVector)
        {
            size_t s = state.dataSize;
            const char* data = reinterpret_cast<const char*>(state.data);
            dataVector.resize(s);
            std::copy(data, data + s, dataVector.begin());
        }

    } // namespace

    //----------------------------------------------------------------------

    ICCIPNode::ICCIPNode(const string& name, const NodeDefinition* def,
                         IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
        , m_active(0)
        , m_inProfile(0)
        , m_inProfileData(0)
        , m_inProfileDesc(0)
        , m_inProfileVersion(0)
        , m_outProfile(0)
        , m_outProfileData(0)
        , m_outProfileDesc(0)
        , m_outProfileVersion(0)
        , m_samples2D(0)
        , m_samples3D(0)
        , m_updating(false)
        , m_transform(0)
        , m_cstransform(0)
        , m_fb(0)
        , m_method(LUT3DMethod)
        , m_mode(TransformMode)
    {
        if (!initialized)
        {
            cmsSetLogErrorHandler(logErrorHandlerFunction);
            initialized = true;
        }

        string type = def->name();

        if (type == "ICCDisplayTransform")
            m_mode = DisplayMode;
        else if (type == "ICCLinearizeTransform")
            m_mode = LinearizeMode;
        else if (type == "ICCTransform")
            m_mode = TransformMode;
        else
            abort();

        PropertyInfo* info = new PropertyInfo(
            PropertyInfo::Persistent | PropertyInfo::RequiresGraphEdit);

        m_active = declareProperty<IntProperty>("node.active", 1);
        m_samples2D = declareProperty<IntProperty>("samples.2d", 256);
        m_samples3D = declareProperty<IntProperty>("samples.3d", 32);

        if (m_mode == DisplayMode || m_mode == TransformMode)
        {
            m_outProfile =
                declareProperty<StringProperty>("outProfile.url", "", info);
            m_outProfileDesc =
                declareProperty<StringProperty>("outProfile.description", "");
            m_outProfileVersion =
                declareProperty<FloatProperty>("outProfile.version", 0.0);
            m_outProfileData =
                declareProperty<ByteProperty>("outProfile.data", info);
        }

        if (m_mode == LinearizeMode || m_mode == TransformMode)
        {
            m_inProfile =
                declareProperty<StringProperty>("inProfile.url", "", info);
            m_inProfileDesc =
                declareProperty<StringProperty>("inProfile.description", "");
            m_inProfileVersion =
                declareProperty<FloatProperty>("inProfile.version", 0.0);
            m_inProfileData =
                declareProperty<ByteProperty>("inProfile.data", info);
        }
    }

    ICCIPNode::~ICCIPNode()
    {
        clearState();
        delete m_fb;
    }

    void ICCIPNode::setInProfile(const std::string& s)
    {
        if (m_inProfile)
        {
            m_inProfileData->valueContainer().clear();
            setProperty(m_inProfile, s);
            updateState();
            IPNode::propertyChanged(m_inProfile);
        }
    }

    void ICCIPNode::setOutProfile(const std::string& s)
    {
        if (m_outProfile)
        {
            m_outProfileData->valueContainer().clear();
            setProperty(m_outProfile, s);
            updateState();
            IPNode::propertyChanged(m_outProfile);
        }
    }

    void ICCIPNode::setInProfileData(const void* data, size_t s)
    {
        if (m_inProfileData)
        {
            m_inProfileData->resize(s);
            memcpy(m_inProfileData->rawData(), data, s);
            setProperty(m_inProfile, "");
            updateState();
            IPNode::propertyChanged(m_inProfileData);
            IPNode::propertyChanged(m_inProfile);
        }
    }

    void ICCIPNode::setOutProfileData(const void* data, size_t s)
    {
        if (m_outProfileData)
        {
            m_outProfileData->resize(s);
            memcpy(m_outProfileData->rawData(), data, s);
            setProperty(m_outProfile, "");
            updateState();
            IPNode::propertyChanged(m_outProfileData);
            IPNode::propertyChanged(m_outProfile);
        }
    }

    string ICCIPNode::inProfile() const
    {
        return propertyValue(m_inProfile, "");
    }

    string ICCIPNode::outProfile() const
    {
        return propertyValue(m_outProfile, "");
    }

    void ICCIPNode::clearState()
    {
        ScopedLock lock(m_stateMutex);
        clearProfileState(m_inState);
        clearProfileState(m_outState);
        if (m_transform)
            cmsDeleteTransform(m_transform);
        m_transform = 0;
    }

    void ICCIPNode::updateProfileMetaData(const ProfileState& state,
                                          StringProperty* desc,
                                          FloatProperty* ver)
    {
        char temp[256];

        if (state.profile)
        {
            cmsGetProfileInfoASCII(state.profile, cmsInfoDescription, "en",
                                   "US", temp, 256);
            setProperty(desc, temp);

            float f = float(cmsGetProfileVersion(state.profile));
            setProperty(ver, f);
        }
        else
        {
            setProperty(desc, "");
            setProperty(ver, 0.0);
        }

        IPNode::propertyChanged(desc);
        IPNode::propertyChanged(ver);
    }

    namespace
    {
    }

    void ICCIPNode::updateState()
    {
        ScopedLock lock(m_stateMutex);
        m_updating = true;

        if (m_transform)
            cmsDeleteTransform(m_transform);
        m_transform = NULL;

        if (m_inProfile)
        {
            if (m_inProfileData && !m_inProfileData->empty())
            {
                createProfileStateFromMemory(m_inState,
                                             m_inProfileData->rawData(),
                                             m_inProfileData->size());
            }
            else if (inProfile() != "")
            {
                createProfileStateFromFile(m_inState, inProfile());
                copyProfileToDataProperty(m_inState,
                                          m_inProfileData->valueContainer());
            }
            else
            {
                clearProfileState(m_inState);

                if (!m_outProfile)
                {
                    m_updating = false;
                    return;
                }
            }

            updateProfileMetaData(m_inState, m_inProfileDesc,
                                  m_inProfileVersion);

            if (!m_outProfile)
                createSRGBProfileState(m_outState);
        }

        if (m_outProfile)
        {
            if (m_outProfileData && !m_outProfileData->empty())
            {
                createProfileStateFromMemory(m_outState,
                                             m_outProfileData->rawData(),
                                             m_outProfileData->size());
            }
            else if (outProfile() != "")
            {
                createProfileStateFromFile(m_outState, outProfile());
                copyProfileToDataProperty(m_outState,
                                          m_outProfileData->valueContainer());
            }
            else
            {
                clearProfileState(m_outState);

                if (!m_inProfile)
                {
                    m_updating = false;
                    return;
                }
            }

            updateProfileMetaData(m_outState, m_outProfileDesc,
                                  m_outProfileVersion);

            if (!m_inProfile)
                createSRGBProfileState(m_inState);
        }

        //
        //  Create the chromaticity matrix
        //  Rec709 -> Display Primaries
        //

        // float inPrimaries[8] =
        // {
        //     0.3127f, 0.3290f, // white
        //     0.6400f, 0.3300f, // red
        //     0.3000f, 0.6000f, // green
        //     0.1500f, 0.0600f  // blue
        // };

        // Vec3 white = XYZ_to_xyY(m_outState.white);
        // Vec3 red   = XYZ_to_xyY(m_outState.red);
        // Vec3 green = XYZ_to_xyY(m_outState.green);
        // Vec3 blue  = XYZ_to_xyY(m_outState.blue);

        // vector<float> outPrimaries(8);
        // outPrimaries[0] = white.x;
        // outPrimaries[1] = white.y;
        // outPrimaries[2] = red.x;
        // outPrimaries[3] = red.y;
        // outPrimaries[4] = green.x;
        // outPrimaries[5] = green.y;
        // outPrimaries[6] = blue.x;
        // outPrimaries[7] = blue.y;

        // TwkFB::colorSpaceConversionMatrix(inPrimaries,
        //                                   &outPrimaries.front(),
        //                                   inPrimaries,
        //                                   &outPrimaries.front(),
        //                                   true,
        //                                   (float*)&m_conversionMatrix);

        //
        //  Create the transform
        //

        const int samples2D = propertyValue(m_samples2D, 256);
        const int samples3D = propertyValue(m_samples3D, 32);

        if (m_inState.profile)
        {
            m_transform = cmsCreateTransform(m_inState.profile, TYPE_RGB_FLT,
                                             m_outState.profile, TYPE_RGB_FLT,
                                             INTENT_PERCEPTUAL, 0);
        }
        else
        {
            m_transform = NULL;
        }

        if (m_cstransform)
            deleteColorSyncTransform(m_cstransform);

        if (m_inState.csProfile)
        {
            m_cstransform = newColorSyncTransformFromProfiles(
                m_inState.csProfile, m_outState.csProfile);
        }
        else
        {
            m_cstransform = NULL;
        }

        if (!m_transform && !m_cstransform)
        {
            delete m_fb;
            m_fb = 0;
            m_updating = false;
            return;
        }

        //
        //  Create the channel LUT
        //

        if (!m_fb)
            m_fb = new FrameBuffer();

        if (m_method == LUT3DMethod)
        {
            m_fb->restructure(samples3D, samples3D, samples3D, 3,
                              FrameBuffer::FLOAT);
        }
        else
        {
            m_fb->restructure(samples2D, 1, 1, 3, FrameBuffer::FLOAT);
        }

        ostringstream str;

        str << "ICC:fb:lut:" << m_method << ":"
            << (m_method == LUT3DMethod ? samples3D : samples2D) << ":" << hex
            << m_inState.dataHash << "|" << inProfile() << "+"
            << m_outState.dataHash << "|" << outProfile();

        const size_t s = m_fb->width();
        const float smax = float(s) - 1;
        m_fb->setIdentifier(str.str());

        vector<float> buffer(m_method == LUT3DMethod ? (s * s * s * 3)
                                                     : (s * 3));

        if (m_method == LUT3DMethod)
        {
            float* data = &buffer.front();
            const size_t s2 = s * s;

            for (size_t z = 0; z < s; z++)
            {
                const float b = float(z) / smax;

                for (size_t y = 0; y < s; y++)
                {
                    const float g = float(y) / smax;

                    for (size_t x = 0; x < s; x++)
                    {
                        const float r = float(x) / smax;

                        size_t i = (x + y * s + z * s2) * 3;
                        data[i + 0] = r;
                        data[i + 1] = g;
                        data[i + 2] = b;
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < buffer.size(); i += 3)
            {
                float v = float(i / 3) / smax;
                buffer[i + 0] = v;
                buffer[i + 1] = v;
                buffer[i + 2] = v;
            }
        }

#if USE_COLORSYNC
        if (m_cstransform)
        {
            convert(m_cstransform, m_method == LUT3DMethod ? s * s : s,
                    m_method == LUT3DMethod ? s : 1, &buffer.front(),
                    m_fb->pixels<float>());
        }
#else
        if (m_transform)
        {
            cmsDoTransform(m_transform, &buffer.front(), m_fb->pixels<void>(),
                           m_method == LUT3DMethod ? s * s * s : s);
        }
#endif

        // float* p = m_fb->pixels<float>();
        // size_t s2 = s / 2;
        // p += s * s * s2 + s * s2 + s2;
        // cout << name() << ":" << this << ": m_transform = " << m_transform
        //      << ", (" << p[0] << ", " << p[1] << ", " << p[2] << ")"
        //      << endl;

        m_updating = false;
    }

    void ICCIPNode::applyTransform(IPImage* root, bool sRGBIn,
                                   bool sRGBOut) const
    {
        if (m_transform)
        {
            FrameBuffer* fb = m_fb->referenceCopy();
            Vec3 outScale(1.0);
            Vec3 outOffset(0.0);

            if (!root->shaderExpr)
                root->shaderExpr = Shader::newSourceRGBA(root);
            // root->shaderExpr = Shader::newColorMatrix(root->shaderExpr,
            // m_conversionMatrix);
            if (sRGBIn)
                root->shaderExpr =
                    Shader::newColorLinearToSRGB(root->shaderExpr);

            if (m_method == LUT3DMethod)
            {
                // XXX temp backdoor in case users have to revert
                if (useOldGLLUTInterp == -1)
                {
                    useOldGLLUTInterp =
                        (getenv("TWK_USE_OLD_GL_LUT_INTERP") == 0) ? 0 : 1;
                }

                if (useOldGLLUTInterp)
                {
                    Vec3 grid = Vec3(1.0 / fb->width(), 1.0 / fb->height(),
                                     1.0 / fb->depth());

                    Vec3 scale = Vec3(Vec3(1.0, 1.0, 1.0) - grid);

                    root->shaderExpr = Shader::newColor3DLUTGLSampling(
                        root->shaderExpr, fb, scale, grid / 2.0f, outScale,
                        outOffset);
                }
                else
                {
                    root->shaderExpr = Shader::newColor3DLUT(
                        root->shaderExpr, fb, outScale, outOffset);
                }
            }
            else
            {
                root->shaderExpr = Shader::newColorChannelLUT(
                    root->shaderExpr, fb, outScale, outOffset);
            }

            if (sRGBOut)
                root->shaderExpr =
                    Shader::newColorSRGBToLinear(root->shaderExpr);
        }

        root->resourceUsage = root->shaderExpr->computeResourceUsageRecursive();
    }

    IPImage* ICCIPNode::evaluate(const Context& context)
    {
        switch (m_mode)
        {
        case DisplayMode:
            return evaluateDisplay(context);
        case LinearizeMode:
            return evaluateLinearize(context);
        case TransformMode:
            return evaluateTransform(context);
        default:
            return IPNode::evaluate(context);
        }
    }

    IPImage* ICCIPNode::evaluateLinearize(const Context& context)
    {
        if (IPImage* root = IPNode::evaluate(context))
        {
            if (!root->isBlank() && !root->isNoImage()
                && propertyValue(m_active, 1) != 0 && m_transform)
            {
                applyTransform(root, false, true);
            }

            return root;
        }
        else
        {
            return IPImage::newNoImage(this, "No Input");
        }
    }

    IPImage* ICCIPNode::evaluateTransform(const Context& context)
    {
        if (IPImage* root = IPNode::evaluate(context))
        {
            if (root->isBlank() || root->isNoImage())
                return root;
            if (propertyValue(m_active, 1) != 0 && m_transform)
                applyTransform(root, false, false);
            return root;
        }
        else
        {
            return IPImage::newNoImage(this, "No Input");
        }
    }

    IPImage* ICCIPNode::evaluateDisplay(const Context& context)
    {
        Context newContext = context;
        IPImage* root = IPNode::evaluate(newContext);

        if (!root)
            return IPImage::newNoImage(this, "No Input");
        if (root->isBlank() || root->isNoImage())
            return root;
        if (propertyValue(m_active, 1) == 0 || !m_transform)
            return root;

        IPImage* nroot =
            new IPImage(this, IPImage::BlendRenderType, context.viewWidth,
                        context.viewHeight, 1.0, IPImage::IntermediateBuffer,
                        IPImage::FloatDataType);

        root->fitToAspect(nroot->displayAspect());
        nroot->appendChild(root);
        nroot->useBackground = true;

        if (m_transform)
            applyTransform(nroot, true, false);
        return nroot;
    }

    void ICCIPNode::propertyChanged(const Property* p)
    {
        if (p == m_inProfile || p == m_outProfile || p == m_inProfileData
            || p == m_outProfileDesc)
        {
            if (!m_updating)
                updateState();
        }
    }

    void ICCIPNode::readCompleted(const std::string&, unsigned int)
    {
        updateState();
    }

    string ICCIPNode::inProfileDescription() const
    {
        return propertyValue(m_inProfileDesc, "");
    }

    string ICCIPNode::outProfileDescription() const
    {
        return propertyValue(m_outProfileDesc, "");
    }

    void ICCIPNode::copyNode(const IPNode* node)
    {
        IPNode::copyNode(node);
        if (node->definition() == definition())
            updateState();
    }

} // namespace IPCore
