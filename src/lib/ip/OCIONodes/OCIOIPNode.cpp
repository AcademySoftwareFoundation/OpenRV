
//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <OCIONodes/OCIOIPNode.h>
#include <OCIONodes/ConfigIOProxy.h>
#include <OCIONodes/OCIO1DLUT.h>
#include <OCIONodes/OCIO3DLUT.h>
#include <IPCore/SessionIPNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/DispTransform2DIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/ShaderCommon.h>
#include <TwkExc/Exception.h>
#include <TwkUtil/EnvVar.h>
#include <TwkMath/MatrixColor.h>

#include <boost/functional/hash.hpp>

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <regex>

namespace IPCore
{
    using namespace std;
    using namespace boost;
    using namespace TwkMath;

    // In the eventualy of an RV user wanting to get the exact same results
    // as in RV 2024 and earlier versions of RV, the following environment
    // variable can be defined to tell OCIO to use the legacy GPU processor
    // implementation (OCIO::getOptimizedLegacyGPUProcessor()):
    static ENVVAR_BOOL(evOCIOUseLegacyGPUProcessor,
                       "RV_OCIO_USE_LEGACY_GPU_PROCESSOR", false);

    // Note: This env var is only taken into account when using the legacy GPU
    // processor implementation (see above)
    static ENVVAR_INT(evOCIOLegacyLut3DSize, "RV_OCIO_3D_LUT_SIZE", 32);

    struct OCIOState
    {
        OCIO::ConstConfigRcPtr config;
        OCIO::ContextRcPtr context;
        string display;
        string view;
        string linear;
        string shaderID;
        Shader::Function* function;

        OCIOState()
            : function(0)
        {
        }
    };

    namespace
    {
#define GPU_LANGUAGE_UNKNOWN OCIO::GpuLanguage::GPU_LANGUAGE_CG
        OCIO::GpuLanguage GPULanguage = GPU_LANGUAGE_UNKNOWN;
    } // namespace

    string OCIOIPNode::stringProp(const string& name,
                                  const string& defaultValue) const
    {
        if (const StringProperty* p = property<StringProperty>(name))
        {
            if (!p->empty() && p->front() != "")
            {
                return p->front();
            }
        }

        return defaultValue;
    }

    int OCIOIPNode::intProp(const string& name, int defaultValue) const
    {
        if (const IntProperty* p = property<IntProperty>(name))
        {
            if (!p->empty())
            {
                return p->front();
            }
        }

        return defaultValue;
    }

    OCIOIPNode::OCIOIPNode(const string& name, const NodeDefinition* def,
                           IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
        , m_useRawConfig(false)
    {
        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        string func = def->stringValue("defaults.function", "color");
        declareProperty<StringProperty>("ocio.function", func);

        m_activeProperty = declareProperty<IntProperty>("ocio.active", 1);

        declareProperty<FloatProperty>("ocio.lut", info);
        declareProperty<IntProperty>("ocio.lut3DSize",
                                     evOCIOLegacyLut3DSize.getValue());
        declareProperty<StringProperty>("ocio.inColorSpace", "");

        declareProperty<StringProperty>("ocio_color.outColorSpace", "");

        declareProperty<StringProperty>("ocio_look.look", "");
        declareProperty<IntProperty>("ocio_look.direction", 0);
        declareProperty<StringProperty>("ocio_look.outColorSpace", "");

        declareProperty<StringProperty>("ocio_display.display", "");
        declareProperty<StringProperty>("ocio_display.view", "");

        m_dither = declareProperty<IntProperty>("color.dither", 0);
        m_channelOrder = declareProperty<StringProperty>("color.channelOrder", "RGBA");
        m_channelFlood = declareProperty<IntProperty>("color.channelFlood", 0);

        if (func == "synlinearize")
        {
            m_inTransformURL =
                declareProperty<StringProperty>("inTransform.url", "", info);
            m_inTransformData =
                declareProperty<ByteProperty>("inTransform.data", info);
            m_useRawConfig = true;
        }

        if (func == "syndisplay")
        {
            m_outTransformURL =
                declareProperty<StringProperty>("outTransform.url", "", info);
            m_useRawConfig = true;
        }

        //
        //  Read-only properties to report config info to the user
        //
        m_configDescription =
            declareProperty<StringProperty>("config.description", "");
        m_configWorkingDir =
            declareProperty<StringProperty>("config.workingDir", "");

        m_state = new OCIOState;

        if (!getenv("OCIO_LOGGING_LEVEL"))
        {
            OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);
        }

        if (GPULanguage == GPU_LANGUAGE_UNKNOWN)
        {
            IPNode* session = graph->sessionNode();
            int major =
                session->property<IntProperty>("opengl.glsl.majorVersion")
                    ->front();
            int minor =
                session->property<IntProperty>("opengl.glsl.minorVersion")
                    ->front();

            if (major == 1 && minor < 30)
            {
                // Note: 1.2 is currently the lowest available value in OCIO
                GPULanguage = OCIO::GPU_LANGUAGE_GLSL_1_2;
            }
            else
            {
                GPULanguage = OCIO::GPU_LANGUAGE_GLSL_1_3;
            }
        }

        updateConfig();
    }

