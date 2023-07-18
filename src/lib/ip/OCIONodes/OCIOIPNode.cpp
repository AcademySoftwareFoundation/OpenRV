//
//  Copyright (c) 2012 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <OCIONodes/OCIOIPNode.h>
#include <IPCore/SessionIPNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/IPGraph.h>
#include <IPCore/ShaderCommon.h>
#include <TwkUtil/EnvVar.h>
#include <OpenColorIO/OpenColorIO.h>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <regex>

namespace OCIO = OCIO_NAMESPACE;

namespace IPCore {
using namespace std;
using namespace boost;

static ENVVAR_INT( evOCIOLut3DSize, "RV_OCIO_3D_LUT_SIZE", 32 );

struct OCIOState
{
    OCIO::ConstConfigRcPtr  config;
    OCIO::ContextRcPtr      context;
    string                  display;
    string                  view;
    string                  linear;
    string                  shaderID;
    string                  lutID;
    Shader::Function*       function;

    OCIOState() : function(0) {}
};

namespace {
#define GPU_LANGUAGE_UNKNOWN OCIO::GpuLanguage::GPU_LANGUAGE_CG
OCIO::GpuLanguage GPULanguage = GPU_LANGUAGE_UNKNOWN;
}

string
OCIOIPNode::stringProp(const string& name, const string& defaultValue) const
{
    if (const StringProperty* p = property<StringProperty>(name))
    {
        if (!p->empty() && p->front() != "") return p->front();
    }

    return defaultValue;
}

int
OCIOIPNode::intProp(const string& name, int defaultValue) const
{
    if (const IntProperty* p = property<IntProperty>(name))
    {
        if (!p->empty()) return p->front();
    }

    return defaultValue;
}

OCIOIPNode::OCIOIPNode(const string& name,
                       const NodeDefinition* def,
                       IPGraph* graph,
                       GroupIPNode* group)
    : IPNode(name, def, graph, group),
      m_lutfb(0)
{
    Property::Info* info = new Property::Info();
    info->setPersistent(false);

    string func = def->stringValue("defaults.function", "color");
    declareProperty<StringProperty>("ocio.function", func);

    m_activeProperty = declareProperty<IntProperty>("ocio.active", 1);

    declareProperty<FloatProperty>("ocio.lut", info);
    m_lutSize = declareProperty<IntProperty>("ocio.lut3DSize", evOCIOLut3DSize.getValue());
    declareProperty<StringProperty>("ocio.inColorSpace", "");

    declareProperty<StringProperty>("ocio_color.outColorSpace", "");

    declareProperty<StringProperty>("ocio_look.look", "");
    declareProperty<IntProperty>("ocio_look.direction", 0);
    declareProperty<StringProperty>("ocio_look.outColorSpace", "");

    declareProperty<StringProperty>("ocio_display.display", "");
    declareProperty<StringProperty>("ocio_display.view", "");
    //declareProperty<FloatProperty>("ocio_display.exposure");
    //declareProperty<FloatProperty>("ocio_display.gamma");
    //declareProperty<IntProperty>("ocio_display.channels");
    
    //
    //  Read-only properties to report config info to the user
    //
    m_configDescription = declareProperty<StringProperty>("config.description", "");
    m_configWorkingDir  = declareProperty<StringProperty>("config.workingDir", "");

    m_state = new OCIOState;

    if (!getenv("OCIO_LOGGING_LEVEL")) OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);

    updateConfig();

