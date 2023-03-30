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
// The OCIO::GPU_LANGUAGE_UNKNOWN no longer exists
// and the value of zero is used by Nvidia Cg shader
#define GPU_LANGUAGE_UNKNOWN -1
OCIO::GpuLanguage GPULanguage = (OCIO::GpuLanguage)GPU_LANGUAGE_UNKNOWN;
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
    pthread_mutex_init(&m_lock, NULL);

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
            //GPULanguage = OCIO::GPU_LANGUAGE_GLSL_1_0;
            // The lowest available value in newer OCIO is now 1.2
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
    pthread_mutex_destroy(&m_lock);
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
        cout << "ERROR: OCIOIPNode updateConfig caught: " << exc.what() << endl;
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
        //m_state->context = OCIO::Context::Create(); // destroys old one

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

    return ns;
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
            OCIO::TransformDirection   direction;
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
            string                      display   = stringProp("ocio_display.display", "");
            string                      view      = stringProp("ocio_display.view", "");

#if 0
            transform->setInputColorSpaceName(inName.c_str());
#endif
            transform->setDisplay(display.c_str());
            transform->setView(view.c_str());
            processor = m_state->config->getProcessor(m_state->context, 
                                                      transform,
                                                      OCIO::TRANSFORM_DIR_FORWARD);

            size_t hashValue = string_hash(inName + display + view);
            shaderName << "OCIO_d_" << shaderLegal(display) << "_" << shaderLegal(view) << "_" << name() << "_" << hex << hashValue;
        }
#if 0
        OCIO::GpuShaderDesc shaderDesc;
        shaderDesc.setLanguage(GPULanguage);
        shaderDesc.setFunctionName(shaderName.str().c_str());
        shaderDesc.setLut3DEdgeLen(lutSize);

        pthread_mutex_lock(&m_lock);

        //
        // The story so far: We always thought OCIO _always_ produced hw
        // shading that required a 3D LUT of some kind.  This is (mostly) not
        // true.
        //
        // The Processor holds 3 vectors of "Ops": those that happen before any
        // LUT (m_gpuOpsHwPreProcess), those that must be implmented in a LUT
        // (m_gpuOpsCpuLatticeProcess), and those that happen after the LUT
        // (m_gpuOpsHwPostProcess).  These lists are created by
        // PartitionGPUOps() and if all the incoming Ops are analytical (have
        // direct GPU implementation) then all the ops will go into the first
        // list and the others will be empty.  In this case, OCIO will not
        // (need not) generate a 3D LUT at all.  AND the "3DLUT ID" generated
        // by getGpuLut3DCacheID() will be "<NULL>".  As far as I can tell,
        // there's no other way for calling code to tell that a 3DLUT is
        // unnecessary
        //
        // BUT, there is hack in OCIO code (to prevent segfault, comment says)
        // so that on Mac only, it _always_ generates a 3D LUT.  When the LUT
        // is not necessary, it will be an identity LUT, although the ID is
        // still "<NULL>".
        //
        // ALSO BUT, the shader that OCIO generates _always_ has LUT argument,
        // whether it is going to use it or not.
        //
        // Now we only add a lut FB if one is needed; NOTE that core OCIO code
        // has also been changed to support this (now it only produces a shader
        // with a LUT parameter IF it needs one.
        //

        string lut3dCacheID  = processor->getGpuLut3DCacheID(shaderDesc);
        string shaderCacheID = processor->getGpuShaderTextCacheID(shaderDesc);

        if (m_state->lutID != lut3dCacheID)
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
                processor->getGpuLut3D(m_lutfb->pixels<float>(), shaderDesc);
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

            ostringstream glsl;
            glsl << processor->getGpuShaderText(shaderDesc);

            m_state->function = new Shader::Function(shaderName.str(), 
                                                     glsl.str(),
                                                     Shader::Function::Color,
                                                     1);
            m_state->shaderID = shaderCacheID;

            if (Shader::debuggingType() != Shader::NoDebugInfo)
            {
                cerr << "OCIONode: " << name() << " new shaderID " << shaderCacheID << endl <<
                        "OCIONode:     new Shader '" << shaderName.str() << "':" << endl << glsl.str() << endl;
            }
        }

        pthread_mutex_unlock(&m_lock);
#endif
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
        cout << "ERROR: OCIOIPNode: " << exc.what() << endl;
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