    OCIOIPNode::~OCIOIPNode()
    {
        if (m_state->function)
        {
            m_state->function->retire();
        }
        delete m_state;
    }

    void OCIOIPNode::updateConfig()
    {
        try
        {
            if (useRawConfig())
            {
                m_state->config = OCIO::Config::CreateFromConfigIOProxy(
                    std::make_shared<ConfigIOProxy>(this));
            }
            else
            {
                m_state->config = OCIO::GetCurrentConfig();
            }
        }
        catch (std::exception& exc)
        {
            delete m_state;
            cerr << "ERROR: OCIOIPNode updateConfig caught: " << exc.what()
                 << endl;
            m_state = 0;
            throw;
        }

        //
        //  XXX  we _could_ serialize the config and store it in the session,
        //  which would enable us to lock down the OCIO config of OCIO nodes
        //  read from a session file.  Currently their config will be semi
        //  arbitrary.
        //
        //  stringstream s;
        //  m_state->config->serialize(s);
        //  m_configText->front() = s.str();
        //

        m_configDescription->front() = m_state->config->getDescription();
        m_configWorkingDir->front() = m_state->config->getWorkingDir();

        m_state->display = m_state->config->getDefaultDisplay();
        m_state->view =
            m_state->config->getDefaultView(m_state->display.c_str());

        if (useRawConfig())
        {
            m_state->linear = "";
        }
        else if (getenv("OCIO"))
        {
            OCIO::ConstColorSpaceRcPtr linearColorSpace =
                m_state->config->getColorSpace(OCIO::ROLE_SCENE_LINEAR);
            m_state->linear =
                linearColorSpace ? linearColorSpace->getName() : "";
        }
        else
        {
            m_state->linear = "";
            std::cerr << "ERROR: OCIO environment variable not set"
                      << std::endl;
        }

        m_state->shaderID = "";

        updateContext();
        updateFunction();
    }

    void OCIOIPNode::updateContext()
    {
        m_state->context =
            m_state->config->getCurrentContext()->createEditableCopy();

        if (Component* context = component("ocio_context"))
        {
            const Component::Container& props = context->properties();

            for (size_t i = 0; i < props.size(); i++)
            {
                if (StringProperty* sp =
                        dynamic_cast<StringProperty*>(props[i]))
                {
                    if (!sp->empty())
                    {
                        m_state->context->setStringVar(sp->name().c_str(),
                                                       sp->front().c_str());
                    }
                }
            }
        }
    }

    namespace
    {

        char op_shaderLegal(char c) { return (isalnum(c)) ? c : '_'; }

        string shaderLegal(const string& s)
        {
            string ns;
            ns.resize(s.size());

            transform(s.begin(), s.end(), ns.begin(), op_shaderLegal);

            // OCIO replaces any consecutive '_' with just one '_' (See:
            // OCIO:GPUShaderDesc::setFunctionName) In order to keep the names
            // aligned use: GPUShaderDesc::getFunctionName() to get the name
            // OCIO uses to set RV's Shader Function and bind the parameters.
            ns = std::regex_replace(
                ns, std::regex("_+"),
                "_"); // one or more underscore: replace by only one.
            ns = std::regex_replace(ns, std::regex("_+$"),
                                    ""); // do not end by an underscore because
                                         // the code will append another one

            return ns;
        }

        // Add the 1D/3D LUT uniform as a shader function parameter to leverage
        // RV's current shader variables binding mechanism which rely on shader
        // variables being passed as function arguments for all its shaders.
        // Note that the OCIOv2 generated shader no longer passes the LUTs as
        // function arguments.
        void shaderAddLutAsParameter(std::string& inout_glsl,
                                     const std::string& lutSamplerName,
                                     const std::string& lutSamplerType)
        {
            const std::string from = "vec4 inPixel";
            std::string to = from + std::string(", ") + lutSamplerType
                             + std::string(" ") + lutSamplerName;
            inout_glsl = std::regex_replace(inout_glsl, std::regex(from), to);
        }

    }; // namespace

