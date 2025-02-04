//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPCore/LUTIPNode.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/Application.h>
#include <IPCore/Exception.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/NodeDefinition.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/File.h>
#include <TwkMath/Function.h>
#include <TwkMath/Iostream.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkUtil;
    using namespace TwkMath;

    bool LUTIPNode::newGLSLlutInterp = false;
    std::map<std::string, LUT::LUTData> LUTIPNode::lutLib;

    //
    //  LUTS:
    //
    //  Luminance LUT:          type == "Luminance", dimensions == 1
    //  RGB individual LUTS:    type == "RGBA", dimensions == 1
    //  RGB 3D LUT:             type == "RGBA", dimensions == 3
    //

    LUTIPNode::LUTIPNode(const std::string& name, const NodeDefinition* def,
                         IPGraph* g, GroupIPNode* group)
        : IPNode(name, def, g, group)
        , m_lutfb(0)
        , m_lowlut3(0)
        , m_lowlut1(0)
        , m_prelut(0)
        , m_floatOutLUT(false)
        , m_usePowerOf2(true)
        , m_lutInMatrix(0)
        , m_lutOutMatrix(0)
        , m_lutLUT(0)
        , m_lutPreLUT(0)
        , m_lutScale(0)
        , m_lutOffset(0)
        , m_lutGamma(0)
        , m_lutType(0)
        , m_lutName(0)
        , m_lutFile(0)
        , m_lutSize(0)
        , m_lutActive(0)
        , m_lutOutputSize(0)
        , m_lutOutputType(0)
        , m_lutOutputLUT(0)
        , m_lutOutputPreLUT(0)
        , m_lutPreLUTSize(0)
    {
        setMaxInputs(1);

        m_lutfb = 0;

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_lutInMatrix = createProperty<Mat44fProperty>("lut.inMatrix");
        m_lutInMatrix->resize(1);

        m_lutOutMatrix = createProperty<Mat44fProperty>("lut.outMatrix");
        m_lutOutMatrix->resize(1);

        m_lutLUT = declareProperty<FloatProperty>("lut.lut");
        m_lutPreLUT = declareProperty<FloatProperty>("lut.prelut");
        m_lutScale = declareProperty<FloatProperty>("lut.scale", 1.0);
        m_lutOffset = declareProperty<FloatProperty>("lut.offset", 0.0);
        m_lutGamma =
            declareProperty<FloatProperty>("lut.conditioningGamma", 1.0);
        m_lutType = declareProperty<StringProperty>("lut.type", "Luminance");
        m_lutName = declareProperty<StringProperty>("lut.name", "");
        m_lutFile = declareProperty<StringProperty>("lut.file", "");
        m_lutSize = createProperty<IntProperty>("lut.size");
        m_lutSize->resize(3);
        (*m_lutSize)[0] = 0;
        (*m_lutSize)[1] = 0;
        (*m_lutSize)[2] = 0;

        m_lutActive = declareProperty<IntProperty>("lut.active", 0);
        m_lutOutputSize =
            declareProperty<IntProperty>("lut:output.size", 256, info);
        m_lutOutputType = declareProperty<StringProperty>("lut:output.type",
                                                          "Luminance", info);

        m_lutOutputLUT = declareProperty<FloatProperty>("lut:output.lut", info);
        m_lutOutputPreLUT =
            declareProperty<FloatProperty>("lut:output.prelut", info);

        m_matrixOutputRGBA =
            createProperty<Mat44fProperty>("matrix:output.RGBA");
        m_matrixOutputRGBA->resize(1);
        m_matrixOutputRGBA->setInfo(info);

        int preLUTSize = def->intValue("defaults.preLUTSize", 2048);
        m_lutPreLUTSize =
            declareProperty<IntProperty>("lut.preLUTSize", preLUTSize, info);
    }

    LUTIPNode::~LUTIPNode()
    {
        delete m_lutfb;
        delete[] m_lowlut3;
        delete[] m_lowlut1;
        // don't delete m_prelut its a plane of m_lutfb if it exists
    }

    void LUTIPNode::copyNode(const IPNode* node)
    {
        IPNode::copy(node);
        if (node->definition() == definition())
            updateProperties();
    }

    void LUTIPNode::propertyChanged(const Property* property)
    {
        IPNode::propertyChanged(property);

        if (property == m_lutFile || property == m_lutPreLUTSize)
        {
            updateProperties();
        }

        if (property == m_lutLUT || property == m_lutPreLUT
            || property == m_lutSize || property == m_lutInMatrix
            || property == m_lutOutMatrix)
        {
            generateLUT();
        }
    }

    void LUTIPNode::readCompleted(const std::string& type, unsigned int version)
    {
        IPNode::readCompleted(type, version);

        if (Mat44fProperty* m = property<Mat44fProperty>("lut", "matrix"))
        {
            if (m->size() >= 1)
            {
                Mat44fProperty* m2 = m_lutInMatrix;
                m2->front() = m->front();
                removeProperty(m);
            }
        }

        updateProperties();
    }

    void LUTIPNode::updateProperties()
    {
        LockGuard lock(m_updateMutex);

        string file = propertyValue<StringProperty>(m_lutFile, "");
        if (file != "")
        {
            string filename = pathConform(Application::mapFromVar(file));
            if (!fileExists(filename.c_str()))
            {
                if (const char* path = getenv("RV_LUT_PATH"))
                {
                    vector<string> names = findInPath(filename, path);
                    if (names.size())
                        filename = names[0];
                }
            }
            if (!fileExists(filename.c_str()))
            {
                TWK_THROW_STREAM(ReadFailedExc,
                                 "Unable to read lut: '" << file << "'");
            }

            setProperty<StringProperty>(m_lutFile, filename);
            LUT::LUTData* lutdata = parseLUT(filename);

            m_lutInMatrix->front() = lutdata->inMatrix;

            setProperty<FloatProperty>(m_lutGamma, lutdata->conditioningGamma);

            m_lutLUT->resize(lutdata->data.size());
            std::copy(lutdata->data.begin(), lutdata->data.end(),
                      m_lutLUT->begin());

            m_lutPreLUT->resize(lutdata->prelutData.size());
            std::copy(lutdata->prelutData.begin(), lutdata->prelutData.end(),
                      m_lutPreLUT->begin());

            m_lutSize->resize(lutdata->dimensions.size());
            std::copy(lutdata->dimensions.begin(), lutdata->dimensions.end(),
                      m_lutSize->begin());

            if (m_lutSize->size() == 1)
            {
                m_lutSize->resize(3);
                (*m_lutSize)[1] = 0;
                (*m_lutSize)[2] = 0;
            }

            setProperty<StringProperty>(m_lutType, "RGB");
        }

        generateLUT();
    }

    LUT::LUTData* LUTIPNode::parseLUT(string filename)
    {
        LUT::LUTData* lutdata(0);

        //
        //  Let the caller deal with exceptions thrown from readFile
        //

        try
        {
            bool newLUT = true;

            if (getenv("RV_NO_LUT_REUSE"))
                lutdata = new LUT::LUTData();
            else
            {
                newLUT = (lutLib.count(filename) == 0);
                lutdata = &lutLib[filename];
            }

            if (newLUT)
            {
                LUT::readFile(filename, *lutdata);
                if (m_usePowerOf2)
                    LUT::resamplePowerOfTwo(*lutdata);
                bool linear = LUT::simplifyPreLUT(*lutdata);
                LUT::compilePreLUT(*lutdata, m_lutPreLUTSize->front());

                if (lutdata->dimensions.size() == 1 && !linear)
                {
                    lutLib.erase(filename);
                    TWK_THROW_EXC_STREAM(
                        "can't handle non-linear prelut with a channel lut. "
                        "Try baking the non-linear portion of the prelut "
                        "into the channel lut");
                }
            }
            else
            {
                static string lastMsg;
                ostringstream msg;

                msg << "INFO: re-using LUT data from '" << filename << "'"
                    << endl;
                if (msg.str() != lastMsg)
                {
                    lastMsg = msg.str();
                    cerr << lastMsg;
                }
            }
        }
        catch (TwkExc::Exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
            throw;
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
            throw;
        }
        catch (...)
        {
            cerr << "ERROR: uncaught exception in parseLUT(), LUT '" << filename
                 << "'" << endl;
            throw;
        }

        return lutdata;
    }

    bool LUTIPNode::lutActive() const
    {
        return (propertyValue<IntProperty>(m_lutActive, 0) == 1);
    }

    inline float lerp1DLUT(FloatProperty* lut, float range, size_t c, float v)
    {
        const float n = lut->size() / 3 - 1;
        const size_t i0 = size_t(v * n);
        const size_t i1 = size_t(i0 >= n ? n : i0 + 1);
        const float d = v * n - float(i0);

        const float v0 = (*lut)[i0 * 3 + c];
        const float v1 = (*lut)[i1 * 3 + c];

        return lerp(v0, v1, d);
    }

    void LUTIPNode::generateLUT()
    {
        IntProperty* sizes = m_lutSize;
        sizes->resize(3);
        int xs = (*sizes)[0];
        int ys = (*sizes)[1];
        int zs = (*sizes)[2];

        if (xs && ys && zs)
        {
            generate3DLUT();
        }
        else if (xs)
        {
            generate1DLUT();
        }
    }

    void LUTIPNode::generate3DLUT()
    {
        delete m_lutfb;
        m_lutfb = 0;

        IntProperty* outSize = m_lutOutputSize;
        Property* outProp = m_lutOutputLUT;
        FloatProperty* out32 = dynamic_cast<FloatProperty*>(outProp);
        HalfProperty* out16 = dynamic_cast<HalfProperty*>(outProp);
        FloatProperty* lut = m_lutLUT;
        FloatProperty* prelut = m_lutPreLUT;
        FloatProperty* scale = m_lutScale;
        FloatProperty* offset = m_lutOffset;
        IntProperty* sizes = m_lutSize;
        Mat44fProperty* inMatrix = m_lutInMatrix;
        Mat44fProperty* outMatrix = m_lutOutMatrix;
        StringProperty* fileP = m_lutFile;

        size_t n = 1;
        n *= (*sizes)[0];
        n *= (*sizes)[1];
        n *= (*sizes)[2];
        outSize->copy(sizes);

        if (3 * n != lut->size())
        {
            EvaluationFailedExc exc;
            exc << "3D LUT size mismatch " << (n * 3) << " != " << lut->size();
            if (fileP && fileP->size())
                exc << " (" << fileP->front() << ")";
            cerr << exc << endl;
            throw exc;
        }

        outProp->resize(lut->size());

        float maxv = -numeric_limits<float>::max();
        float minv = -maxv;

        unsigned int h = (*sizes)[0] << 16 | (*sizes)[1] << 8 | (*sizes)[2];
        for (int i = 0, s = lut->size(); i < s; i++)
        {
            float c = (*lut)[i];
            if (c > maxv)
                maxv = c;
            if (c < minv)
                minv = c;

            if (out32)
                (*out32)[i] = c;
            else
                (*out16)[i] = c;
            h += i * int(1000 * c);
        }

        float rangev = maxv - minv;

        FrameBuffer::DataType dataType =
            out32 ? FrameBuffer::FLOAT : FrameBuffer::HALF;

        if (!ImageRenderer::hasFloatFormats() && !m_floatOutLUT)
        {
            dataType = FrameBuffer::USHORT;
            if (m_lowlut3)
                delete[] m_lowlut3;
            m_lowlut3 = new unsigned short[outProp->size()];

            scale->front() = rangev;
            offset->front() = minv;

            for (int i = 0, s = outProp->size(); i < s; i++)
            {
                if (out32)
                {
                    m_lowlut3[i] =
                        (unsigned short)(((*out32)[i] - minv) / rangev
                                         * double(numeric_limits<
                                                  unsigned short>::max()));
                }
                else
                {
                    m_lowlut3[i] =
                        (unsigned short)(((*out16)[i] - minv) / rangev
                                         * double(numeric_limits<
                                                  unsigned short>::max()));
                }

                h += i * m_lowlut3[i];
            }
        }
        else
        {
            if (m_lowlut3)
                delete m_lowlut3;
            m_lowlut3 = 0;
            scale->front() = 1.0;
            offset->front() = 0.0;
        }

        m_lutfb =
            new FrameBuffer(FrameBuffer::NormalizedCoordinates, (*sizes)[0],
                            (*sizes)[1], (*sizes)[2], 3, dataType,
                            m_lowlut3 ? (unsigned char*)m_lowlut3
                                      : (unsigned char*)outProp->rawData(),
                            0, FrameBuffer::BOTTOMLEFT, false);

        m_lutfb->attribute<float>("scale") = scale->front();
        m_lutfb->attribute<float>("offset") = offset->front();

        m_lutfb->idstream() << h;

        if (fileP)
            m_lutfb->idstream() << ":" << fileP->front();

        if (inMatrix)
        {
            m_lutfb->attribute<Mat44f>("inMatrix") = inMatrix->front();
        }

        if (outMatrix)
        {
            m_lutfb->attribute<Mat44f>("outMatrix") = outMatrix->front();
        }

        if (prelut && !prelut->empty())
        {
            generate1DLUT(true);
            if (m_prelut)
                m_lutfb->appendPlane(m_prelut);
        }
    }

    void LUTIPNode::generate1DLUT(bool asprelut)
    {
        if (!asprelut)
        {
            delete m_lutfb;
            m_lutfb = 0;
        }

        IntProperty* outSize = m_lutOutputSize;
        Property* outProp = asprelut ? m_lutOutputPreLUT : m_lutOutputLUT;
        FloatProperty* out32 = dynamic_cast<FloatProperty*>(outProp);
        HalfProperty* out16 = dynamic_cast<HalfProperty*>(outProp);
        FloatProperty* lut = asprelut ? m_lutPreLUT : m_lutLUT;
        FloatProperty* scale = m_lutScale;
        FloatProperty* offset = m_lutOffset;
        Mat44fProperty* inMatrix = m_lutInMatrix;
        Mat44fProperty* outMatrix = m_lutOutMatrix;

        if (lut->empty() && asprelut)
            return;

        size_t n = lut->size() / 3;
        const float n2 = lut->size() - 1;
        unsigned int h = n;

        if (!outSize->size())
        {
            outSize->resize(3);
            (*outSize)[0] = n;
            (*outSize)[1] = 0;
            (*outSize)[2] = 0;
        }

        outProp->resize(n * 3);

        float maxv = -numeric_limits<float>::max();
        float minv = -maxv;

        for (int i = 0; i < n; i++)
        {
            const double cindex = double(i) / double(n - 1);
            Vec3f c = Vec3f(cindex);

            c.x = lerp1DLUT(lut, 1.0, 0, c.x);
            c.y = lerp1DLUT(lut, 1.0, 1, c.y);
            c.z = lerp1DLUT(lut, 1.0, 2, c.z);

            if (c.x > maxv)
                maxv = c.x;
            if (c.y > maxv)
                maxv = c.y;
            if (c.z > maxv)
                maxv = c.z;
            if (c.x < minv)
                minv = c.x;
            if (c.y < minv)
                minv = c.y;
            if (c.z < minv)
                minv = c.z;

            if (out32)
            {
                (*out32)[i * 3 + 0] = c.x;
                (*out32)[i * 3 + 1] = c.y;
                (*out32)[i * 3 + 2] = c.z;
            }
            else if (out16)
            {
                (*out16)[i * 3 + 0] = c.x;
                (*out16)[i * 3 + 1] = c.y;
                (*out16)[i * 3 + 2] = c.z;
            }

            h ^= char(c.x * 255) | char(c.y * 255) << 8 | char(c.z * 255) << 16;
            h = h << 8 | h >> 24;
        }

        float rangev = maxv - minv;

        FrameBuffer::DataType dataType =
            out32 ? FrameBuffer::FLOAT : FrameBuffer::HALF;
        typedef unsigned short IType;

        if (!ImageRenderer::hasFloatFormats() && !m_floatOutLUT)
        {
            const IType smax = numeric_limits<IType>::max();
            const double dmax = smax;

            scale->front() = rangev;
            offset->front() = minv;

            dataType = FrameBuffer::USHORT;
            if (m_lowlut1)
                delete[] m_lowlut1;
            m_lowlut1 = new IType[outProp->size()];

            for (int i = 0, s = outProp->size(); i < s; i++)
            {
                double f;

                if (out32)
                    f = ((*out32)[i] - minv) / double(rangev);
                else
                    f = ((*out16)[i] - minv) / double(rangev);

                const IType it = IType(f > 1.0 ? dmax : f * dmax);

                // cout << " " << f;
                // if (i % 3 == 2) cout << endl;

                m_lowlut1[i] = it;
                h ^= m_lowlut1[i];
                h = h << 8 | h >> 24;
            }
        }
        else
        {
            if (m_lowlut1)
                delete m_lowlut1;
            m_lowlut1 = 0;
            scale->front() = 1.0;
            offset->front() = 0.0;
        }

        FrameBuffer* fb =
            new FrameBuffer(n, 1, 3, dataType,
                            m_lowlut1 ? (unsigned char*)m_lowlut1
                                      : (unsigned char*)outProp->rawData(),
                            0, FrameBuffer::NATURAL, false);

        fb->attribute<float>("scale") = scale->front();
        fb->attribute<float>("offset") = offset->front();
        fb->idstream() << h << ":" << size_t(m_lutfb);

        if (inMatrix)
        {
            fb->attribute<Mat44f>("inMatrix") = inMatrix->front();
        }

        if (outMatrix)
        {
            fb->attribute<Mat44f>("outMatrix") = outMatrix->front();
        }

        if (asprelut)
        {
            m_prelut = fb;
        }
        else
        {
            m_lutfb = fb;
        }
    }

    TwkFB::FrameBuffer* LUTIPNode::evaluateLUT(const Context& context)
    {
        if (lutActive() && m_lutfb)
        {
            return m_lutfb->referenceCopy();
        }

        return 0;
    }

    void LUTIPNode::prepareForWrite()
    {
        m_lutdata.data.resize(0);
        m_lutdata.prelutData.resize(0);

        if (StringProperty* sp = m_lutFile)
        {
            if (sp->size() && sp->front() != "")
            {
                sp->front() = Application::mapToVar(sp->front());

                FloatProperty* lut = m_lutLUT;
                FloatProperty* prelut = m_lutPreLUT;

                m_lutdata.data.resize(lut->size());
                m_lutdata.prelutData.resize(prelut->size());

                std::copy(lut->begin(), lut->end(), m_lutdata.data.begin());

                std::copy(prelut->begin(), prelut->end(),
                          m_lutdata.prelutData.begin());

                lut->resize(0);
                prelut->resize(0);
            }
        }
    }

    void LUTIPNode::writeCompleted()
    {
        if (m_lutdata.data.size() || m_lutdata.prelutData.size())
        {
            StringProperty* sp = m_lutFile;

            sp->front() = Application::mapFromVar(sp->front());

            FloatProperty* lut = m_lutLUT;
            FloatProperty* prelut = m_lutPreLUT;

            lut->resize(m_lutdata.data.size());
            prelut->resize(m_lutdata.prelutData.size());

            std::copy(m_lutdata.data.begin(), m_lutdata.data.end(),
                      lut->begin());

            std::copy(m_lutdata.prelutData.begin(), m_lutdata.prelutData.end(),
                      prelut->begin());

            m_lutdata.data.resize(0);
            m_lutdata.prelutData.resize(0);
        }
    }

    void LUTIPNode::addLUTPipeline(const Context& context, IPImage* img)
    {
        FrameBuffer* LUT = evaluateLUT(context);
        if (!LUT)
            return;

        if (!img->shaderExpr)
        {
            if (img->node)
            {
                cerr << "ERROR: cannot add LUT pipeline, image has no shader: "
                     << img->node->name() << endl;
            }
            return;
        }

        FrameBuffer* preLUT = LUT->nextPlane();
        if (preLUT)
        {
            if (m_lutGamma->front() != 1.0)
            {
                Vec3f gammaVec(m_lutGamma->front());
                img->shaderExpr =
                    Shader::newColorGamma(img->shaderExpr, gammaVec);
            }

            LUT->removePlane(preLUT);
            preLUT->idstream() << LUT->identifier() << ":preLUT";
        }

        if (LUT->hasAttribute("inMatrix"))
        {
            img->shaderExpr = Shader::newColorMatrix(
                img->shaderExpr, LUT->attribute<Mat44f>("inMatrix"));
        }

        if (LUT->depth() > 1)
        {
            if (preLUT)
            {
                Vec3f outScale(1.0f);
                Vec3f outOffset(0.0f);
                if (preLUT->hasAttribute("scale"))
                    outScale = Vec3f(preLUT->attribute<float>("scale"));
                if (preLUT->hasAttribute("offset"))
                    outOffset = Vec3f(preLUT->attribute<float>("offset"));

                img->shaderExpr = Shader::newColorChannelLUT(
                    img->shaderExpr, preLUT, outScale, outOffset);
            }

            Vec3f grid = Vec3f(1.0 / LUT->width(), 1.0 / LUT->height(),
                               1.0 / LUT->depth());

            Vec3f scale = Vec3f(Vec3f(1.0, 1.0, 1.0) - grid);

            Vec3f outScale(1.0f);
            Vec3f outOffset(0.0f);

            if (LUT->hasAttribute("scale"))
                outScale = Vec3f(LUT->attribute<float>("scale"));
            if (LUT->hasAttribute("offset"))
                outOffset = Vec3f(LUT->attribute<float>("offset"));

            if (newGLSLlutInterp)
            {
                img->shaderExpr = Shader::newColor3DLUT(img->shaderExpr, LUT,
                                                        outScale, outOffset);
            }
            else
            {
                img->shaderExpr = Shader::newColor3DLUTGLSampling(
                    img->shaderExpr, LUT, scale, grid / 2.0f, outScale,
                    outOffset);
            }
        }
        else
        {
            Vec3f outScale(1.0f);
            Vec3f outOffset(0.0f);
            if (LUT->hasAttribute("scale"))
                outScale = Vec3f(LUT->attribute<float>("scale"));
            if (LUT->hasAttribute("offset"))
                outOffset = Vec3f(LUT->attribute<float>("offset"));

            img->shaderExpr = Shader::newColorChannelLUT(img->shaderExpr, LUT,
                                                         outScale, outOffset);
        }

        if (LUT->hasAttribute("outMatrix"))
        {
            img->shaderExpr = Shader::newColorMatrix(
                img->shaderExpr, LUT->attribute<Mat44f>("outMatrix"));
        }
    }

    IPImage* LUTIPNode::evaluate(const Context& context)
    {
        IPImage* head = IPNode::evaluate(context);

        if (!head)
            return IPImage::newNoImage(this, "No Input");

        //
        //  If input to this node is a blend, prepare img for shaderExpr mods,
        //  etc.
        //
        convertBlendRenderTypeToIntermediate(head);

        addLUTPipeline(context, head);

        return head;
    }

} // namespace IPCore