    if (GPULanguage == GPU_LANGUAGE_UNKNOWN)
    {
        IPNode* session = graph->sessionNode();
        int major = session->property<IntProperty>("opengl.glsl.majorVersion")->front();
        int minor = session->property<IntProperty>("opengl.glsl.minorVersion")->front();

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
}

OCIOIPNode::~OCIOIPNode()
{
    if (m_state->function) m_state->function->retire();
    delete m_state;
    if (m_lutfb)
    {
        if (!m_lutfb->hasStaticRef() || m_lutfb->staticUnRef()) 
        {
            delete m_lutfb;
        }
    }
}

void
OCIOIPNode::updateConfig()
{
    try 
    {
        m_state->config = OCIO::GetCurrentConfig();
    }
    catch (std::exception& exc)
    {
        delete m_state;
        cerr << "ERROR: OCIOIPNode updateConfig caught: " << exc.what() << endl;
        m_state = 0;
        throw;
    }

    //
    //  XXX  we _could_ serialize the config and store it in the session, which
    //  would enable us to lock down the OCIO config of OCIO nodes read from a
    //  session file.  Currently their config will be semi arbitrary. 
    //
    //  stringstream s;
    //  m_state->config->serialize(s);
    //  m_configText->front() = s.str();
    //

    m_configDescription->front() = m_state->config->getDescription();
    m_configWorkingDir->front()  = m_state->config->getWorkingDir();

    m_state->display  = m_state->config->getDefaultDisplay();
    m_state->view     = m_state->config->getDefaultView(m_state->display.c_str());
    m_state->linear   = m_state->config->getColorSpace(OCIO::ROLE_SCENE_LINEAR)->getName();
    m_state->shaderID = "";
    m_state->lutID    = "";

    if (m_state->function) m_state->function->retire();
    m_state->function = 0;

    updateContext();
}

void
OCIOIPNode::updateContext()
{
    m_state->context = m_state->config->getCurrentContext()->createEditableCopy();

    if (Component* context = component("ocio_context"))
    {
        const Component::Container& props = context->properties();

        for (size_t i = 0; i < props.size(); i++)
        {
            if (StringProperty* sp = dynamic_cast<StringProperty*>(props[i]))
            {
                if (!sp->empty())
                {
                    m_state->context->setStringVar(sp->name().c_str(), sp->front().c_str());
                }
            }
        }
    }
}

namespace {

char 
op_shaderLegal(char c) 
{ 
    return (isalnum(c)) ? c : '_'; 
}

string
shaderLegal(const string& s)
{
    string ns;
    ns.resize(s.size());

    transform(s.begin(), s.end(), ns.begin(), op_shaderLegal);

    // OCIO replaces any consecutive '_' with just one '_' (See: OCIO:GPUShaderDesc::setFunctionName)
    // In order to keep the names aligned use: GPUShaderDesc::getFunctionName() to get the name OCIO uses to set RV's Shader Function and bind the parameters.
    ns = std::regex_replace(ns, std::regex("_+"), "_");   // one or more underscore: replace by only one.
    ns = std::regex_replace(ns, std::regex("_+$"), "");   // do not end by an underscore because the code will append another one

    return ns;
}

// Add the 3D LUT uniform as a shader function parameter to leverage RV's current 
// shader variables binding mechanism which rely on shader variables being passed 
// as function arguments for all its shaders.
// Note that the OCIOv2 generated shader no longer passes the LUTs as function 
// arguments.
void 
shaderAdd3DLutAsParameter(std::string& inout_glsl, const std::string& lutSamplerName)
{
    const std::string from = "vec4 inPixel";
    std::string to = from + std::string(", sampler3D ") + lutSamplerName;
    inout_glsl = std::regex_replace( inout_glsl, std::regex(from), to );
}

};

IPImage*
OCIOIPNode::evaluate(const Context& context)
{
    string ociofunction = stringProp("ocio.function", "color");
    IPImage* image = 0;

    //
    //  We don't really know _why_ we need an intermediate-buffer-generating
    //  node in the Display Pipeline, but we seem to.  In particular, if you've
    //  replaced the RVColorNode with an OCIODisplay node, and you don't
    //  generate an intermediate buffer, the result is much lower res.  So now
    //  we generate an intermediate buffer in the "display" case.
    //

    if (ociofunction != "display")
    {
        image = IPNode::evaluate(context);
        if (!image) return IPImage::newNoImage(this, "No Input");
        if (m_activeProperty->size() != 1 || m_activeProperty->front() == 0) return image;

        IPImageVector images(1);
        IPImageSet modifiedImages;
        images[0] = image;

        convertBlendRenderTypeToIntermediate(images, modifiedImages);
        balanceResourceUsage(IPNode::accumulate,
                            images,
                            modifiedImages,
                            8, 8, 81, 1);
    }
    else
    {
        Context newContext = context;
        IPImage* child = IPNode::evaluate(newContext);
        if (!child) return IPImage::newNoImage(this, "No Input");

        if (m_activeProperty->size() != 1 || m_activeProperty->front() == 0) return child;

        //
        //  Don't make a separate intermediate images unless we really have to.
        //
        if (willConvertToIntermediate(child))
        {
            image = new IPImage(this, 
                                IPImage::BlendRenderType,
                                context.viewWidth,
                                context.viewHeight,
                                1.0,
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
            balanceResourceUsage(IPNode::accumulate,
                                images,
                                modifiedImages,
                                8, 8, 81, 1);
            image = images[0];
        }
    }
    boost::hash<string> string_hash;
    string inName       = stringProp("ocio.inColorSpace", m_state->linear);
    int    lutSize      = intProp("ocio.lut3DSize", evOCIOLut3DSize.getValue());

    try
    {
        OCIO::ConstColorSpaceRcPtr srcCS = m_state->config->getColorSpace(inName.c_str());
        OCIO::ConstProcessorRcPtr  processor;
        ostringstream shaderName;

        if (ociofunction == "color")
        {
            //
            //  Emulate the nuke OCIOColor node
            //

            string outName = stringProp("ocio_color.outColorSpace", "");
            OCIO::ConstColorSpaceRcPtr dstCS = m_state->config->getColorSpace(outName.c_str());
            processor = m_state->config->getProcessor(m_state->context, srcCS, dstCS);

            size_t hashValue = string_hash(inName + outName);
            shaderName << "OCIO_c_" << shaderLegal(inName) << "_2_" << shaderLegal(outName) << "_" << name() << "_" << hex << hashValue;
        }
        else if (ociofunction == "look")
        {
            //
            //  Emulate the nuke OCIOLook node
            //

            OCIO::LookTransformRcPtr   transform = OCIO::LookTransform::Create();
            OCIO::TransformDirection   direction = OCIO::TRANSFORM_DIR_FORWARD;
            string                     looksName = stringProp("ocio_look.look", "");
            string                     outName   = stringProp("ocio_look.outColorSpace", m_state->linear);
            bool                       reverse   = intProp("ocio_look.direction", 0) == 1;
            OCIO::ConstColorSpaceRcPtr dstCS     = m_state->config->getColorSpace(outName.c_str());

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

            processor = m_state->config->getProcessor(m_state->context, transform, direction);

            size_t hashValue = string_hash(inName + outName);
            shaderName << "OCIO_l_" << shaderLegal(looksName) << "_" << name() << "_" << hex << hashValue << "_" << direction;
        }
        else if (ociofunction == "display")
        {
            //
            //  Emulate the nuke OCIODisplay node
            //

            OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
            string                          display   = stringProp("ocio_display.display", "");
            string                          view      = stringProp("ocio_display.view", "");

            transform->setSrc(inName.c_str());
            transform->setDisplay(display.c_str());
            transform->setView(view.c_str());
            processor = m_state->config->getProcessor(m_state->context, 
                                                      transform,
                                                      OCIO::TRANSFORM_DIR_FORWARD);

            size_t hashValue = string_hash(inName + display + view);
            shaderName << "OCIO_d_" << shaderLegal(display) << "_" << shaderLegal(view) << "_" << name() << "_" << hex << hashValue;
        }

        QMutexLocker(&this->m_lock);
        OCIO::ConstGPUProcessorRcPtr legacyGPUProcessor = processor->getOptimizedLegacyGPUProcessor(OCIO::OPTIMIZATION_NONE, lutSize);
        OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(GPULanguage);
        shaderDesc->setFunctionName(shaderName.str().c_str());

        // Fills the shaderDesc from the proc.
        legacyGPUProcessor->extractGpuShaderInfo(shaderDesc);

        // When using getOptimizedLegacyGPUProcessor, getNumTextures(), which is for any Lut1Ds, 
        // should always be 0 and getNum3DTextures() should never be more than 1/
        if (shaderDesc->getNumTextures() != 0 && shaderDesc->getNum3DTextures() > 1)
        {
            cerr << "OCIO IP Node error: " << "Unknown case about number of textures in Nuke node transform." 
                 << "num text: " << shaderDesc->getNumTextures() 
                 << ", num 3d tex: " << shaderDesc->getNum3DTextures()
                 << endl;
            return nullptr;
        }

        string lut3dCacheID  = legacyGPUProcessor->getCacheID();
        string shaderCacheID = lut3dCacheID;
        if (m_state->lutID != lut3dCacheID && 1 == shaderDesc->getNum3DTextures())
        {
            if (m_lutfb)
            {
                if (!m_lutfb->hasStaticRef() || m_lutfb->staticUnRef()) 
                {
                    delete m_lutfb;
                }        
                m_lutfb = 0;
            }            

            if (lut3dCacheID != "<NULL>")
            {
                vector<string> channels(3);
                channels[0] = "R";
                channels[1] = "G";
                channels[2] = "B";

                m_lutfb = new TwkFB::FrameBuffer(FrameBuffer::NormalizedCoordinates,
                                                 lutSize, lutSize, lutSize, 3, FrameBuffer::FLOAT,
                                                 0, &channels, FrameBuffer::BOTTOMLEFT,
                                                 true);

		        m_lutfb->staticRef();
                m_lutfb->setIdentifier(lut3dCacheID);

                const char* tex_name = nullptr;
                const char* sampler_name = nullptr;
                unsigned int lut3DTexEdgeLen = 0;
                OCIO::Interpolation lut3DInterpolation = OCIO::INTERP_BEST;
                shaderDesc->get3DTexture(0, tex_name, sampler_name, lut3DTexEdgeLen, lut3DInterpolation);
                m_lutSamplerName = sampler_name;

                if (lut3DTexEdgeLen == lutSize)
                {
                    const float * temp = nullptr;
                    shaderDesc->get3DTextureValues(0, temp);
                    memcpy(m_lutfb->pixels<float>(), temp, 3*sizeof(float)*lutSize*lutSize*lutSize);
                }                
                else
                {
                    cerr << "ERROR: OCIONode: "<< name() << " - Expected 3D LUT Size = " << lutSize 
                         << ", Found = " << lut3DTexEdgeLen << endl;
                }
            }
            m_state->lutID = lut3dCacheID;

            if (Shader::debuggingType() == Shader::AllDebugInfo)
            {
                cerr << "OCIONode: "<< name() << " new lutID " << lut3dCacheID << endl;
            }
        }

        if (m_state->shaderID != shaderCacheID)
        {
            if (m_state->function) m_state->function->retire();

            string glsl(shaderDesc->getShaderText());

            // Add the 3D LUT uniform as a shader function parameter if any
            if (m_lutfb)
            {
                shaderAdd3DLutAsParameter(glsl, m_lutSamplerName);
            }

            m_state->function = new Shader::Function(shaderDesc->getFunctionName(),
                                                     glsl,
                                                     Shader::Function::Color,
                                                     1);
            m_state->shaderID = shaderCacheID;

            if (Shader::debuggingType() != Shader::NoDebugInfo)
            {
                cerr << "OCIONode: " << name() << " new shaderID " << shaderCacheID << endl <<
                        "OCIONode:     new Shader '" << shaderDesc->getFunctionName() << "':" << endl << glsl << endl;
            }
        }

        const Shader::Function* F = m_state->function;
        Shader::ArgumentVector args(F->parameters().size());

        if (image->mergeExpr)
        {
                         args[0] = new Shader::BoundExpression(F->parameters()[0], image->mergeExpr); 
            if (m_lutfb) args[1] = new Shader::BoundSampler(F->parameters()[1], Shader::ImageOrFB(m_lutfb, 0));
            image->mergeExpr = new Shader::Expression(F, args, image);
        }
        else if (image->shaderExpr)
        {
                         args[0] = new Shader::BoundExpression(F->parameters()[0], image->shaderExpr); 
            if (m_lutfb) args[1] = new Shader::BoundSampler(F->parameters()[1], Shader::ImageOrFB(m_lutfb, 0));
            image->shaderExpr = new Shader::Expression(F, args, image);
        }

        if (ociofunction == "display" && image->shaderExpr)
        {
            image->resourceUsage = image->shaderExpr->computeResourceUsageRecursive();
        }
    }
    catch (std::exception& exc)
    {
        cerr << "ERROR: OCIOIPNode: " << exc.what() << endl;
    }

    return image;
}

void 
OCIOIPNode::propertyChanged(const Property* p)
{
    if (p == m_lutSize)
    {
        if (m_lutfb)
        {
            if (!m_lutfb->hasStaticRef() || m_lutfb->staticUnRef()) 
            {
                delete m_lutfb;
            }        
            m_lutfb = 0;
        }
    }

    if (Component* context = component("ocio_context"))
    {
        if (context->hasProperty(p)) updateContext();
    }

    IPNode::propertyChanged(p);
}

void 
OCIOIPNode::readCompleted(const string& t, unsigned int v)
{
    updateContext();

    IPNode::readCompleted(t,v);
}

} // Rv