    OCIO::MatrixTransformRcPtr
    OCIOIPNode::createMatrixTransformXYZToRec709() const
    {
        OCIO::MatrixTransformRcPtr matrix_xyz_to_rec709 =
            OCIO::MatrixTransform::Create();
        double m44[16] = {3.240969941905,
                          -1.537383177570,
                          -0.498610760293,
                          0,
                          -0.969243636281,
                          1.875967501508,
                          0.041555057407,
                          0,
                          0.055630079697,
                          -0.203976958889,
                          1.056971514243,
                          0,
                          0,
                          0,
                          0,
                          1};
        matrix_xyz_to_rec709->setMatrix(m44);

        return matrix_xyz_to_rec709;
    }

    // Note: Ensure that the m_lock mutex is locked prior to calling this
    // function
    OCIO::MatrixTransformRcPtr OCIOIPNode::getMatrixTransformXYZToRec709()
    {
        if (!m_matrix_xyz_to_rec709)
        {
            m_matrix_xyz_to_rec709 = createMatrixTransformXYZToRec709();
        }

        return m_matrix_xyz_to_rec709;
    }

    // Note: Ensure that the m_lock mutex is locked prior to calling this
    // function
    OCIO::MatrixTransformRcPtr OCIOIPNode::getMatrixTransformRec709ToXYZ()
    {
        if (!m_matrix_rec709_to_xyz)
        {
            m_matrix_rec709_to_xyz = createMatrixTransformXYZToRec709();
            m_matrix_rec709_to_xyz->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
        }

        return m_matrix_rec709_to_xyz;
    }

    void OCIOIPNode::updateFunction()
    {
        if (m_activeProperty->size() != 1 || m_activeProperty->front() == 0)
            return;

        string ociofunction = stringProp("ocio.function", "color");

        boost::hash<string> string_hash;
        string inName = stringProp("ocio.inColorSpace", m_state->linear);

        if (inName.empty())
            return;

        try
        {
            OCIO::ConstColorSpaceRcPtr srcCS =
                m_state->config->getColorSpace(inName.c_str());
            OCIO::ConstProcessorRcPtr processor;
            ostringstream shaderName;

            if (ociofunction == "color")
            {
                //
                //  Emulate the nuke OCIOColor node
                //

                string outName =
                    stringProp("ocio_color.outColorSpace", m_state->linear);
                OCIO::ConstColorSpaceRcPtr dstCS =
                    m_state->config->getColorSpace(outName.c_str());
                processor = m_state->config->getProcessor(m_state->context,
                                                          srcCS, dstCS);

                size_t hashValue = string_hash(inName + outName);
                shaderName << "OCIO_c_" << shaderLegal(inName) << "_2_"
                           << shaderLegal(outName) << "_" << name() << "_"
                           << hex << hashValue;
            }
            else if (ociofunction == "look")
            {
                //
                //  Emulate the nuke OCIOLook node
                //

                OCIO::LookTransformRcPtr transform =
                    OCIO::LookTransform::Create();
                OCIO::TransformDirection direction =
                    OCIO::TRANSFORM_DIR_FORWARD;
                string looksName = stringProp("ocio_look.look", "");
                string outName =
                    stringProp("ocio_look.outColorSpace", m_state->linear);
                bool reverse = intProp("ocio_look.direction", 0) == 1;
                OCIO::ConstColorSpaceRcPtr dstCS =
                    m_state->config->getColorSpace(outName.c_str());

                transform->setLooks(looksName.c_str());

                if (reverse)
                {
                    //
                    //  NOTE: src and dst are SWAPPED here on purpose
                    //

                    transform->setSrc(outName.c_str());
                    transform->setDst(inName.c_str());
                    direction = OCIO::TRANSFORM_DIR_INVERSE;
                }
                else
                {
                    transform->setSrc(inName.c_str());
                    transform->setDst(outName.c_str());
                    direction = OCIO::TRANSFORM_DIR_FORWARD;
                }

                processor = m_state->config->getProcessor(m_state->context,
                                                          transform, direction);

                size_t hashValue = string_hash(inName + outName);
                shaderName << "OCIO_l_" << shaderLegal(looksName) << "_"
                           << name() << "_" << hex << hashValue << "_"
                           << direction;
            }
            else if (ociofunction == "display")
            {
                //
                //  Emulate the nuke OCIODisplay node
                //

                OCIO::DisplayViewTransformRcPtr transform =
                    OCIO::DisplayViewTransform::Create();
                string display =
                    stringProp("ocio_display.display", m_state->display);
                string view = stringProp("ocio_display.view", m_state->view);

                transform->setSrc(inName.c_str());
                transform->setDisplay(display.c_str());
                transform->setView(view.c_str());
                processor = m_state->config->getProcessor(
                    m_state->context, transform, OCIO::TRANSFORM_DIR_FORWARD);

                size_t hashValue = string_hash(inName + display + view);
                shaderName << "OCIO_d_" << shaderLegal(display) << "_"
                           << shaderLegal(view) << "_" << name() << "_" << hex
                           << hashValue;
            }
            else if (ociofunction == "synlinearize")
            {
                // The input transform can be specified in two ways: via a url
                // or via a data array
                string inTransformURL = stringProp("inTransform.url", "");
                if (inTransformURL.empty()
                    && (!m_inTransformData || m_inTransformData->size() == 0))
                {
                    TWK_THROW_EXC_STREAM(
                        "Either inTransform.url or inTransform.data property "
                        "needs to be set for synlinearize function");
                }

                if (!m_transform)
                {
                    m_transform = OCIO::GroupTransform::Create();

                    // Is the input transform specified via a data array ?
                    if (inTransformURL.empty())
                    {
                        // We need to provide a unique name to OCIOv2,
                        // otherwise it might use a potentially incorrect
                        // color transform with the same name already in its
                        // cache.
                        static int uniqueCounter = 0;
                        inTransformURL =
                            name() + "." + std::to_string(uniqueCounter++) + "."
                            + ConfigIOProxy::USE_IN_TRANSFORM_DATA_PROPERTY;
                    }

                    // Inverse the ICC transform
                    OCIO::FileTransformRcPtr transform =
                        OCIO::FileTransform::Create();
                    transform->setSrc(inTransformURL.c_str());
                    transform->setInterpolation(OCIO::INTERP_BEST);
                    transform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
                    m_transform->appendTransform(transform);

                    // Concatenate it with the RV's working space transform
                    // which is currently assumed to be 709 linear like the
                    // original EXR spec.
                    m_transform->appendTransform(
                        getMatrixTransformXYZToRec709());
                }

                processor = m_state->config->getProcessor(
                    m_state->context, m_transform, OCIO::TRANSFORM_DIR_FORWARD);

                size_t hashValue = string_hash(name());
                shaderName << "OCIO_sl_" << name() << "_" << hex << hashValue;
            }
            else if (ociofunction == "syndisplay")
            {
                // The outTransform.url property typically refers to an ICC
                // monitor profile
                const string outTransformURL =
                    stringProp("outTransform.url", "");
                if (outTransformURL.empty())
                {
                    TWK_THROW_EXC_STREAM("outTransform.url property needs to "
                                         "be set for syndisplay function");
                }

                if (!m_transform)
                {
                    m_transform = OCIO::GroupTransform::Create();

                    // RV's working space is currently assumed to be 709
                    // linear like the original EXR spec. In the display
                    // case this transform is inverted.
                    m_transform->appendTransform(
                        getMatrixTransformRec709ToXYZ());

                    // Concatenate the inverse workingSpaceTransform with
                    // the ICC monitor profile transforma
                    OCIO::FileTransformRcPtr transform =
                        OCIO::FileTransform::Create();
                    transform->setSrc(outTransformURL.c_str());
                    transform->setInterpolation(OCIO::INTERP_BEST);
                    m_transform->appendTransform(transform);
                }

                processor = m_state->config->getProcessor(
                    m_state->context, m_transform, OCIO::TRANSFORM_DIR_FORWARD);

                size_t hashValue = string_hash(outTransformURL);
                shaderName << "OCIO_sd_" << name() << "_" << hex << hashValue;
            }

            OCIO::GpuShaderDescRcPtr shaderDesc =
                OCIO::GpuShaderDesc::CreateShaderDesc();
            shaderDesc->setFunctionName(shaderName.str().c_str());
            shaderDesc->setLanguage(GPULanguage);
            OCIO::ConstGPUProcessorRcPtr gpuProcessor;

            if (evOCIOUseLegacyGPUProcessor.getValue())
            {
                const int legacyLutSize =
                    intProp("ocio.lut3DSize", evOCIOLegacyLut3DSize.getValue());
                gpuProcessor = processor->getOptimizedLegacyGPUProcessor(
                    OCIO::OPTIMIZATION_DEFAULT, legacyLutSize);
            }
            else
            {
                gpuProcessor = processor->getOptimizedGPUProcessor(
                    OCIO::OPTIMIZATION_DEFAULT);
            }

            // Fills the shaderDesc from the proc.
            gpuProcessor->extractGpuShaderInfo(shaderDesc);

            string shaderCacheID = gpuProcessor->getCacheID();

            if (m_state->shaderID != shaderCacheID)
            {
                if (m_state->function)
                {
                    m_state->function->retire();
                }

                string glsl(shaderDesc->getShaderText());

                m_1DLUTs.clear();
                const unsigned int numTextures = shaderDesc->getNumTextures();
                for (unsigned idx = 0; idx < numTextures; ++idx)
                {
                    m_1DLUTs.push_back(std::make_shared<OCIO1DLUT>(
                        shaderDesc, idx, shaderCacheID));

                    // Add the LUTs'shader uniform as a shader function
                    // parameter
                    shaderAddLutAsParameter(glsl, m_1DLUTs[idx]->samplerName(),
                                            m_1DLUTs[idx]->samplerType());
                }

                m_3DLUTs.clear();
                const unsigned int num3DTextures =
                    shaderDesc->getNum3DTextures();
                for (unsigned idx = 0; idx < num3DTextures; ++idx)
                {
                    m_3DLUTs.push_back(std::make_shared<OCIO3DLUT>(
                        shaderDesc, idx, shaderCacheID));

                    // Add the LUTs'shader uniform as a shader function
                    // parameter
                    shaderAddLutAsParameter(glsl, m_3DLUTs[idx]->samplerName(),
                                            m_3DLUTs[idx]->samplerType());
                }

                m_state->function = new Shader::Function(
                    shaderDesc->getFunctionName(), glsl,
                    Shader::Function::Color, numTextures + num3DTextures);
                m_state->shaderID = shaderCacheID;

                if (Shader::debuggingType() != Shader::NoDebugInfo)
                {
                    cout << "OCIONode: " << name() << " new shaderID "
                         << shaderCacheID << endl
                         << "OCIONode: " << numTextures << "x 1D LUTs, "
                         << num3DTextures << "x 3D LUTs"
                         << "OCIONode:     new Shader '"
                         << shaderDesc->getFunctionName() << "':" << endl
                         << glsl << endl;
                }
            }
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: OCIOIPNode: " << exc.what() << endl;
        }
    }

    IPImage* OCIOIPNode::evaluate(const Context& context)
    {
        IPImage* image = IPNode::evaluate(context);
        if (!image)
            return IPImage::newNoImage(this, "No Input");

        if (m_activeProperty->size() != 1 || m_activeProperty->front() == 0
            || !m_state->function)
            return image;

        string order = propertyValue(m_channelOrder, "");
        int flood = propertyValue(m_channelFlood, 0);
        int dither = propertyValue(m_dither, 0);
        size_t seed = context.frame + DispTransform2DIPNode::transformHash();
        string ociofunction = stringProp("ocio.function", "color");

        //
        //  We don't really know _why_ we need an intermediate-buffer-generating
        //  node in the Display Pipeline, but we seem to.  In particular, if
        //  you've replaced the RVColorNode with an OCIODisplay node, and you
        //  don't generate an intermediate buffer, the result is much lower res.
        //  So now we generate an intermediate buffer in the "display" case.
        //

        if (ociofunction != "display" && ociofunction != "syndisplay")
        {
            IPImageVector images(1);
            IPImageSet modifiedImages;
            images[0] = image;

            convertBlendRenderTypeToIntermediate(images, modifiedImages);
            balanceResourceUsage(IPNode::accumulate, images, modifiedImages, 8,
                                 8, 81, 1);
        }
        else
        {
            IPImage* child = image;

            //
            //  Don't make a separate intermediate images unless we really have
            //  to.
            //
            if (willConvertToIntermediate(child))
            {
                image = new IPImage(this, IPImage::BlendRenderType,
                                    context.viewWidth, context.viewHeight, 1.0,
                                    IPImage::IntermediateBuffer,
                                    IPImage::FloatDataType);

                image->appendChild(child);
                image->shaderExpr = Shader::newSourceRGBA(image);
            }
            else
            {
                IPImageVector images(1);
                IPImageSet modifiedImages;
                images[0] = image = child;

                convertBlendRenderTypeToIntermediate(images, modifiedImages);
                balanceResourceUsage(IPNode::accumulate, images, modifiedImages,
                                     8, 8, 81, 1);
                image = images[0];
            }
        }

        if (image->mergeExpr || image->shaderExpr)
        {
            const Shader::Function* F = m_state->function;
            Shader::ArgumentVector args(F->parameters().size());
            Shader::Expression*& expr =
                image->mergeExpr ? image->mergeExpr : image->shaderExpr;

            args[0] = new Shader::BoundExpression(F->parameters()[0], expr);

            // Add the 3D LUTs expressions (if any)
            for (unsigned idx = 0; idx < m_3DLUTs.size(); ++idx)
            {
                args[idx + 1] = new Shader::BoundSampler(
                    F->parameters()[idx + 1],
                    Shader::ImageOrFB(
                        m_3DLUTs[m_3DLUTs.size() - 1 - idx]->lutfb(), 0));
            }

            // Add the 1D LUTs expressions (if any)
            for (unsigned idx = 0; idx < m_1DLUTs.size(); ++idx)
            {
                args[idx + m_3DLUTs.size() + 1] = new Shader::BoundSampler(
                    F->parameters()[idx + m_3DLUTs.size() + 1],
                    Shader::ImageOrFB(
                        m_1DLUTs[m_1DLUTs.size() - 1 - idx]->lutfb(), 0));
            }

            expr = new Shader::Expression(F, args, image);

            if (ociofunction == "display")
            {

                // Handle channel reording and flooding
                Mat44f Ca;
                if (order != "")
                {
                    Mat44f M(0.0);

                    for (int i = 0; i < order.size() && i < 4; i++)
                    {
                        switch (order[i])
                        {
                        case 'R':
                            M(i, 0) = 1.0;
                            break;
                        case 'G':
                            M(i, 1) = 1.0;
                            break;
                        case 'B':
                            M(i, 2) = 1.0;
                            break;
                        case 'A':
                            M(i, 3) = 1.0;
                            break;
                        case '1':
                            M(i, 0) = 1.0, M(i, 1) = 1.0, M(i, 2) = 1.0, M(i, 3) = 1.0;
                            break;
                        default:
                        case '0':
                            M(i, 0) = 0.0, M(i, 1) = 0.0, M(i, 2) = 0.0, M(i, 3) = 0.0;
                            break;
                        }
                    }

                    Ca = M * Ca;
                }

                if (flood != 0)
                {
                    int ch = flood;

                    Mat44f M = Rec709FullRangeRGBToYUV8<float>();
                    const float rw709 = M.m00;
                    const float gw709 = M.m01;
                    const float bw709 = M.m02;

                    Mat44f F;

                    switch (ch)
                    {
                    case 1:
                        F = Mat44f(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1);
                        break;
                    case 2:
                        F = Mat44f(0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);
                        break;
                    case 3:
                        F = Mat44f(0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1);
                        break;
                    case 4:
                        F = Mat44f(0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1);
                        break;
                    case 5:
                        F = Mat44f(rw709, gw709, bw709, 0, rw709, gw709, bw709, 0,
                                rw709, gw709, bw709, 0, 0, 0, 0, 1);
                        break;
                    default:
                        // nothing
                        break;
                    }
                    Ca = F * Ca;
                }

                if (Ca != Mat44f())
                {
                    expr = Shader::newColorMatrix4D(expr, Ca);
                }

                if (dither > 0)
                {
                    expr = Shader::newDither(image, expr, dither, seed);
                }
            }

            if (ociofunction == "display" && image->shaderExpr)
            {
                image->resourceUsage =
                    image->shaderExpr->computeResourceUsageRecursive();
            }
        }

        return image;
    }

    void OCIOIPNode::propertyChanged(const Property* p)
    {
        if (Component* context = component("ocio_context"))
        {
            if (context->hasProperty(p))
            {
                updateContext();
            }
        }

        // synlinearize/syndisplay functions:
        // Reset transforms and associated color transform files if a linked
        // property changed
        if (p == m_inTransformURL || p == m_inTransformData
            || p == m_outTransformURL)
        {
            m_transform.reset();
        }

        updateFunction();

        IPNode::propertyChanged(p);
    }

    void OCIOIPNode::readCompleted(const string& t, unsigned int v)
    {
        updateContext();
        updateFunction();

        IPNode::readCompleted(t, v);
    }

} // namespace IPCore
